#pragma once

#include "plugin_registry.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace mt {

enum class InstanceStatus { Starting, Running, Stopping, Stopped, Crashed };

struct PluginInstance {
  std::string instance_id;
  std::string plugin_id;
  std::string display_name;
  std::string host_exe{"plugin_host"};
  std::string plugin_library;
  std::uint64_t os_pid{0};
  int restart_count{0};
  bool restart_enabled{true};
  int max_restart_attempts{3};
  InstanceStatus status{InstanceStatus::Starting};
  std::chrono::steady_clock::time_point last_heartbeat{};
};

class Supervisor {
 public:
  explicit Supervisor(std::string host_executable);

  std::string start_instance(const PluginDefinition& definition);
  bool stop_instance(const std::string& instance_id, int graceful_timeout_ms);
  void on_host_exit(const std::string& instance_id, int exit_code);
  void on_heartbeat(const std::string& instance_id);
  void tick();

  std::optional<PluginInstance> get(const std::string& instance_id) const;

 private:
  std::string next_instance_id(const std::string& plugin_id);
  void spawn_host(PluginInstance& instance);
  void force_kill(const PluginInstance& instance);

  std::string host_executable_;
  std::unordered_map<std::string, int> counters_;
  std::unordered_map<std::string, PluginInstance> instances_;
};

} // namespace mt
