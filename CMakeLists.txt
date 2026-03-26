cmake_minimum_required(VERSION 3.21)
project(Multitool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(plugin_api INTERFACE)
target_include_directories(plugin_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(multitool_main
  src/main/main.cpp
  src/main/plugin_registry.cpp
  src/main/supervisor.cpp
)
target_link_libraries(multitool_main PRIVATE plugin_api)

add_executable(plugin_host
  src/host/plugin_host.cpp
  src/host/dll_loader.cpp
)
target_link_libraries(plugin_host PRIVATE plugin_api)

add_library(scintilla_plugin SHARED
  plugins/scintilla_plugin/scintilla_plugin.cpp
)
target_link_libraries(scintilla_plugin PRIVATE plugin_api)
target_compile_definitions(scintilla_plugin PRIVATE BUILDING_PLUGIN_DLL=1)
