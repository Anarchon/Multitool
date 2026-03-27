#include "shared/plugin_api.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

std::atomic<bool> g_running{false};
std::string g_instance_id;
const mt::PluginHostCallbacks* g_callbacks = nullptr;

void safe_copy(char* dst, std::uint32_t dst_size, const std::string& src) {
  if (!dst || dst_size == 0) return;
  const auto len = std::min<std::size_t>(src.size(), dst_size - 1);
  std::memcpy(dst, src.data(), len);
  dst[len] = '\0';
}

bool initialize(const mt::InstanceContext* context, const mt::PluginHostCallbacks* callbacks) noexcept {
  g_callbacks = callbacks;
  g_instance_id = context && context->instance_id ? context->instance_id : "unknown";
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, g_instance_id.c_str(), "scintilla initialized");
  }
  return true;
}

bool start() noexcept {
  g_running.store(true, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event(g_instance_id.c_str(), "{\"topic\":\"scintilla.started\"}");
  }
  return true;
}

bool request_stop(int /*timeout_ms*/) noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event(g_instance_id.c_str(), "{\"topic\":\"scintilla.stopped\"}");
  }
  return true;
}

bool command(const char* command_json, char* response_buf, std::uint32_t response_buf_size) noexcept {
  const std::string cmd = command_json ? command_json : "{}";
  if (cmd.find("insert_text") != std::string::npos) {
    safe_copy(response_buf, response_buf_size, "{\"ok\":true,\"result\":\"inserted\"}");
    return true;
  }
  if (cmd.find("simulate_crash") != std::string::npos) {
    std::abort();
  }
  safe_copy(response_buf, response_buf_size, "{\"ok\":true,\"result\":\"noop\"}");
  return true;
}

bool health_check(char* response_buf, std::uint32_t response_buf_size) noexcept {
  safe_copy(response_buf, response_buf_size,
            g_running.load(std::memory_order_acquire) ? "{\"state\":\"running\"}" : "{\"state\":\"stopped\"}");
  return true;
}

void shutdown() noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, g_instance_id.c_str(), "scintilla shutdown");
  }
}

const mt::PluginInfo kInfo{
    mt::kPluginAbiVersion,
    "scintilla.editor",
    "Scintilla Editor Plugin",
    "2.0.0",
    "editor,syntax-highlighting"
};

const mt::PluginVTable kVTable{
    &initialize,
    &start,
    &request_stop,
    &command,
    &health_check,
    &shutdown
};

} // namespace

PLUGIN_EXPORT const mt::PluginInfo* GetPluginInfo() noexcept { return &kInfo; }
PLUGIN_EXPORT const mt::PluginVTable* GetPluginVTable() noexcept { return &kVTable; }
