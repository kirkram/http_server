package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"flag"
	"fmt"
	"io"
	"math"
	"net"
	"net/http"
	"os"
	"sort"
	"strings"
	"sync"
	"time"
)

type headerList struct {
	values []string
}

func (h *headerList) String() string {
	return strings.Join(h.values, ", ")
}

func (h *headerList) Set(value string) error {
	if strings.TrimSpace(value) == "" {
		return fmt.Errorf("header cannot be empty")
	}
	h.values = append(h.values, value)
	return nil
}

type benchConfig struct {
	name         string
	url          string
	method       string
	concurrency  int
	duration     time.Duration
	warmup       time.Duration
	timeout      time.Duration
	bodySize     int
	insecureTLS  bool
	headerValues []string
}

type workerStats struct {
	ops             uint64
	responses       uint64
	success         uint64
	transportErr    uint64
	responseReadErr uint64
	bytesRead       uint64
	statusCounts    map[int]uint64
	latenciesNs     []int64
}

type benchResult struct {
	name            string
	url             string
	method          string
	duration        time.Duration
	concurrency     int
	ops             uint64
	responses       uint64
	success         uint64
	transportErr    uint64
	responseReadErr uint64
	bytesRead       uint64
	statusCounts    map[int]uint64
	latenciesNs     []int64
}

type latencyStats struct {
	avg time.Duration
	p50 time.Duration
	p90 time.Duration
	p95 time.Duration
	p99 time.Duration
	max time.Duration
}

func main() {
	var (
		url          = flag.String("url", "", "Primary target URL (required)")
		name         = flag.String("name", "target", "Primary target display name")
		compareURL   = flag.String("compare-url", "", "Optional second target URL")
		compareName  = flag.String("compare-name", "compare", "Second target display name")
		method       = flag.String("method", "GET", "HTTP method for requests")
		concurrency  = flag.Int("concurrency", 64, "Number of concurrent workers")
		durationText = flag.String("duration", "15s", "Benchmark duration (e.g. 10s, 1m)")
		warmupText   = flag.String("warmup", "2s", "Warmup duration before measurement")
		timeoutText  = flag.String("timeout", "5s", "Per-request timeout")
		bodySize     = flag.Int("body-size", 0, "Request body size in bytes (useful for POST/PUT)")
		insecureTLS  = flag.Bool("insecure", false, "Skip TLS certificate verification")
		headers      headerList
	)

	flag.Var(&headers, "header", "Custom header 'Key: Value' (can be repeated)")
	flag.Parse()

	if *url == "" {
		fmt.Fprintln(os.Stderr, "error: -url is required")
		os.Exit(1)
	}
	if *concurrency <= 0 {
		fmt.Fprintln(os.Stderr, "error: -concurrency must be > 0")
		os.Exit(1)
	}
	if *bodySize < 0 {
		fmt.Fprintln(os.Stderr, "error: -body-size cannot be negative")
		os.Exit(1)
	}

	duration, err := time.ParseDuration(*durationText)
	if err != nil || duration <= 0 {
		fmt.Fprintln(os.Stderr, "error: invalid -duration")
		os.Exit(1)
	}
	warmup, err := time.ParseDuration(*warmupText)
	if err != nil || warmup < 0 {
		fmt.Fprintln(os.Stderr, "error: invalid -warmup")
		os.Exit(1)
	}
	timeout, err := time.ParseDuration(*timeoutText)
	if err != nil || timeout <= 0 {
		fmt.Fprintln(os.Stderr, "error: invalid -timeout")
		os.Exit(1)
	}

	base := benchConfig{
		method:       strings.ToUpper(*method),
		concurrency:  *concurrency,
		duration:     duration,
		warmup:       warmup,
		timeout:      timeout,
		bodySize:     *bodySize,
		insecureTLS:  *insecureTLS,
		headerValues: headers.values,
	}

	first := base
	first.name = *name
	first.url = *url

	firstResult := runBenchmark(first)
	printResult(firstResult)

	if *compareURL != "" {
		second := base
		second.name = *compareName
		second.url = *compareURL
		secondResult := runBenchmark(second)
		printResult(secondResult)
		printComparison(firstResult, secondResult)
	}
}

