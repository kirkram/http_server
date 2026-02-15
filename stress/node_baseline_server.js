const http = require("http");
const fs = require("fs");
const path = require("path");
const { URL } = require("url");

const port = Number(process.env.PORT || 3000);
const host = process.env.HOST || "0.0.0.0";
const rootDir = __dirname;

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".txt": "text/plain; charset=utf-8",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".gif": "image/gif",
  ".ico": "image/x-icon",
  ".svg": "image/svg+xml",
  ".mp4": "video/mp4",
  ".mov": "video/quicktime",
  ".webm": "video/webm"
};

function safeResolvePath(urlPathname) {
  let decodedPath = "/";
  try {
    decodedPath = decodeURIComponent(urlPathname);
  } catch {
    return null;
  }
  if (decodedPath === "/") {
    decodedPath = "/indexNode.html";
  }
  const absolutePath = path.resolve(rootDir, "." + decodedPath);
  if (!absolutePath.startsWith(rootDir + path.sep) && absolutePath !== rootDir) {
    return null;
  }
  return absolutePath;
}

function sendStaticFile(req, res, absolutePath) {
  fs.stat(absolutePath, (statErr, stat) => {
    if (statErr || !stat.isFile()) {
      res.writeHead(404, { "Content-Type": "text/plain; charset=utf-8" });
      res.end("not found");
      return;
    }

    const ext = path.extname(absolutePath).toLowerCase();
    const contentType = mimeTypes[ext] || "application/octet-stream";
    res.writeHead(200, {
      "Content-Type": contentType,
      "Content-Length": stat.size
    });
    if (req.method === "HEAD") {
      res.end();
      return;
    }
    fs.createReadStream(absolutePath).pipe(res);
  });
}

const server = http.createServer((req, res) => {
  const parsed = new URL(req.url, `http://${req.headers.host || "localhost"}`);
  if (req.method === "GET" && parsed.pathname === "/json") {
    const body = JSON.stringify({ ok: true, server: "node", ts: Date.now() });
    res.writeHead(200, {
      "Content-Type": "application/json",
      "Content-Length": Buffer.byteLength(body)
    });
    res.end(body);
    return;
  }

  if (req.method === "POST" && (parsed.pathname === "/echo" || parsed.pathname === "/upload")) {
    let size = 0;
    req.on("data", (chunk) => {
      size += chunk.length;
    });
    req.on("end", () => {
      const body = JSON.stringify({ ok: true, bytes: size });
      res.writeHead(200, {
        "Content-Type": "application/json",
        "Content-Length": Buffer.byteLength(body)
      });
      res.end(body);
    });
    return;
  }

  if (req.method === "GET" || req.method === "HEAD") {
    const filePath = safeResolvePath(parsed.pathname);
    if (!filePath) {
      res.writeHead(400, { "Content-Type": "text/plain; charset=utf-8" });
      res.end("bad request");
      return;
    }
    sendStaticFile(req, res, filePath);
    return;
  }

  res.writeHead(404, { "Content-Type": "text/plain; charset=utf-8" });
  res.end("not found");
});

server.listen(port, host, () => {
  console.log(`Node baseline server listening on http://${host}:${port}`);
});
