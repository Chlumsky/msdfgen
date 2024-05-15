#include "freetype-loader.h"

#include <vector>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define MSDFGEN_WINDOWS
#else
#include <dlfcn.h>
#endif

namespace msdfgen {

struct DynamicLibrary {
    void* handle;

    explicit DynamicLibrary(const std::vector<std::string>& names)
        : handle(nullptr) {
        for(const auto& name : names) {
#if defined(MSDFGEN_WINDOWS)
            // TODO: implement Windows
#else
            handle = dlopen(name.c_str(), RTLD_LAZY);
#endif
            if(handle) {
                break; // If we loaded one, stop trying
            }
        }
    }

    ~DynamicLibrary() {
        if(handle) {
#if defined(MSDFGEN_WINDOWS)
            // TODO: implement Windows
#else
            dlclose(handle);
#endif
        }
    }

    [[nodiscard]] void* getFunction(const char* name) {
#if defined(MSDFGEN_WINDOWS)
        return nullptr; // TODO: implement Windows
#else
        return dlsym(handle, name);
#endif
    }
};

[[nodiscard]] void* loadFreetypeFunction(const char* name) {
    static DynamicLibrary library{{
#ifdef MSDFGEN_WINDOWS
        "freetype.dll",
        "freetype6.dll"
#else
#if defined(__APPLE__)
        "libfreetype.dylib",
        "libfreetype.6.dylib"
#else
        "libfreetype.so",
        "libfreetype.so.6"
#endif
#endif
    }};
    return library.getFunction(name);
}

}