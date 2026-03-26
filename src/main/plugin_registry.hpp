#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace mt {

struct PluginManifest {
  std::string id;
  std::string name;
  std::string version;
  std::string entry_library;
  bool auto_restart{true};
  int max_restart_attempts{3};
};

class PluginRegistry {
 public:
  explicit PluginRegistry(std::filesystem::path root);
  std::vector<PluginManifest> discover() const;

 private:
  std::filesystem::path root_;
};

} // namespace mt
