# Multitool Plugin-Architektur (C++)

## 1) Architekturdiagramm (Textform)

```text
+--------------------------------------------------------------------------------+
|                                Main Process                                    |
|--------------------------------------------------------------------------------|
| PluginRegistry | Supervisor/Watchdog | IPC Router | UI/CLI/API Layer           |
+---------------------------+--------------------+-------------------------------+
                            |                    |
            JSONL over pipe |                    | JSONL over pipe
                            v                    v
                +--------------------+  +--------------------+
                | Plugin Host A      |  | Plugin Host B      |
                | (gecko.host.exe)   |  | (cef.host.exe)     |
                |--------------------|  |--------------------|
                | DLL Loader         |  | DLL Loader         |
                | Crash Guard        |  | Crash Guard        |
                | Gecko Plugin DLL   |  | CEF Plugin DLL     |
                +--------------------+  +--------------------+
                            |                    |
                            |                    |
                            v                    v
                     geckoview/Firefox      Chromium Embedded
                     APIs + native code      Framework APIs
```

**Isolation-Regel:** Ein Absturz in `Plugin Host A` betrifft nur Gecko; `Plugin Host B` (CEF) und Main bleiben stabil.

## 2) Projektstruktur

```text
Multitool/
├─ CMakeLists.txt
├─ docs/
│  └─ architecture.md
├─ include/shared/
│  ├─ plugin_api.hpp          # stabiles C-ABI Interface
│  └─ ipc_contract.hpp        # IPC-Vertrag (JSONL)
├─ src/main/
│  ├─ main.cpp                # bootstrap main process
│  ├─ plugin_registry.hpp/.cpp
│  └─ supervisor.hpp/.cpp
├─ src/host/
│  ├─ plugin_host.cpp         # Host-Prozess + IPC loop
│  └─ dll_loader.hpp/.cpp     # DLL laden, ABI prüfen
└─ plugins/
   ├─ scintilla/
   │  └─ plugin.json          # Metadaten
   └─ scintilla_plugin/
      └─ scintilla_plugin.cpp # Beispiel-DLL
```

## 3) Gemeinsames Plugin-Interface

- C-ABI mit Exporten `GetPluginInfo`, `GetPluginVTable`.
- Kein C++-Objekt über DLL-Grenze (ABI-sicher).
- Host stellt Callback-Tabelle (`log`, `emit_event`) bereit.

## 4) Host-Interface zum Laden von DLL-Plugins

- `DllLoader::load(path, error)`:
  - lädt DLL (`LoadLibrary`/`dlopen`)
  - resolved Exporte
  - prüft `abi_version`
- `DllLoader::unload()` entlädt sauber.

## 5) IPC-Vertrag Main <-> Host

- Transport: `JSONL` über stdin/stdout Pipes (einfach, robust, gut debuggbar).
- Requests: `load`, `start`, `command`, `stop`, `shutdown`.
- Responses: `loaded`, `response`, `event`, `crash`.
- Erweiterbar über `type` + optionale Felder.

## 6) Plugin-Metadaten

Siehe `plugins/scintilla/plugin.json`.

Wichtig:
- `id`, `version`, `entry_library`
- Restart-Policy (`max_attempts`, Backoff)
- `abi` und `min_main_api`

## 7) Watchdog/Supervisor-Konzept

- Main hält pro Plugin genau einen Host-Zustand (`HostState`).
- Erkennt Crash über Prozess-Exit oder Heartbeat-Timeout.
- Restart-Policy pro Plugin (optional, begrenzt).
- Kein globaler Restart: nur betroffener Host wird neu gestartet.

## 8) Beispielcode

- Main: `src/main/main.cpp`, `supervisor.*`, `plugin_registry.*`
- Host: `src/host/plugin_host.cpp`, `dll_loader.*`
- Plugin: `plugins/scintilla_plugin/scintilla_plugin.cpp`

## 9) Deployment und Versionierung

- Plugin-Paket pro Plugin:
  - `plugin.json`
  - `bin/<plugin>.dll`
  - optionale Ressourcen
- ABI-Version strikt (`kPluginAbiVersion`).
- SemVer:
  - Main API (`min_main_api`)
  - Plugin-Version separat
- CI-Prüfung: Host lädt jedes Plugin im Smoke-Test.

## 10) Sichere Fehlerbehandlung

- Plugin-Crash darf nie Main crashen (separater Prozess).
- Jede IPC-Operation mit Timeout.
- Defensive Parsing-Regeln für JSON.
- Keine Exceptions über C-ABI-Grenze.
- Thread-Safety im Plugin via atomics/mutex.
- Circuit Breaker: nach N Crashes Plugin deaktivieren.

## C#-Alternative (optional)

Wenn Prozessmanagement und Telemetrie im Fokus stehen, kann der **Main/Supervisor** in C# umgesetzt werden
(`Process`, `System.IO.Pipes`, `async/await`), während Host+Plugins in C++ bleiben. Dadurch wird Orchestrierung einfacher,
bei gleichbleibender nativer Integrationsfähigkeit (Gecko/CEF/Scintilla).