func runBenchmark(cfg benchConfig) benchResult {
	client := buildClient(cfg.timeout, cfg.insecureTLS, cfg.concurrency)
	headers := parseHeaders(cfg.headerValues)
	payload := make([]byte, cfg.bodySize)
	for i := range payload {
		payload[i] = 'x'
	}

	if len(payload) > 0 && headers.Get("Content-Type") == "" {
		headers.Set("Content-Type", "application/octet-stream")
	}

	if cfg.warmup > 0 {
		fmt.Printf("\n[%s] Warmup for %s ...\n", cfg.name, cfg.warmup)
		_ = runPhase(cfg, client, headers, payload, cfg.warmup, false)
	}
	fmt.Printf("\n[%s] Running benchmark for %s (%d workers)\n",
		cfg.name, cfg.duration, cfg.concurrency)
	result := runPhase(cfg, client, headers, payload, cfg.duration, true)
	client.CloseIdleConnections()
	return result
}

func runPhase(cfg benchConfig, client *http.Client, headers http.Header, payload []byte,
	phaseDuration time.Duration, collect bool) benchResult {
	ctx, cancel := context.WithTimeout(context.Background(), phaseDuration)
	defer cancel()

	statsCh := make(chan workerStats, cfg.concurrency)
	var wg sync.WaitGroup

	for i := 0; i < cfg.concurrency; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			statsCh <- worker(ctx, cfg, client, headers, payload, collect)
		}()
	}

	wg.Wait()
	close(statsCh)

	out := benchResult{
		name:         cfg.name,
		url:          cfg.url,
		method:       cfg.method,
		duration:     phaseDuration,
		concurrency:  cfg.concurrency,
		statusCounts: make(map[int]uint64),
	}

	for st := range statsCh {
		out.ops += st.ops
		out.responses += st.responses
		out.success += st.success
		out.transportErr += st.transportErr
		out.responseReadErr += st.responseReadErr
		out.bytesRead += st.bytesRead
		for code, count := range st.statusCounts {
			out.statusCounts[code] += count
		}
		if collect {
			out.latenciesNs = append(out.latenciesNs, st.latenciesNs...)
		}
	}

	return out
}

func worker(ctx context.Context, cfg benchConfig, client *http.Client,
	headers http.Header, payload []byte, collectLatency bool) workerStats {
	stats := workerStats{
		statusCounts: make(map[int]uint64),
	}

	for {
		select {
		case <-ctx.Done():
			return stats
		default:
		}

		var body io.Reader
		if len(payload) > 0 {
			body = bytes.NewReader(payload)
		}

		req, err := http.NewRequestWithContext(ctx, cfg.method, cfg.url, body)
		if err != nil {
			stats.transportErr++
			stats.ops++
			continue
		}
		req.Header = headers.Clone()

		start := time.Now()
		resp, err := client.Do(req)
		latency := time.Since(start)
		if err != nil {
			if ctx.Err() != nil {
				return stats
			}
			stats.transportErr++
			stats.ops++
			continue
		}

		stats.responses++
		stats.statusCounts[resp.StatusCode]++
		if resp.StatusCode >= 200 && resp.StatusCode < 400 {
			stats.success++
		}

		n, readErr := io.Copy(io.Discard, resp.Body)
		_ = resp.Body.Close()
		if readErr != nil {
			stats.responseReadErr++
		}
		if n > 0 {
			stats.bytesRead += uint64(n)
		}

		stats.ops++
		if collectLatency {
			stats.latenciesNs = append(stats.latenciesNs, latency.Nanoseconds())
		}
	}
}

