#pragma once

#include <string>

namespace mt::ipc {

// JSONL wire format (one JSON object per line)
// Main -> Host requests:
// {"type":"load","plugin_path":"...","plugin_id":"..."}
// {"type":"start","request_id":"r-1"}
// {"type":"command","request_id":"r-2","command":{"action":"open"}}
// {"type":"stop","request_id":"r-3"}
// {"type":"shutdown","request_id":"r-4"}
//
// Host -> Main responses/events:
// {"type":"loaded","ok":true,"plugin":{"id":"...","version":"..."}}
// {"type":"response","request_id":"r-2","ok":true,"payload":{...}}
// {"type":"event","event":{"topic":"telemetry"}}
// {"type":"crash","reason":"unhandled exception"}

struct Envelope {
  std::string type;
  std::string raw_json;
};

} // namespace mt::ipc
