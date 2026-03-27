#pragma once

#include <cstdint>

#if defined(_WIN32)
  #if defined(BUILDING_PLUGIN_DLL)
    #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
  #else
    #define PLUGIN_EXPORT extern "C" __declspec(dllimport)
  #endif
#else
  #define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace mt {

constexpr std::uint32_t kPluginAbiVersion = 2;

struct InstanceContext {
  const char* plugin_id;
  const char* instance_id;
  const char* config_path;
  const char* data_dir;
};

struct PluginHostCallbacks {
  void (*log)(int level, const char* instance_id, const char* message) noexcept;
  void (*emit_event)(const char* instance_id, const char* json_event) noexcept;
};

struct PluginInfo {
  std::uint32_t abi_version;
  const char* plugin_id;
  const char* name;
  const char* version;
  const char* capabilities_csv;
};

struct PluginVTable {
  bool (*initialize)(const InstanceContext* context, const PluginHostCallbacks* callbacks) noexcept;
  bool (*start)() noexcept;
  bool (*request_stop)(int timeout_ms) noexcept;
  bool (*command)(const char* command_json, char* response_buf, std::uint32_t response_buf_size) noexcept;
  bool (*health_check)(char* response_buf, std::uint32_t response_buf_size) noexcept;
  void (*shutdown)() noexcept;
};

using GetPluginInfoFn = const PluginInfo* (*)() noexcept;
using GetPluginVTableFn = const PluginVTable* (*)() noexcept;

} // namespace mt
