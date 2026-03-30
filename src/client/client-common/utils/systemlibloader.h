#pragma once

#include <string>
#include <system_error>
#include <utility>

#include <windows.h>

/*

This class ensures a DLL is loaded from the system32, or its localized equivalent, folder
to mitigate DLL hijacking attacks.

NOTES:
- You do not need to use this class for 'known' DLLs listed in HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs.
  The OS loader ensures all DLLs in that list, and any DLLs loaded by them, are only loaded from system32.
- One should use this class to replace any statically linked DLLs listed in cmake's target_link_libraries declaration.
  TL;DR your cmake file should not have a target_link_libraries declaration.

*/

namespace wsl {

class SystemLibLoader {
public:
    SystemLibLoader(const SystemLibLoader&) = delete;
    SystemLibLoader& operator=(const SystemLibLoader&) = delete;

    SystemLibLoader(SystemLibLoader &&other) noexcept : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    SystemLibLoader& operator=(SystemLibLoader &&other) noexcept
    {
        if (this != &other) {
            std::swap(handle_, other.handle_);
        }
        return *this;
    }

    explicit SystemLibLoader(const char *libName)
    {
        if (!libName) {
            throw std::invalid_argument("Null parameter passed to SystemLibLoader");
        }

        // NOTE: previously used the LOAD_LIBRARY_REQUIRE_SIGNED_TARGET flag here, but apparently some Windows
        // system DLLs do not support having their digital signature verified by the system loader.  Using this
        // flag will cause LoadLibraryEx to fail with ERROR_INVALID_IMAGE_HASH for those DLLs (e.g. wlanapi.dll).
        handle_ = ::LoadLibraryExA(libName, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (!handle_) {
            throw std::system_error(::GetLastError(), std::system_category(), std::string("SystemLibLoader failed to load DLL ") + libName);
        }
    }

    ~SystemLibLoader()
    {
        if (handle_) {
            ::FreeLibrary(handle_);
        }
    }

    static bool isAvailable(const char *libName) noexcept
    {
        if (!libName) {
            return false;
        }

        HMODULE handle = ::LoadLibraryExA(libName, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (handle) {
            ::FreeLibrary(handle);
            return true;
        }

        return false;
    }

    template<typename T>
    T *getFunction(const char *name) const
    {
        return reinterpret_cast<T *>(getProcAddress(name));
    }

private:
    HMODULE handle_ = nullptr;

    FARPROC getProcAddress(const char *name) const
    {
        if (!name) {
            throw std::invalid_argument("Null parameter passed to getProcAddress");
        }

        if (!handle_) {
            throw std::logic_error("The DLL has not been loaded");
        }

        auto symbol = ::GetProcAddress(handle_, name);

        if (symbol == nullptr) {
            throw std::system_error(::GetLastError(), std::system_category(), std::string("SystemLibLoader failed to load function ") + name);
        }

        return symbol;
    }
};

}