func buildClient(timeout time.Duration, insecureTLS bool, concurrency int) *http.Client {
	transport := &http.Transport{
		Proxy: http.ProxyFromEnvironment,
		DialContext: (&net.Dialer{
			Timeout:   timeout,
			KeepAlive: 30 * time.Second,
		}).DialContext,
		ForceAttemptHTTP2:     true,
		MaxIdleConns:          concurrency * 4,
		MaxIdleConnsPerHost:   concurrency * 4,
		IdleConnTimeout:       90 * time.Second,
		TLSHandshakeTimeout:   timeout,
		ExpectContinueTimeout: 1 * time.Second,
	}
	if insecureTLS {
		transport.TLSClientConfig = &tls.Config{InsecureSkipVerify: true}
	}

	return &http.Client{
		Transport: transport,
		Timeout:   timeout,
	}
}

func parseHeaders(raw []string) http.Header {
	headers := make(http.Header)
	for _, entry := range raw {
		parts := strings.SplitN(entry, ":", 2)
		if len(parts) != 2 {
			continue
		}
		k := strings.TrimSpace(parts[0])
		v := strings.TrimSpace(parts[1])
		if k != "" {
			headers.Add(k, v)
		}
	}
	return headers
}

func printResult(r benchResult) {
	fmt.Printf("\n=== %s ===\n", r.name)
	fmt.Printf("URL: %s\n", r.url)
	fmt.Printf("Method: %s\n", r.method)
	fmt.Printf("Duration: %s\n", r.duration)
	fmt.Printf("Concurrency: %d\n", r.concurrency)
	fmt.Printf("Operations: %d\n", r.ops)
	fmt.Printf("Responses: %d\n", r.responses)
	fmt.Printf("Transport errors: %d\n", r.transportErr)
	fmt.Printf("Response read errors: %d\n", r.responseReadErr)
	fmt.Printf("Bytes read: %d\n", r.bytesRead)

	rps := float64(r.ops) / r.duration.Seconds()
	fmt.Printf("Req/sec: %.2f\n", rps)

	successRate := 0.0
	if r.responses > 0 {
		successRate = float64(r.success) * 100 / float64(r.responses)
	}
	fmt.Printf("HTTP success rate (2xx/3xx): %.2f%%\n", successRate)

	printStatusCounts(r.statusCounts)
	printLatencyStats(r.latenciesNs)
}

func printStatusCounts(counts map[int]uint64) {
	if len(counts) == 0 {
		fmt.Println("Status codes: none")
		return
	}

	codes := make([]int, 0, len(counts))
	for code := range counts {
		codes = append(codes, code)
	}
	sort.Ints(codes)

	var parts []string
	for _, code := range codes {
		parts = append(parts, fmt.Sprintf("%d=%d", code, counts[code]))
	}
	fmt.Printf("Status codes: %s\n", strings.Join(parts, ", "))
}

func printLatencyStats(latenciesNs []int64) {
	if len(latenciesNs) == 0 {
		fmt.Println("Latency: no samples")
		return
	}
	lat := make([]int64, len(latenciesNs))
	copy(lat, latenciesNs)
	sort.Slice(lat, func(i, j int) bool { return lat[i] < lat[j] })
	stats := calcLatencyStats(lat)

	fmt.Printf("Latency avg/p50/p90/p95/p99/max: %s / %s / %s / %s / %s / %s\n",
		stats.avg, stats.p50, stats.p90, stats.p95, stats.p99, stats.max)
}

func calcLatencyStats(sortedNs []int64) latencyStats {
	var sum int64
	for _, v := range sortedNs {
		sum += v
	}

	avg := time.Duration(sum / int64(len(sortedNs)))
	return latencyStats{
		avg: avg,
		p50: percentile(sortedNs, 50),
		p90: percentile(sortedNs, 90),
		p95: percentile(sortedNs, 95),
		p99: percentile(sortedNs, 99),
		max: time.Duration(sortedNs[len(sortedNs)-1]),
	}
}

func percentile(sortedNs []int64, p float64) time.Duration {
	if len(sortedNs) == 0 {
		return 0
	}
	index := int(math.Ceil((p/100.0)*float64(len(sortedNs)))) - 1
	if index < 0 {
		index = 0
	}
	if index >= len(sortedNs) {
		index = len(sortedNs) - 1
	}
	return time.Duration(sortedNs[index])
}

