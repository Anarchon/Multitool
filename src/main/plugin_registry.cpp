#include "plugin_registry.hpp"

#include <fstream>
#include <sstream>

namespace mt {

PluginRegistry::PluginRegistry(std::filesystem::path root) : root_(std::move(root)) {}

std::vector<PluginManifest> PluginRegistry::discover() const {
  std::vector<PluginManifest> result;
  if (!std::filesystem::exists(root_)) {
    return result;
  }

  for (const auto& entry : std::filesystem::directory_iterator(root_)) {
    if (!entry.is_directory()) {
      continue;
    }

    const auto manifest_path = entry.path() / "plugin.json";
    if (!std::filesystem::exists(manifest_path)) {
      continue;
    }

    // Minimal parser for the skeleton; production code should use a JSON lib.
    std::ifstream in(manifest_path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto text = buffer.str();

    PluginManifest m;
    m.id = entry.path().filename().string();
    m.name = m.id;
    m.version = "0.1.0";
    m.entry_library = (entry.path() / "bin" / "plugin.dll").string();

    if (text.find("\"auto_restart\": false") != std::string::npos) {
      m.auto_restart = false;
    }
    result.push_back(std::move(m));
  }

  return result;
}

} // namespace mt
