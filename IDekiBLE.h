#pragma once

#include <deki/providers/IPackage.h>
#include <cstdint>
#include <cstddef>

namespace DekiBle
{

/// Address type of a BLE peer.
enum class DekiBLEAddrType : uint8_t
{
    Public = 0,
    RandomStatic = 1,
    RandomPrivateResolvable = 2,
    RandomPrivateNonResolvable = 3,
};

/// 48-bit BLE device address.
struct DekiBLEAddress
{
    uint8_t bytes[6] = { 0, 0, 0, 0, 0, 0 };
    DekiBLEAddrType type = DekiBLEAddrType::Public;
};

/// BLE UUID, stored as 128-bit big-endian bytes.
///
/// For a SIG-assigned 16-bit UUID (e.g. 0x180D Heart Rate), set is16bit=true
/// and put the value in shortId. Fill in the full 128-bit form too (the
/// Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB with the short id
/// in bytes[2..3]), so backends that read only bytes[] still work.
struct DekiBLEUUID
{
    uint8_t bytes[16] = { 0 };
    bool is16bit = false;
    uint16_t shortId = 0;
};

/// One BLE advertiser seen during a scan.
///
/// Has the common fields already parsed and the raw advertisement and scan
/// response payloads. Use the parsed fields for the common cases (name,
/// manufacturer id filter) and the raw bytes for a custom AD type (iBeacon,
/// Eddystone, vendor frames).
struct DekiBLEDevice
{
    DekiBLEAddress addr;
    int8_t rssi = 0;        // dBm
    char name[32] = { 0 };  // parsed Complete or Shortened Local Name, "" if absent

    // Raw payloads. advData is always the primary 31-byte payload; scanResp
    // is filled only by an active scan the advertiser answered.
    uint8_t advData[31] = { 0 };
    uint8_t advLen = 0;
    uint8_t scanResp[31] = { 0 };
    uint8_t scanRespLen = 0;

    // Parsed fields (also in the raw bytes).
    uint16_t manufacturerId = 0xFFFF;  // 0xFFFF when absent
    uint8_t manufacturerData[27] = { 0 };
    uint8_t manufacturerDataLen = 0;

    // The first advertised service UUIDs. Any more are still in advData.
    DekiBLEUUID serviceUuids[4];
    uint8_t serviceUuidCount = 0;
};

/// What to broadcast when advertising.
///
/// The backend packs the fields into a 31-byte advertisement. Set rawOverride
/// to broadcast exact bytes instead (for iBeacon, Eddystone or any custom AD
/// layout); the other fields are then ignored.
struct DekiBLEAdvData
{
    const char* localName = nullptr;
    const DekiBLEUUID* serviceUuids = nullptr;
    uint8_t serviceUuidCount = 0;

    uint16_t manufacturerId = 0xFFFF;  // 0xFFFF = omit
    const uint8_t* manufacturerData = nullptr;
    uint8_t manufacturerDataLen = 0;

    bool connectable = true;
    uint16_t intervalMs = 100;  // valid range 20..10240

    // When set, these bytes are broadcast as they are. rawOverrideLen must be
    // <= 31.
    const uint8_t* rawOverride = nullptr;
    uint8_t rawOverrideLen = 0;
};

/// Characteristic property bitmask.
enum DekiBLECharProps : uint8_t
{
    DekiBLECharPropRead = 0x01,
    DekiBLECharPropWrite = 0x02,
    DekiBLECharPropWriteNoResp = 0x04,
    DekiBLECharPropNotify = 0x08,
    DekiBLECharPropIndicate = 0x10,
};

using DekiBLECharHandle = uint16_t;
using DekiBLEConnHandle = uint16_t;

static constexpr DekiBLECharHandle kDekiBLEInvalidCharHandle = 0xFFFF;
static constexpr DekiBLEConnHandle kDekiBLEInvalidConnHandle = 0xFFFF;

/// One characteristic to expose on the GATT server. The backend fills
/// `valueHandle` in BuildGattServer; callers keep it for NotifyValue and to
/// match incoming write callbacks.
struct DekiBLECharSpec
{
    DekiBLEUUID uuid;
    uint8_t props = 0;                                          // bitmask of DekiBLECharProps
    uint16_t maxLen = 20;                                       // typical default MTU payload
    DekiBLECharHandle valueHandle = kDekiBLEInvalidCharHandle;  // filled by BuildGattServer
};

/// One GATT service and its characteristics.
struct DekiBLEServiceSpec
{
    DekiBLEUUID uuid;
    DekiBLECharSpec* chars = nullptr;
    uint8_t charCount = 0;
};

// =============================================================================
// Callback typedefs
// =============================================================================

using DekiBLEScanCb = void (*)(const DekiBLEDevice& device, void* user);
using DekiBLEConnCb = void (*)(DekiBLEConnHandle conn, const DekiBLEAddress& peer, bool connected, void* user);
using DekiBLECharWriteCb = void (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle, const uint8_t* data, size_t len,
                                    void* user);
using DekiBLECharReadCb = int (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle, uint8_t* out, size_t maxLen,
                                  void* user);
using DekiBLENotifyCb = void (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle, const uint8_t* data, size_t len,
                                 void* user);

/// A BLE radio.
///
/// Covers the four BLE roles: scan (observer/central), advertise
/// (broadcaster/peripheral), GATT server and GATT client. Events (scan
/// results, GATT writes from a remote central, incoming notifications) arrive
/// through registered callbacks; the start/stop calls do not block.
///
/// Pairing and bonding policy belongs to higher layers, which decide whether
/// to require encryption or keep keys. Every implementation defaults to
/// "Just Works", no IO capability, no stored bond.
///
/// Implemented by the platform integration package loaded at runtime, which
/// registers its driver with DekiBLE::SetCurrent when it loads. One active
/// driver: a chip has one BT controller.
class IDekiBLE : public Deki::IPackage
{
public:
    const char* GetPackageCategory() const override { return "ble"; }

