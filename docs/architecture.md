# Robustes Multitool-Plugin-System (Windows-first, C++)

## 1) Architekturübersicht

Die Architektur trennt strikt:
1. **Main Application** (GUI + Supervisor + Routing)
2. **Plugin Host Process** (genau 1 Prozess pro Plugin-Instanz)
3. **Plugin DLL** (unsicherer/nativer Code)

**Sicherheitsregel:** Main lädt **nie** Plugin-DLLs direkt.

## 2) Architekturdiagramm (Text)

```text
+--------------------------------------------------------------------------------+
| MAIN APP (GUI + Supervisor + PluginRegistry + InstanceManager + IPC Client)    |
|--------------------------------------------------------------------------------|
| Plugin-Typen (Definitionen)            Laufende Instanzen (plugin_id+instance) |
+---------------------------+----------------------+-----------------------------+
                            | Start/Stop/Status/Command via IPC                 
          +-----------------+--------------------------+------------------------+
          |                                            |                        |
          v                                            v                        v
+---------------------------+               +---------------------------+   ...
| HOST PROCESS inst=ff#1    |               | HOST PROCESS inst=ff#2    |
| plugin_id=firefox.gecko   |               | plugin_id=firefox.gecko   |
|---------------------------|               |---------------------------|
| DLL Loader + ABI Check    |               | DLL Loader + ABI Check    |
| Heartbeat + Timeout       |               | Heartbeat + Timeout       |
| firefox_plugin.dll        |               | firefox_plugin.dll        |
+---------------------------+               +---------------------------+
          |
          v
+---------------------------+
| HOST PROCESS inst=cef#1   |
| plugin_id=chromium.cef    |
| cef_plugin.dll            |
+---------------------------+
```

## 3) Instanzmodell

- **PluginDefinition** = statischer Plugin-Typ (`plugin_id`, Name, Version, DLL-Pfad)
- **PluginInstance** = laufende Instanz (`instance_id`, Status, eigener Host-Prozess, eigene IPC)
- **TabViewModel** = GUI-Repräsentation einer Instanz (`tab_id -> instance_id`)

Status pro Instanz:
- Starting
- Running
- Stopping
- Stopped
- Crashed

## 4) Lebenszyklus

`Discover -> LoadDefinition -> StartInstance -> HostSpawn -> DLL Initialize -> Start -> Running -> Stop/Shutdown -> Unload`

Crash-Pfad:
`Running -> HostExit/Timeout -> Crashed -> optional Restart (nur diese Instanz)`

## 5) IPC-Strategie (konkret)

Verwendet wird ein **line-basiertes Frame-Protokoll** (lokal, robust debuggbar):
- Ein Frame pro Zeile
- `key=value;key=value`

Beispiele:
- Request: `cmd=start;request_id=42;instance_id=firefox.gecko#1`
- Request: `cmd=command;payload={"action":"navigate"}`
- Response/Event: `event=response;ok=true;payload={...}`
- Heartbeat: `event=heartbeat;instance_id=...`

Ziele erfüllt:
- Start/Stop/Status
- Kommandos
- Logs/Events
- Heartbeat/Timeout-Erkennung

## 6) GUI-Tab-Verhalten

- Jeder Tab entspricht genau einer `instance_id`.
- Titel z. B. `Firefox Plugin firefox.gecko#2`.
- Klick auf X:
  1. `stop_instance(instance_id, timeout)`
  2. Graceful shutdown via IPC
  3. Bei Timeout: Force kill nur dieses Host-PID
  4. Entferne nur diesen Tab

## 7) Watchdog / Supervisor

- Main führt Instanzmap: `instance_id -> PluginInstance`.
- Heartbeat-Timer pro Instanz.
- Bei Crash/Timeout: markiere `Crashed`, optional instanzspezifischer Restart.
- Restart-Limit pro Instanz (`max_restart_attempts`).

## 8) ABI/DLL-Sicherheit

- Stabiler C-ABI-Vertrag (`GetPluginInfo`, `GetPluginVTable`)
- `kPluginAbiVersion` hart geprüft
- Keine C++-Objekte über DLL-Grenze
- Nur POD/C-Strings + Funktionszeiger

## 9) Thread-Sicherheit und Robustheit

- Plugin-intern `std::atomic` für Running-State
- Keine Exceptions über ABI-Grenzen
- Defensives Parsing im Host
- Timeouts für Stop und Heartbeats
- Ressourcenfreigabe pro Instanz (kein globales Shared-Fate)

## 10) Empfehlung Technologie-Stack

**Empfehlung für dieses Projekt:**
- **C++ + Qt (GUI)** für produktive Windows-Tooling-Apps mit stabilem Tab-UI und Prozesssteuerung.
- **C++ Hosts + DLLs** für native Browser/Gecko/CEF Integration.

Alternativ:
- C# (WPF) als Main/Supervisor + C++ Host/DLL ist gut für schnelle GUI-Entwicklung.
- Reines ImGui ist pragmatisch für interne Tools, aber weniger „Desktop-App-poliert" als Qt.

## 11) Deployment/Versionierung

- Paket pro Plugin-Typ:
  - `plugin.json`
  - `bin/<plugin>.dll`
  - optionale Ressourcen/Config
- SemVer auf Plugin-Version
- Harte ABI-Version (`abi` + `kPluginAbiVersion`)
- Main-API-Kompatibilität via `min_main_api`

## 12) Warum DLL + separater Host-Prozess?

- DLL erlaubt modulare native Erweiterung mit direkter API-Nähe (Gecko/CEF/Scintilla)
- Separater Host-Prozess kapselt Crash-Risiko vollständig
- Kombiniert Performance/Flexibilität (DLL) mit Stabilität/Isolation (Prozessgrenze)

## 13) Beispiel: Gecko-Plugin

Ein konkretes Gecko-Plugin-Skelett liegt in `plugins/gecko_plugin/gecko_plugin.cpp` und implementiert:
- `navigate` (URL setzen)
- `current_url` (Statusabfrage)
- `simulate_crash` (Crash-Test für Supervisor/Recovery)

Damit lassen sich mehrere Firefox/Gecko-Instanzen parallel in getrennten Host-Prozessen testen.
