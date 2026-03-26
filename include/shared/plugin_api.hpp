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

constexpr std::uint32_t kPluginAbiVersion = 1;

struct PluginHostCallbacks {
  void (*log)(int level, const char* message) noexcept;
  void (*emit_event)(const char* json_event) noexcept;
};

struct PluginInfo {
  std::uint32_t abi_version;
  const char* id;
  const char* name;
  const char* version;
  const char* capabilities_csv;
};

struct PluginVTable {
  bool (*init)(const PluginHostCallbacks* callbacks, const char* init_json) noexcept;
  bool (*start)() noexcept;
  bool (*stop)() noexcept;
  bool (*handle_command)(const char* command_json, char* response_buf, std::uint32_t response_buf_size) noexcept;
  void (*shutdown)() noexcept;
};

using GetPluginInfoFn = const PluginInfo* (*)() noexcept;
using GetPluginVTableFn = const PluginVTable* (*)() noexcept;

} // namespace mt
