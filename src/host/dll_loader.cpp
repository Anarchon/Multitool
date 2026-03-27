#include "dll_loader.hpp"

#if defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace mt {

bool DllLoader::load(const std::filesystem::path& path, std::string& error) {
#if defined(_WIN32)
  handle_ = reinterpret_cast<void*>(LoadLibraryW(path.wstring().c_str()));
  if (!handle_) {
    error = "LoadLibrary failed";
    return false;
  }
  auto get_info = reinterpret_cast<GetPluginInfoFn>(GetProcAddress((HMODULE)handle_, "GetPluginInfo"));
  auto get_vt = reinterpret_cast<GetPluginVTableFn>(GetProcAddress((HMODULE)handle_, "GetPluginVTable"));
#else
  handle_ = dlopen(path.c_str(), RTLD_NOW);
  if (!handle_) {
    error = dlerror();
    return false;
  }
  auto get_info = reinterpret_cast<GetPluginInfoFn>(dlsym(handle_, "GetPluginInfo"));
  auto get_vt = reinterpret_cast<GetPluginVTableFn>(dlsym(handle_, "GetPluginVTable"));
#endif

  if (!get_info || !get_vt) {
    error = "Required entry points missing";
    unload();
    return false;
  }

  info_ = get_info();
  vtable_ = get_vt();

  if (!info_ || !vtable_ || info_->abi_version != kPluginAbiVersion) {
    error = "ABI version mismatch";
    unload();
    return false;
  }

  return true;
}

void DllLoader::unload() {
  info_ = nullptr;
  vtable_ = nullptr;
  if (!handle_) {
    return;
  }
#if defined(_WIN32)
  FreeLibrary((HMODULE)handle_);
#else
  dlclose(handle_);
#endif
  handle_ = nullptr;
}

} // namespace mt
