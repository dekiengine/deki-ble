/**
 * @file DekiBLEPackage.cpp
 * @brief Package entry point for deki-ble
 */
#include "DekiBLEPackage.h"
#include "DekiBLE.h"
#include <deki/interop/Plugin.h>
#include <deki/LogSystem.h>

extern void DekiBLERegisterComponents();
extern int DekiBLEGetAutoComponentCount();
extern const Deki::ComponentMeta* DekiBLEGetAutoComponentMeta(int index);

namespace DekiBle
{

#ifdef DEKI_EDITOR
#endif

static bool s_BLERegistered = false;

}  // namespace DekiBle
// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiBle;

extern "C"
{
    DEKI_BLE_API int DekiBLEEnsureRegistered(void)
    {
#ifdef DEKI_EDITOR
        if (s_BLERegistered)
        {
            return ::DekiBLEGetAutoComponentCount();
        }
        s_BLERegistered = true;
        ::DekiBLERegisterComponents();
        return ::DekiBLEGetAutoComponentCount();
#else
        return 0;
#endif
    }

    DEKI_PLUGIN_API const char* DekiPluginGetName(void)
    {
        return "Deki BLE Package";
    }
    DEKI_PLUGIN_API const char* DekiPluginGetVersion(void)
    {
#ifdef DEKI_PACKAGE_VERSION
        return DEKI_PACKAGE_VERSION;
#else
        return "0.0.0-dev";
#endif
    }

    DEKI_PLUGIN_API int DekiPluginInit(void)
    {
        return 0;
    }

    DEKI_PLUGIN_API void DekiPluginShutdown(void)
    {
        // Null the active driver so a hot-reload of the integration package that
        // owns it doesn't leave a dangling pointer to its vtable.
        DekiBLE::SetCurrent(nullptr);
        s_BLERegistered = false;
    }

#ifdef DEKI_EDITOR
    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return ::DekiBLEGetAutoComponentCount();
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int index)
    {
        return ::DekiBLEGetAutoComponentMeta(index);
    }
#else
    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return 0;
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int)
    {
        return nullptr;
    }
#endif

    DEKI_PLUGIN_API void DekiPluginRegisterComponents(void)
    {
#ifdef DEKI_EDITOR
        DekiBLEEnsureRegistered();
#endif
    }

    // This package owns only the IDekiBLE interface and the SetCurrent/GetCurrent
    // facade — it registers no provider of its own. Concrete drivers live in the
    // platform integration packages and call DekiBLE::SetCurrent themselves.

}  // extern "C"
