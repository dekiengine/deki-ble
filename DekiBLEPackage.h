#pragma once

#ifdef _WIN32
    #ifdef DEKI_BLE_EXPORTS
        #define DEKI_BLE_API __declspec(dllexport)
    #else
        #define DEKI_BLE_API __declspec(dllimport)
    #endif
#else
    #define DEKI_BLE_API __attribute__((visibility("default")))
#endif
