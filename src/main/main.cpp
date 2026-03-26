#include "plugin_registry.hpp"
#include "supervisor.hpp"

#include <iostream>

int main() {
  mt::PluginRegistry registry{"plugins"};
  auto manifests = registry.discover();

  mt::Supervisor supervisor;
  for (const auto& manifest : manifests) {
    supervisor.start_host(manifest);
  }

  std::cout << "multitool main is running; discovered " << manifests.size() << " plugins\n";

  // Demo crash isolation: only restart a single plugin host.
  if (!manifests.empty()) {
    supervisor.on_host_crash(manifests.front().id, "simulated crash");
  }

  return 0;
}
