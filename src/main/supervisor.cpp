#include "supervisor.hpp"

#include <iostream>

namespace mt {

void Supervisor::start_host(const PluginManifest& plugin) {
  auto& state = hosts_[plugin.id];
  state.manifest = plugin;
  state.running = true;
  state.last_start = std::chrono::steady_clock::now();

  std::cout << "[supervisor] host started for " << plugin.id
            << " dll=" << plugin.entry_library << '\n';

  // Real implementation: spawn plugin_host process with dedicated pipes.
}

void Supervisor::stop_host(const std::string& plugin_id) {
  auto it = hosts_.find(plugin_id);
  if (it == hosts_.end()) {
    return;
  }
  it->second.running = false;
  std::cout << "[supervisor] host stopped for " << plugin_id << '\n';
}

void Supervisor::on_host_crash(const std::string& plugin_id, const std::string& reason) {
  auto it = hosts_.find(plugin_id);
  if (it == hosts_.end()) {
    return;
  }

  auto& state = it->second;
  state.running = false;
  std::cerr << "[supervisor] host crash plugin=" << plugin_id << " reason=" << reason << '\n';

  if (!state.manifest.auto_restart || state.restart_count >= state.manifest.max_restart_attempts) {
    std::cerr << "[supervisor] restart skipped for " << plugin_id << '\n';
    return;
  }

  ++state.restart_count;
  start_host(state.manifest);
}

void Supervisor::tick() {
  // Real implementation: poll child-process table and heartbeat deadlines.
}

} // namespace mt
