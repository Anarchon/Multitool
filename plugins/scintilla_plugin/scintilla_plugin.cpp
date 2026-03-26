#include "shared/plugin_api.hpp"

#include <atomic>
#include <cstring>
#include <string>

namespace {

std::atomic<bool> g_running{false};
const mt::PluginHostCallbacks* g_callbacks = nullptr;

void safe_copy(char* dst, std::uint32_t dst_size, const std::string& src) {
  if (!dst || dst_size == 0) {
    return;
  }
  const auto len = std::min<std::size_t>(src.size(), dst_size - 1);
  std::memcpy(dst, src.data(), len);
  dst[len] = '\0';
}

bool init(const mt::PluginHostCallbacks* callbacks, const char* /*init_json*/) noexcept {
  g_callbacks = callbacks;
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, "scintilla plugin initialized");
  }
  return true;
}

bool start() noexcept {
  g_running.store(true, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event("{\"topic\":\"scintilla.started\"}");
  }
  return true;
}

bool stop() noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event("{\"topic\":\"scintilla.stopped\"}");
  }
  return true;
}

bool handle_command(const char* command_json, char* response_buf, std::uint32_t response_buf_size) noexcept {
  const std::string cmd = command_json ? command_json : "{}";
  if (cmd.find("insert_text") != std::string::npos) {
    safe_copy(response_buf, response_buf_size, "{\"ok\":true,\"result\":\"text inserted\"}");
    return true;
  }
  safe_copy(response_buf, response_buf_size, "{\"ok\":true,\"result\":\"noop\"}");
  return true;
}

void shutdown() noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, "scintilla plugin shutdown");
  }
}

const mt::PluginInfo kInfo{
    mt::kPluginAbiVersion,
    "scintilla.editor",
    "Scintilla Editor Plugin",
    "1.0.0",
    "editor,syntax-highlighting"
};

const mt::PluginVTable kVTable{
    &init,
    &start,
    &stop,
    &handle_command,
    &shutdown
};

} // namespace

PLUGIN_EXPORT const mt::PluginInfo* GetPluginInfo() noexcept {
  return &kInfo;
}

PLUGIN_EXPORT const mt::PluginVTable* GetPluginVTable() noexcept {
  return &kVTable;
}