    // -------------------------------------------------------------------------
    // Scanning (observer / central)
    // -------------------------------------------------------------------------

    /// Starts scanning for nearby advertisers without blocking; results arrive
    /// through the SetScanCallback callback. `intervalMs` and `windowMs` are
    /// the BLE scan interval and window (window <= interval). `active` asks
    /// for scan responses; false only listens. durationMs = 0 scans until
    /// StopScan; otherwise the scan stops after that long.
    virtual bool StartScan(uint16_t intervalMs, uint16_t windowMs, bool active, uint32_t durationMs) = 0;

    virtual void StopScan() = 0;

    /// The callback runs on the BLE host task. Keep it short: copy out what
    /// you need and post it to your own queue.
    virtual void SetScanCallback(DekiBLEScanCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // Advertising (broadcaster / peripheral)
    // -------------------------------------------------------------------------

    /// Starts advertising `data`. If already advertising, the current
    /// advertisement is stopped first.
    virtual bool StartAdvertising(const DekiBLEAdvData& data) = 0;

    virtual void StopAdvertising() = 0;

    virtual bool IsAdvertising() const = 0;

    // -------------------------------------------------------------------------
    // GATT server
    // -------------------------------------------------------------------------

    /// Registers `services` as a GATT server and, on success, fills
    /// `valueHandle` in each DekiBLECharSpec. Calling it again replaces the
    /// services, but most stacks do not support that: build once, and call
    /// Shutdown then Initialize to rebuild.
    virtual bool BuildGattServer(DekiBLEServiceSpec* services, uint8_t count) = 0;

    /// Sends a notification (an indication, if the characteristic was
    /// declared Indicate) to one connected central.
    virtual bool NotifyValue(DekiBLEConnHandle conn, DekiBLECharHandle handle, const void* data, size_t len) = 0;

    virtual void SetCharWriteCallback(DekiBLECharWriteCb cb, void* user) = 0;

    /// The read callback gets `out`/`maxLen` and returns the number of bytes
    /// written into `out`, or a negative value to refuse the read. Without a
    /// callback, the backend serves a zero-length value.
    virtual void SetCharReadCallback(DekiBLECharReadCb cb, void* user) = 0;

    virtual void SetConnectionCallback(DekiBLEConnCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // GATT client
    // -------------------------------------------------------------------------

    /// Starts connecting to a remote peripheral without blocking. The outcome
    /// (success with the new handle, or failure) arrives through the
    /// connection callback. timeoutMs is the supervision timeout for the
    /// connect phase.
    virtual bool Connect(const DekiBLEAddress& addr, uint32_t timeoutMs) = 0;

    virtual void DisconnectClient(DekiBLEConnHandle conn) = 0;

    /// Discovers the characteristics of a service on the remote. On success,
    /// writes the first characteristic's value handle to `outFirstHandle` and
    /// the number found to `outCount`; the handles are contiguous in
    /// attribute-handle order.
    virtual bool DiscoverService(DekiBLEConnHandle conn, const DekiBLEUUID& service, DekiBLECharHandle* outFirstHandle,
                                 uint8_t* outCount) = 0;

    virtual bool ReadRemote(DekiBLEConnHandle conn, DekiBLECharHandle handle, uint8_t* out, size_t* len) = 0;

    virtual bool WriteRemote(DekiBLEConnHandle conn, DekiBLECharHandle handle, const void* data, size_t len,
                             bool withResponse) = 0;

    /// Subscribes or unsubscribes to notifications by writing the CCCD
    /// descriptor right after `handle`. Notifications arrive through the
    /// notify callback.
    virtual bool Subscribe(DekiBLEConnHandle conn, DekiBLECharHandle handle, bool enable) = 0;

    virtual void SetNotifyCallback(DekiBLENotifyCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// Stops the BLE stack and frees the advertise, scan and GATT state.
    /// Initialize afterwards starts from scratch.
    virtual void Shutdown() = 0;
};

}  // namespace DekiBle