func printComparison(a, b benchResult) {
	fmt.Println("\n=== Comparison ===")
	fmt.Printf("%-24s %-18s %-18s %-12s\n", "Metric", a.name, b.name, "Better")

	aRPS := float64(a.ops) / a.duration.Seconds()
	bRPS := float64(b.ops) / b.duration.Seconds()
	printCmpFloat("Req/sec", aRPS, bRPS, true, a.name, b.name)

	aSuccessRate := percentage(a.success, a.responses)
	bSuccessRate := percentage(b.success, b.responses)
	printCmpFloat("Success rate %", aSuccessRate, bSuccessRate, true, a.name, b.name)

	aErr := a.transportErr + a.responseReadErr
	bErr := b.transportErr + b.responseReadErr
	printCmpInt("Transport+read errs", int64(aErr), int64(bErr), false, a.name, b.name)

	aLat := calcLatencyFromResult(a)
	bLat := calcLatencyFromResult(b)
	printCmpDuration("p95 latency", aLat.p95, bLat.p95, false, a.name, b.name)
	printCmpDuration("p99 latency", aLat.p99, bLat.p99, false, a.name, b.name)
}

func calcLatencyFromResult(r benchResult) latencyStats {
	if len(r.latenciesNs) == 0 {
		return latencyStats{}
	}
	lat := make([]int64, len(r.latenciesNs))
	copy(lat, r.latenciesNs)
	sort.Slice(lat, func(i, j int) bool { return lat[i] < lat[j] })
	return calcLatencyStats(lat)
}

func percentage(a, b uint64) float64 {
	if b == 0 {
		return 0
	}
	return float64(a) * 100 / float64(b)
}

func printCmpFloat(metric string, a, b float64, higherBetter bool, aName, bName string) {
	better := pickBetterFloat(a, b, higherBetter, aName, bName)
	fmt.Printf("%-24s %-18s %-18s %-12s\n",
		metric, fmt.Sprintf("%.2f", a), fmt.Sprintf("%.2f", b), better)
}

func printCmpInt(metric string, a, b int64, higherBetter bool, aName, bName string) {
	better := pickBetterInt(a, b, higherBetter, aName, bName)
	fmt.Printf("%-24s %-18d %-18d %-12s\n", metric, a, b, better)
}

func printCmpDuration(metric string, a, b time.Duration, higherBetter bool, aName, bName string) {
	aNs := int64(a)
	bNs := int64(b)
	better := pickBetterInt(aNs, bNs, higherBetter, aName, bName)
	fmt.Printf("%-24s %-18s %-18s %-12s\n", metric, a.String(), b.String(), better)
}

func pickBetterFloat(a, b float64, higherBetter bool, aName, bName string) string {
	if a == b {
		return "tie"
	}
	if higherBetter {
		if a > b {
			return aName
		}
		return bName
	}
	if a < b {
		return aName
	}
	return bName
}

func pickBetterInt(a, b int64, higherBetter bool, aName, bName string) string {
	if a == b {
		return "tie"
	}
	if higherBetter {
		if a > b {
			return aName
		}
		return bName
	}
	if a < b {
		return aName
	}
	return bName
}

func init() {
	flag.Usage = func() {
		out := flag.CommandLine.Output()
		fmt.Fprintf(out, "Usage:\n")
		fmt.Fprintf(out, "  go run stress/http_stress.go -url http://127.0.0.1:8080/\n\n")
		fmt.Fprintf(out, "Compare mode:\n")
		fmt.Fprintf(out, "  go run stress/http_stress.go -url http://127.0.0.1:8080/ -name webserv ")
		fmt.Fprintf(out, "-compare-url http://127.0.0.1:3000/ -compare-name node\n\n")
		fmt.Fprintf(out, "Flags:\n")
		flag.PrintDefaults()
	}
}
