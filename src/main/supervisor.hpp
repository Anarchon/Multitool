#pragma once

#include "plugin_registry.hpp"

#include <chrono>
#include <string>
#include <unordered_map>

namespace mt {

struct HostState {
  PluginManifest manifest;
  int restart_count{0};
  bool running{false};
  std::chrono::steady_clock::time_point last_start{};
};

class Supervisor {
 public:
  void start_host(const PluginManifest& plugin);
  void stop_host(const std::string& plugin_id);
  void on_host_crash(const std::string& plugin_id, const std::string& reason);
  void tick();

 private:
  std::unordered_map<std::string, HostState> hosts_;
};

} // namespace mt
