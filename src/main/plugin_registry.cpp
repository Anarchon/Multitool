#include "plugin_registry.hpp"

#include <fstream>
#include <sstream>

namespace mt {

namespace {

std::string read_text(const std::filesystem::path& file) {
  std::ifstream in(file);
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string find_value(const std::string& text, const std::string& key, const std::string& fallback) {
  const auto token = "\"" + key + "\"";
  const auto k = text.find(token);
  if (k == std::string::npos) return fallback;
  const auto q1 = text.find('"', text.find(':', k));
  const auto q2 = text.find('"', q1 + 1);
  if (q1 == std::string::npos || q2 == std::string::npos || q2 <= q1) return fallback;
  return text.substr(q1 + 1, q2 - q1 - 1);
}

} // namespace

PluginRegistry::PluginRegistry(std::filesystem::path root) : root_(std::move(root)) {}

std::vector<PluginDefinition> PluginRegistry::discover() const {
  std::vector<PluginDefinition> result;
  if (!std::filesystem::exists(root_)) return result;

  for (const auto& entry : std::filesystem::directory_iterator(root_)) {
    if (!entry.is_directory()) continue;

    const auto manifest = entry.path() / "plugin.json";
    if (!std::filesystem::exists(manifest)) continue;

    const auto text = read_text(manifest);
    PluginDefinition d;
    d.plugin_id = find_value(text, "id", entry.path().filename().string());
    d.name = find_value(text, "name", d.plugin_id);
    d.version = find_value(text, "version", "0.1.0");
    const auto rel = find_value(text, "entry_library", "bin/plugin.dll");
    d.entry_library = (entry.path() / rel).string();
    if (text.find("\"enabled\": false") != std::string::npos) {
      d.auto_restart = false;
    }
    result.push_back(std::move(d));
  }
  return result;
}

} // namespace mt
