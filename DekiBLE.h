#pragma once

#include "IDekiBLE.h"
#include "DekiBLEPackage.h"

namespace DekiBle
{

/// Holds the one active BLE driver, like DekiWifi::DekiWiFi and DekiHttp: a
/// platform integration package sets it with SetCurrent, and users (game code,
/// BLE sensors, provisioning helpers) reach it with GetCurrent.
///
/// One active driver, not a multi-provider registry: a chip has one BT
/// controller. If a board ever has a second one, this can become a
/// multi-provider registry without changing the call sites.
class DEKI_BLE_API DekiBLE
{
public:
    static void SetCurrent(IDekiBLE* driver);
    static IDekiBLE* GetCurrent();
};

}  // namespace DekiBle
