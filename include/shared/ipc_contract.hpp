#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace mt::ipc {

// Wire format: one line per frame, semicolon-separated key-value pairs.
// Example:
// cmd=start;request_id=42;instance_id=inst-0001
// event=log;instance_id=inst-0001;level=1;message=started

inline std::string make_frame(const std::unordered_map<std::string, std::string>& fields) {
  std::ostringstream out;
  bool first = true;
  for (const auto& [k, v] : fields) {
    if (!first) out << ';';
    first = false;
    out << k << '=' << v;
  }
  return out.str();
}

inline std::unordered_map<std::string, std::string> parse_frame(const std::string& line) {
  std::unordered_map<std::string, std::string> fields;
  std::stringstream ss(line);
  std::string part;
  while (std::getline(ss, part, ';')) {
    const auto pos = part.find('=');
    if (pos == std::string::npos) continue;
    fields[part.substr(0, pos)] = part.substr(pos + 1);
  }
  return fields;
}

inline std::optional<std::string> get(const std::unordered_map<std::string, std::string>& fields,
                                      const std::string& key) {
  auto it = fields.find(key);
  if (it == fields.end()) return std::nullopt;
  return it->second;
}

} // namespace mt::ipc
