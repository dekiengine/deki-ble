/**
 * @file DekiBLEPackage.cpp
 * @brief Package entry point for deki-ble
 */
#include "DekiBLEPackage.h"
#include "DekiBLE.h"
#include <deki/interop/Plugin.h>
#include <deki/LogSystem.h>

#ifdef DEKI_EDITOR
extern void DekiBLE_RegisterComponents();
extern int  DekiBLE_GetAutoComponentCount();
extern const Deki::ComponentMeta* DekiBLE_GetAutoComponentMeta(int index);
#endif

static bool s_BLERegistered = false;

extern "C" {

DEKI_BLE_API int DekiBLE_EnsureRegistered(void)
{
#ifdef DEKI_EDITOR
    if (s_BLERegistered) return DekiBLE_GetAutoComponentCount();
    s_BLERegistered = true;
    DekiBLE_RegisterComponents();
    return DekiBLE_GetAutoComponentCount();
#else
    return 0;
#endif
}

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)    { return "Deki BLE Package"; }
DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_PACKAGE_VERSION
    return DEKI_PACKAGE_VERSION;
#else
    return "0.0.0-dev";
#endif
}

DEKI_PLUGIN_API int  DekiPlugin_Init(void)
{
    DEKI_LOG_INFO("[deki-ble] DekiPlugin_Init");
    return 0;
}

DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
{
    // Null the active driver so a hot-reload of the integration package that
    // owns it doesn't leave a dangling pointer to its vtable.
    DekiBLE::SetCurrent(nullptr);
    s_BLERegistered = false;
}

#ifdef DEKI_EDITOR
DEKI_PLUGIN_API int  DekiPlugin_GetComponentCount(void) { return DekiBLE_GetAutoComponentCount(); }
DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiBLE_GetAutoComponentMeta(index);
}
#else
DEKI_PLUGIN_API int  DekiPlugin_GetComponentCount(void) { return 0; }
DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPlugin_GetComponentMeta(int) { return nullptr; }
#endif

DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
#ifdef DEKI_EDITOR
    int n = DekiBLE_EnsureRegistered();
    DEKI_LOG_INFO("[deki-ble] DekiPlugin_RegisterComponents -> %d component(s)", n);
#endif
}


// This package owns only the IDekiBLE interface and the SetCurrent/GetCurrent
// facade — it registers no provider of its own. Concrete drivers live in the
// platform integration packages and call DekiBLE::SetCurrent themselves.

}  // extern "C"
