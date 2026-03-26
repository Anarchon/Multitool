#include "dll_loader.hpp"

#include <cstring>
#include <iostream>
#include <string>

namespace {

void host_log(int level, const char* message) noexcept {
  std::cerr << "{\"type\":\"event\",\"event\":{\"topic\":\"log\",\"level\":"
            << level << ",\"message\":\"" << (message ? message : "") << "\"}}" << std::endl;
}

void host_emit_event(const char* json_event) noexcept {
  std::cout << "{\"type\":\"event\",\"event\":" << (json_event ? json_event : "{}") << "}" << std::endl;
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: plugin_host <plugin-library>\n";
    return 2;
  }

  mt::DllLoader loader;
  std::string error;
  if (!loader.load(argv[1], error)) {
    std::cerr << "{\"type\":\"loaded\",\"ok\":false,\"error\":\"" << error << "\"}" << std::endl;
    return 1;
  }

  const mt::PluginHostCallbacks callbacks{&host_log, &host_emit_event};
  if (!loader.vtable()->init(&callbacks, "{}")) {
    std::cerr << "{\"type\":\"loaded\",\"ok\":false,\"error\":\"init failed\"}" << std::endl;
    return 1;
  }

  std::cout << "{\"type\":\"loaded\",\"ok\":true,\"plugin\":{\"id\":\""
            << loader.info()->id << "\",\"version\":\"" << loader.info()->version << "\"}}" << std::endl;

  std::string line;
  char response[4096]{};
  while (std::getline(std::cin, line)) {
    if (line.find("\"type\":\"start\"") != std::string::npos) {
      const bool ok = loader.vtable()->start();
      std::cout << "{\"type\":\"response\",\"ok\":" << (ok ? "true" : "false") << "}" << std::endl;
    } else if (line.find("\"type\":\"stop\"") != std::string::npos) {
      const bool ok = loader.vtable()->stop();
      std::cout << "{\"type\":\"response\",\"ok\":" << (ok ? "true" : "false") << "}" << std::endl;
    } else if (line.find("\"type\":\"command\"") != std::string::npos) {
      std::memset(response, 0, sizeof(response));
      const bool ok = loader.vtable()->handle_command(line.c_str(), response, sizeof(response));
      std::cout << "{\"type\":\"response\",\"ok\":" << (ok ? "true" : "false")
                << ",\"payload\":" << (ok ? response : "{}") << "}" << std::endl;
    } else if (line.find("\"type\":\"shutdown\"") != std::string::npos) {
      break;
    }
  }

  loader.vtable()->shutdown();
  loader.unload();
  return 0;
}
