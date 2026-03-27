#include "supervisor.hpp"

#include <cstdlib>
#include <iostream>

namespace mt {

namespace {

const char* to_text(InstanceStatus s) {
  switch (s) {
    case InstanceStatus::Starting: return "Starting";
    case InstanceStatus::Running: return "Running";
    case InstanceStatus::Stopping: return "Stopping";
    case InstanceStatus::Stopped: return "Stopped";
    case InstanceStatus::Crashed: return "Crashed";
  }
  return "Unknown";
}

} // namespace

Supervisor::Supervisor(std::string host_executable) : host_executable_(std::move(host_executable)) {}

std::string Supervisor::next_instance_id(const std::string& plugin_id) {
  const int id = ++counters_[plugin_id];
  return plugin_id + "#" + std::to_string(id);
}

void Supervisor::spawn_host(PluginInstance& instance) {
  instance.status = InstanceStatus::Starting;
  instance.last_heartbeat = std::chrono::steady_clock::now();

  // Production: create process + dedicated stdin/stdout IPC pipes.
  // Here we only show command line shape for deterministic orchestration.
  std::cout << "[supervisor] spawn " << host_executable_
            << " --instance-id=" << instance.instance_id
            << " --plugin-path=" << instance.plugin_library << '\n';

  instance.os_pid = static_cast<std::uint64_t>(std::rand());
  instance.status = InstanceStatus::Running;
}

std::string Supervisor::start_instance(const PluginDefinition& definition) {
  PluginInstance instance;
  instance.instance_id = next_instance_id(definition.plugin_id);
  instance.plugin_id = definition.plugin_id;
  instance.display_name = definition.name + " " + instance.instance_id;
  instance.plugin_library = definition.entry_library;
  instance.restart_enabled = definition.auto_restart;
  instance.max_restart_attempts = definition.max_restart_attempts;

  spawn_host(instance);
  const auto id = instance.instance_id;
  instances_.emplace(id, std::move(instance));
  return id;
}

bool Supervisor::stop_instance(const std::string& instance_id, int graceful_timeout_ms) {
  auto it = instances_.find(instance_id);
  if (it == instances_.end()) return false;

  auto& instance = it->second;
  instance.status = InstanceStatus::Stopping;
  std::cout << "[supervisor] stop request -> " << instance.instance_id
            << " timeout=" << graceful_timeout_ms << "ms\n";

  // Production: send IPC command `cmd=shutdown` and wait for ACK/process exit.
  instance.status = InstanceStatus::Stopped;
  return true;
}

void Supervisor::force_kill(const PluginInstance& instance) {
  std::cerr << "[supervisor] force kill pid=" << instance.os_pid
            << " instance=" << instance.instance_id << '\n';
}

void Supervisor::on_host_exit(const std::string& instance_id, int exit_code) {
  auto it = instances_.find(instance_id);
  if (it == instances_.end()) return;

  auto& instance = it->second;
  if (instance.status == InstanceStatus::Stopping || instance.status == InstanceStatus::Stopped) {
    instance.status = InstanceStatus::Stopped;
    return;
  }

  instance.status = InstanceStatus::Crashed;
  std::cerr << "[supervisor] crash instance=" << instance_id << " exit=" << exit_code << '\n';

  if (!instance.restart_enabled || instance.restart_count >= instance.max_restart_attempts) {
    std::cerr << "[supervisor] restart disabled for " << instance_id << '\n';
    return;
  }

  ++instance.restart_count;
  spawn_host(instance);
}

void Supervisor::on_heartbeat(const std::string& instance_id) {
  auto it = instances_.find(instance_id);
  if (it == instances_.end()) return;
  it->second.last_heartbeat = std::chrono::steady_clock::now();
}

void Supervisor::tick() {
  const auto now = std::chrono::steady_clock::now();
  for (auto& [_, instance] : instances_) {
    if (instance.status != InstanceStatus::Running) continue;
    const auto dt = std::chrono::duration_cast<std::chrono::seconds>(now - instance.last_heartbeat).count();
    if (dt > 5) {
      std::cerr << "[supervisor] heartbeat timeout instance=" << instance.instance_id
                << " state=" << to_text(instance.status) << '\n';
      force_kill(instance);
      on_host_exit(instance.instance_id, -1001);
    }
  }
}

std::optional<PluginInstance> Supervisor::get(const std::string& instance_id) const {
  auto it = instances_.find(instance_id);
  if (it == instances_.end()) return std::nullopt;
  return it->second;
}

} // namespace mt
