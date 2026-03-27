#include "tab_controller.hpp"

#include <iostream>

namespace mt {

TabController::TabController(Supervisor& supervisor) : supervisor_(supervisor) {}

std::string TabController::add_for_instance(const PluginInstance& instance) {
  const std::string tab_id = "tab-" + instance.instance_id;
  TabViewModel model{
      tab_id,
      instance.display_name,
      instance.instance_id,
      instance.status,
  };

  if (instance.plugin_id == "firefox.gecko") {
    model.show_address_bar = true;
    model.address_value = "about:blank";
  }

  tabs_[tab_id] = model;
  std::cout << "[gui] add tab title='" << instance.display_name << "' instance=" << instance.instance_id
            << " address_bar=" << (model.show_address_bar ? "on" : "off") << '\n';
  return tab_id;
}

bool TabController::render_tab_content(const std::string& tab_id) {
  auto it = tabs_.find(tab_id);
  if (it == tabs_.end()) return false;

  auto& tab = it->second;
  if (!tab.plugin_owns_tab_rendering) {
    std::cout << "[gui] host-owned render tab=" << tab_id << '\n';
    return true;
  }

  std::string render_json;
  const bool ok = supervisor_.query_tab_view(tab.instance_id, render_json);
  if (!ok) {
    std::cerr << "[gui] render query failed tab=" << tab_id << '\n';
    return false;
  }

  tab.last_render_json = render_json;
  tab.renders_html = render_json.find("\"type\":\"html\"") != std::string::npos;
  tab.supports_js = render_json.find("\"scripts\"") != std::string::npos;
  if (tab.supports_js) {
    tab.js_bundle = "plugin-inline-js";
  }
  std::cout << "[gui] plugin-render tab=" << tab_id << " json=" << render_json
            << " html=" << (tab.renders_html ? "yes" : "no")
            << " js=" << (tab.supports_js ? "yes" : "no") << '\n';
  return true;
}

bool TabController::on_address_submitted(const std::string& tab_id, const std::string& url) {
  auto it = tabs_.find(tab_id);
  if (it == tabs_.end()) return false;

  auto& tab = it->second;
  if (!tab.show_address_bar) {
    std::cerr << "[gui] tab has no address bar: " << tab_id << '\n';
    return false;
  }

  tab.address_value = url;
  const std::string payload = "{action=\"navigate\",url=\"" + url + "\"}";
  const bool ok = supervisor_.send_command(tab.instance_id, payload);
  if (!ok) {
    std::cerr << "[gui] navigate failed tab=" << tab_id << " url=" << url << '\n';
    return false;
  }

  std::cout << "[gui] navigate tab=" << tab_id << " -> " << url << '\n';
  return true;
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
