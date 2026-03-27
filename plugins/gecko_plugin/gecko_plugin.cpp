#include "shared/plugin_api.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

std::atomic<bool> g_running{false};
std::string g_instance_id;
std::string g_current_url = "about:blank";
const mt::PluginHostCallbacks* g_callbacks = nullptr;

void write_buf(char* dst, std::uint32_t size, const std::string& text) {
  if (!dst || size == 0) return;
  const auto n = std::min<std::size_t>(text.size(), size - 1);
  std::memcpy(dst, text.data(), n);
  dst[n] = '\0';
}

bool initialize(const mt::InstanceContext* context, const mt::PluginHostCallbacks* callbacks) noexcept {
  g_callbacks = callbacks;
  g_instance_id = context && context->instance_id ? context->instance_id : "unknown";
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, g_instance_id.c_str(), "gecko plugin initialized");
  }
  return true;
}

bool start() noexcept {
  g_running.store(true, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event(g_instance_id.c_str(), "{\"topic\":\"gecko.started\"}");
  }
  return true;
}

bool request_stop(int /*timeout_ms*/) noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->emit_event) {
    g_callbacks->emit_event(g_instance_id.c_str(), "{\"topic\":\"gecko.stopped\"}");
  }
  return true;
}

bool command(const char* command_json, char* response_buf, std::uint32_t response_buf_size) noexcept {
  const std::string cmd = command_json ? command_json : "{}";

  if (cmd.find("navigate") != std::string::npos) {
    const auto marker = "url=\"";
    const auto p = cmd.find(marker);
    if (p != std::string::npos) {
      const auto start = p + std::strlen(marker);
      const auto end = cmd.find('"', start);
      if (end != std::string::npos) {
        g_current_url = cmd.substr(start, end - start);
      }
    }
    write_buf(response_buf, response_buf_size, "{\"ok\":true,\"result\":\"navigated\"}");
    return true;
  }

  if (cmd.find("current_url") != std::string::npos) {
    write_buf(response_buf, response_buf_size, "{\"ok\":true,\"url\":\"" + g_current_url + "\"}");
    return true;
  }

  if (cmd.find("simulate_crash") != std::string::npos) {
    std::abort();
  }

  write_buf(response_buf, response_buf_size, "{\"ok\":false,\"error\":\"unknown command\"}");
  return false;
}

bool health_check(char* response_buf, std::uint32_t response_buf_size) noexcept {
  const std::string state = g_running.load(std::memory_order_acquire) ? "running" : "stopped";
  write_buf(response_buf, response_buf_size,
            "{\"state\":\"" + state + "\",\"engine\":\"gecko\",\"url\":\"" + g_current_url + "\"}");
  return true;
}

void shutdown() noexcept {
  g_running.store(false, std::memory_order_release);
  if (g_callbacks && g_callbacks->log) {
    g_callbacks->log(1, g_instance_id.c_str(), "gecko plugin shutdown");
  }
}

const mt::PluginInfo kInfo{
    mt::kPluginAbiVersion,
    "firefox.gecko",
    "Firefox Gecko Plugin",
    "1.0.0",
    "browser,gecko,navigation"
};

const mt::PluginVTable kVTable{
    &initialize,
    &start,
    &request_stop,
    &command,
    &health_check,
    &shutdown,
};

} // namespace

PLUGIN_EXPORT const mt::PluginInfo* GetPluginInfo() noexcept { return &kInfo; }
PLUGIN_EXPORT const mt::PluginVTable* GetPluginVTable() noexcept { return &kVTable; }
