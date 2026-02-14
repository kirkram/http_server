#include <iostream>

int main() {
  std::string line;
  std::cout << "Content-Type: text/html\r\n\r\n" << std::flush;
  std::cin >> line;
  std::size_t pos = line.find('=');
  if (pos != std::string::npos)
    line = line.substr(pos + 1);

  std::cout
      << "<!doctype html>\n"
         "<html lang=\"en\">\n"
         "<head>\n"
         "  <meta charset=\"utf-8\">\n"
         "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
         "  <title>cgi message</title>\n"
         "  <style>\n"
         "    :root { --bg:#000000; --accent:#62ffcd; --card:rgba(8,18,16,.82); --border:rgba(98,255,205,.4); }\n"
         "    * { box-sizing:border-box; }\n"
         "    body { margin:0; min-height:100vh; display:grid; place-items:center; background:radial-gradient(circle at top,#102221 0%,#050505 45%,#000 100%); font-family:sans-serif; color:var(--accent); }\n"
         "    .card { width:min(92vw,700px); background:var(--card); border:1px solid var(--border); border-radius:18px; padding:34px 30px; box-shadow:0 0 25px rgba(98,255,205,.18), 0 20px 45px rgba(0,0,0,.55); text-align:center; backdrop-filter:blur(4px); }\n"
         "    h1 { margin:0; font-size:clamp(1.6rem,4vw,2.5rem); letter-spacing:.04em; }\n"
         "    .name { color:var(--accent); font-weight:800; text-shadow:0 0 12px rgba(98,255,205,.4); }\n"
         "  </style>\n"
         "</head>\n"
         "<body>\n"
         "  <main class=\"card\">\n"
         "    <h1>Hello <span class=\"name\">"
      << line
      << "</span>!</h1>\n"
         "  </main>\n"
         "</body>\n"
         "</html>\n"
      << std::flush;
}
