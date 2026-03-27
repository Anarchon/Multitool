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
  bool show_address_bar{false};
  std::string address_value{"about:blank"};
  bool plugin_owns_tab_rendering{true};
  std::string last_render_json{"{}"};
  bool renders_html{false};
  bool supports_js{false};
  std::string js_bundle{""};
};

class TabController {
 public:
  explicit TabController(Supervisor& supervisor);

  std::string add_for_instance(const PluginInstance& instance);
  bool on_close_clicked(const std::string& tab_id);
  bool on_address_submitted(const std::string& tab_id, const std::string& url);
  bool render_tab_content(const std::string& tab_id);
  void sync_status(const std::string& instance_id, InstanceStatus status);

 private:
  Supervisor& supervisor_;
  std::unordered_map<std::string, TabViewModel> tabs_;
};

} // namespace mt
