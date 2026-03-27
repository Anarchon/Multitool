#pragma once

#include "shared/plugin_api.hpp"

#include <filesystem>
#include <string>

namespace mt {

class DllLoader {
 public:
  bool load(const std::filesystem::path& path, std::string& error);
  void unload();

  const PluginInfo* info() const { return info_; }
  const PluginVTable* vtable() const { return vtable_; }

 private:
  void* handle_{nullptr};
  const PluginInfo* info_{nullptr};
  const PluginVTable* vtable_{nullptr};
};

} // namespace mt
