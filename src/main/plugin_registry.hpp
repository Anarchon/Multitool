#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace mt {

struct PluginDefinition {
  std::string plugin_id;
  std::string name;
  std::string version;
  std::string entry_library;
  bool auto_restart{true};
  int max_restart_attempts{3};
};

class PluginRegistry {
 public:
  explicit PluginRegistry(std::filesystem::path root);
  std::vector<PluginDefinition> discover() const;

 private:
  std::filesystem::path root_;
};

} // namespace mt
