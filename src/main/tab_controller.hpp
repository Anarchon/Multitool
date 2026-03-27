#pragma once

#include "supervisor.hpp"

#include <string>
#include <unordered_map>

namespace mt {

struct TabViewModel {
  std::string tab_id;
  std::string title;
  std::string instance_id;
  InstanceStatus status{InstanceStatus::Starting};
};

class TabController {
 public:
  explicit TabController(Supervisor& supervisor);

  std::string add_for_instance(const PluginInstance& instance);
  bool on_close_clicked(const std::string& tab_id);
  void sync_status(const std::string& instance_id, InstanceStatus status);

 private:
  Supervisor& supervisor_;
  std::unordered_map<std::string, TabViewModel> tabs_;
};

} // namespace mt
