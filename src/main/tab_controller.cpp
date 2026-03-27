#include "tab_controller.hpp"

#include <iostream>

namespace mt {

TabController::TabController(Supervisor& supervisor) : supervisor_(supervisor) {}

std::string TabController::add_for_instance(const PluginInstance& instance) {
  const std::string tab_id = "tab-" + instance.instance_id;
  tabs_[tab_id] = TabViewModel{
      tab_id,
      instance.display_name,
      instance.instance_id,
      instance.status,
  };
  std::cout << "[gui] add tab title='" << instance.display_name << "' instance=" << instance.instance_id << '\n';
  return tab_id;
}

bool TabController::on_close_clicked(const std::string& tab_id) {
  auto it = tabs_.find(tab_id);
  if (it == tabs_.end()) return false;

  const auto instance_id = it->second.instance_id;
  std::cout << "[gui] X clicked tab=" << tab_id << " instance=" << instance_id << '\n';

  const bool stopped = supervisor_.stop_instance(instance_id, 1500);
  if (!stopped) {
    std::cerr << "[gui] stop failed instance=" << instance_id << '\n';
    return false;
  }

  tabs_.erase(it);
  std::cout << "[gui] tab removed=" << tab_id << '\n';
  return true;
}

void TabController::sync_status(const std::string& instance_id, InstanceStatus status) {
  for (auto& [_, tab] : tabs_) {
    if (tab.instance_id == instance_id) {
      tab.status = status;
      std::cout << "[gui] status update instance=" << instance_id << '\n';
      return;
    }
  }
}

} // namespace mt
