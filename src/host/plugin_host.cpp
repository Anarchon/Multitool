#include "dll_loader.hpp"
#include "shared/ipc_contract.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {

std::string g_instance_id;

void host_log(int level, const char* instance_id, const char* message) noexcept {
  std::cout << mt::ipc::make_frame({
      {"event", "log"},
      {"instance_id", instance_id ? instance_id : ""},
      {"level", std::to_string(level)},
      {"message", message ? message : ""},
  }) << std::endl;
}

void host_emit_event(const char* instance_id, const char* json_event) noexcept {
  std::cout << mt::ipc::make_frame({
      {"event", "plugin"},
      {"instance_id", instance_id ? instance_id : ""},
      {"payload", json_event ? json_event : "{}"},
  }) << std::endl;
}

std::string arg_value(int argc, char** argv, const std::string& key, const std::string& fallback = "") {
  const auto prefix = key + "=";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind(prefix, 0) == 0) return a.substr(prefix.size());
  }
  return fallback;
}

} // namespace

int main(int argc, char** argv) {
  const auto plugin_path = arg_value(argc, argv, "--plugin-path");
  g_instance_id = arg_value(argc, argv, "--instance-id", "unknown-instance");
  const auto plugin_id = arg_value(argc, argv, "--plugin-id", "unknown-plugin");

  if (plugin_path.empty()) {
    std::cerr << "missing --plugin-path\n";
    return 2;
  }

  mt::DllLoader loader;
  std::string error;
  if (!loader.load(plugin_path, error)) {
    std::cout << mt::ipc::make_frame({{"event", "loaded"}, {"ok", "false"}, {"error", error}}) << std::endl;
    return 1;
  }

  const mt::PluginHostCallbacks callbacks{&host_log, &host_emit_event};
  const mt::InstanceContext context{plugin_id.c_str(), g_instance_id.c_str(), "", ""};
  if (!loader.vtable()->initialize(&context, &callbacks)) {
    std::cout << mt::ipc::make_frame({{"event", "loaded"}, {"ok", "false"}, {"error", "initialize failed"}})
              << std::endl;
    return 1;
  }

  std::cout << mt::ipc::make_frame({
      {"event", "loaded"},
      {"ok", "true"},
      {"instance_id", g_instance_id},
      {"plugin_id", loader.info()->plugin_id},
      {"version", loader.info()->version},
  }) << std::endl;

  std::string line;
  char response[4096]{};
  while (std::getline(std::cin, line)) {
    auto f = mt::ipc::parse_frame(line);
    auto cmd = mt::ipc::get(f, "cmd");
    if (!cmd.has_value()) continue;

    if (cmd.value() == "start") {
      const bool ok = loader.vtable()->start();
      std::cout << mt::ipc::make_frame({{"event", "response"}, {"ok", ok ? "true" : "false"}}) << std::endl;
    } else if (cmd.value() == "status") {
      std::memset(response, 0, sizeof(response));
      loader.vtable()->health_check(response, sizeof(response));
      std::cout << mt::ipc::make_frame({{"event", "status"}, {"instance_id", g_instance_id}, {"payload", response}})
                << std::endl;
    } else if (cmd.value() == "command") {
      std::memset(response, 0, sizeof(response));
      const auto payload = mt::ipc::get(f, "payload").value_or("{}");
      const bool ok = loader.vtable()->command(payload.c_str(), response, sizeof(response));
      std::cout << mt::ipc::make_frame({
          {"event", "response"},
          {"ok", ok ? "true" : "false"},
          {"payload", ok ? response : "{}"},
      }) << std::endl;
    } else if (cmd.value() == "shutdown") {
      loader.vtable()->request_stop(1500);
      break;
    }
  }

  loader.vtable()->shutdown();
  loader.unload();
  return 0;
}
