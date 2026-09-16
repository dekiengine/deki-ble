#pragma once

#include "IDekiBLE.h"
#include "DekiBLEPackage.h"

namespace DekiBle
{

/**
 * @brief Active-driver registry for BLE (single-instance).
 *
 * Mirrors the DekiWifi::DekiWiFi / DekiHttp pattern: a platform integration package
 * Consumers (game code, BLE-backed sensors, provisioning helpers) reach the
 * active driver via GetCurrent.
 *
 * Single-active rather than the multi-provider registry pattern: there is
 * one BT controller per chip, swapping the driver at runtime is not a
 * realistic use case. If a board ever ships a secondary controller, this
 * category can switch to the multi-provider pattern without changing the
 * consumer call sites.
 */
class DEKI_BLE_API DekiBLE
{
public:
    static void      SetCurrent(IDekiBLE* driver);
    static IDekiBLE* GetCurrent();
};

}  // namespace DekiBle
