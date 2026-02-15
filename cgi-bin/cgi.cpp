#include <iostream>
#include <sstream>

std::string urlDecode(const std::string& str) {
  std::string result;
  for (std::size_t i = 0; i < str.length(); ++i) {
    if (str[i] == '+') {
      result += ' ';
    } else if (str[i] == '%' && i + 2 < str.length()) {
      std::string hex = str.substr(i + 1, 2);
      char ch = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
      result += ch;
      i += 2;
    } else {
      result += str[i];
    }
  }
  return result;
}

int main() {
  std::string line;
  std::cout << "Content-Type: text/html\r\n\r\n" << std::flush;
  std::cin >> line;
  std::size_t pos = line.find('=');
  if (pos != std::string::npos)
    line = line.substr(pos + 1);
  
  line = urlDecode(line);

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
         "    .btn { display:inline-block; margin-top:24px; padding:12px 28px; background:transparent; border:2px solid var(--accent); border-radius:8px; color:var(--accent); text-decoration:none; font-weight:600; transition:all .3s; }\n"
         "    .btn:hover { background:var(--accent); color:#000; box-shadow:0 0 20px rgba(98,255,205,.5); }\n"
         "  </style>\n"
         "</head>\n"
         "<body>\n"
         "  <main class=\"card\">\n"
         "    <h1>Hello <span class=\"name\">"
      << line
      << "</span>!</h1>\n"
         "    <a href=\"/\" class=\"btn\">Go Back</a>\n"
         "  </main>\n"
         "</body>\n"
         "</html>\n"
      << std::flush;
}
