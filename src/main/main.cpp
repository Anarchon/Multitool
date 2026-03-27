#include "plugin_registry.hpp"
#include "supervisor.hpp"
#include "tab_controller.hpp"

#include <iostream>
#include <unordered_map>

int main() {
  mt::PluginRegistry registry{"plugins"};
  const auto plugins = registry.discover();

  std::unordered_map<std::string, mt::PluginDefinition> by_id;
  for (const auto& p : plugins) by_id[p.plugin_id] = p;

  mt::Supervisor supervisor{"plugin_host"};
  mt::TabController tabs{supervisor};

  // Example: two instances of same plugin type, each in own host process.
  if (by_id.contains("firefox.gecko")) {
    const auto i1 = supervisor.start_instance(by_id.at("firefox.gecko"));
    const auto i2 = supervisor.start_instance(by_id.at("firefox.gecko"));
    tabs.add_for_instance(*supervisor.get(i1));
    tabs.add_for_instance(*supervisor.get(i2));
  }

  if (by_id.contains("chromium.cef")) {
    const auto i3 = supervisor.start_instance(by_id.at("chromium.cef"));
    tabs.add_for_instance(*supervisor.get(i3));
  }

  std::cout << "[main] simulated run loop\n";

  // Simulate "close tab X" => exactly one instance is stopped.
  tabs.on_close_clicked("tab-firefox.gecko#1");

  // Simulate crash of one instance and per-instance restart.
  supervisor.on_host_exit("chromium.cef#1", -1073741819);

  return 0;
}
