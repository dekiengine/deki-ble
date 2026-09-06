#pragma once

#include <deki/providers/IPackage.h>
#include <cstdint>
#include <cstddef>

/**
 * @brief Address type for a BLE peer.
 */
enum class DekiBLEAddrType : uint8_t
{
    Public                       = 0,
    RandomStatic                 = 1,
    RandomPrivateResolvable      = 2,
    RandomPrivateNonResolvable   = 3,
};

/**
 * @brief 48-bit BLE device address.
 */
struct DekiBLEAddress
{
    uint8_t         bytes[6] = {0, 0, 0, 0, 0, 0};
    DekiBLEAddrType type     = DekiBLEAddrType::Public;
};

/**
 * @brief BLE UUID. Stored canonically as 128-bit big-endian bytes.
 *
 * For SIG-assigned 16-bit UUIDs (e.g. 0x180D Heart Rate), set is16bit=true
 * and put the 16-bit value in shortId. The full 128-bit form is also filled
 * in (Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB with the
 * short id spliced into bytes[2..3]) so backends that only consume bytes[]
 * still work.
 */
struct DekiBLEUUID
{
    uint8_t  bytes[16] = {0};
    bool     is16bit   = false;
    uint16_t shortId  = 0;
};

/**
 * @brief One BLE advertiser observed during a scan.
 *
 * Carries both the parsed-out common fields and the raw advertisement /
 * scan-response payloads. Use the parsed fields for the common cases
 * (name lookup, manufacturer id filter); fall back to the raw bytes when
 * you need to parse a custom AD type (iBeacon, Eddystone, vendor frames).
 */
struct DekiBLEDevice
{
    DekiBLEAddress addr;
    int8_t         rssi          = 0;            // dBm
    char           name[32]      = {0};          // parsed Complete or Shortened Local Name, "" if absent

    // Raw payloads. adv_data is always the primary 31-byte payload; scan_resp
    // is only populated for active scans where the advertiser answered.
    uint8_t        adv_data[31]    = {0};
    uint8_t        advLen         = 0;
    uint8_t        scan_resp[31]   = {0};
    uint8_t        scanRespLen   = 0;

    // Parsed convenience fields (also derivable from the raw bytes).
    uint16_t       manufacturerId          = 0xFFFF;   // 0xFFFF when absent
    uint8_t        manufacturerData[27]    = {0};
    uint8_t        manufacturerDataLen    = 0;

    // First few advertised service UUIDs. If the advertiser lists more, the
    // tail is still parseable from adv_data.
    DekiBLEUUID    serviceUuids[4];
    uint8_t        serviceUuidCount       = 0;
};

/**
 * @brief What to broadcast when advertising.
 *
 * Fill the structured fields and the backend marshals them into a 31-byte
 * advertisement. Set rawOverride to push exact bytes instead (for iBeacon,
 * Eddystone, or any custom AD layout); the structured fields are ignored
 * when rawOverride != nullptr.
 */
struct DekiBLEAdvData
{
    const char*           localName              = nullptr;
    const DekiBLEUUID*    serviceUuids           = nullptr;
    uint8_t               serviceUuidCount      = 0;

    uint16_t              manufacturerId         = 0xFFFF;  // 0xFFFF = omit
    const uint8_t*        manufacturerData       = nullptr;
    uint8_t               manufacturerDataLen   = 0;

    bool                  connectable             = true;
    uint16_t              intervalMs             = 100;     // valid range 20..10240

    // If non-null, bypass struct-driven encoding and broadcast these bytes
    // verbatim. rawOverrideLen must be <= 31.
    const uint8_t*        rawOverride            = nullptr;
    uint8_t               rawOverrideLen        = 0;
};

/**
 * @brief Characteristic property bitmask.
 */
enum DekiBLECharProps : uint8_t
{
    DekiBLECharProp_Read        = 0x01,
    DekiBLECharProp_Write       = 0x02,
    DekiBLECharProp_WriteNoResp = 0x04,
    DekiBLECharProp_Notify      = 0x08,
    DekiBLECharProp_Indicate    = 0x10,
};

using DekiBLECharHandle = uint16_t;
using DekiBLEConnHandle = uint16_t;

static constexpr DekiBLECharHandle DekiBLEInvalidCharHandle = 0xFFFF;
static constexpr DekiBLEConnHandle DekiBLEInvalidConnHandle = 0xFFFF;

/**
 * @brief Description of one characteristic to expose on the GATT server.
 *
 * The backend fills `valueHandle` during BuildGattServer; callers retain
 * that handle to call NotifyValue or to match against incoming write
 * callbacks.
 */
struct DekiBLECharSpec
{
    DekiBLEUUID       uuid;
    uint8_t           props       = 0;          // bitmask of DekiBLECharProps
    uint16_t          maxLen     = 20;         // typical default MTU payload
    DekiBLECharHandle valueHandle = DekiBLEInvalidCharHandle;  // filled by BuildGattServer
};

/**
 * @brief One GATT service plus its characteristics.
 */
struct DekiBLEServiceSpec
{
    DekiBLEUUID      uuid;
    DekiBLECharSpec* chars       = nullptr;
    uint8_t          charCount  = 0;
};

// =============================================================================
// Callback typedefs
// =============================================================================

using DekiBLEScanCb       = void (*)(const DekiBLEDevice& device, void* user);
using DekiBLEConnCb       = void (*)(DekiBLEConnHandle conn, const DekiBLEAddress& peer,
                                     bool connected, void* user);
using DekiBLECharWriteCb  = void (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                                     const uint8_t* data, size_t len, void* user);
using DekiBLECharReadCb   = int  (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                                     uint8_t* out, size_t maxLen, void* user);
using DekiBLENotifyCb     = void (*)(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                                     const uint8_t* data, size_t len, void* user);

/**
 * @brief Abstract BLE radio.
 *
 * Covers the four BLE roles in one interface: scan (observer/central),
 * advertise (broadcaster/peripheral), GATT server, and GATT client. All
 * event-driven flows (scan results, GATT writes from a remote central,
 * incoming notifications) deliver through registered callbacks; the
 * start/stop calls themselves are non-blocking.
 *
 * No pairing / bonding policy here. Higher layers decide whether to require
 * encryption or persist keys. The default of every implementation is
 * "Just Works", no IO capability, no persisted bond.
 *
 * Implemented by whichever platform integration package is loaded at runtime.
 * The integration package registers its concrete driver with
 * DekiBLE::SetCurrent at package load. Single-active: one BT controller per
 * chip, no multi-provider registry needed for this category.
 */
class IDekiBLE : public Deki::IPackage
{
public:
    const char* GetPackageCategory() const override { return "ble"; }

    // -------------------------------------------------------------------------
    // Scanning (observer / central)
    // -------------------------------------------------------------------------

    /// Start scanning for nearby advertisers. Non-blocking: results stream
    /// through the callback registered via SetScanCallback. `intervalMs` and
    /// `windowMs` are the BLE scan interval/window (window <= interval).
    /// `active` requests scan responses; passive listening only when false.
    /// durationMs=0 means scan until StopScan; otherwise auto-stop after the
    /// duration elapses.
    virtual bool StartScan(uint16_t intervalMs, uint16_t windowMs,
                           bool active, uint32_t durationMs) = 0;

    virtual void StopScan() = 0;

    /// The callback is invoked from the BLE host task. Keep work small; copy
    /// out anything you need and post to your own queue.
    virtual void SetScanCallback(DekiBLEScanCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // Advertising (broadcaster / peripheral)
    // -------------------------------------------------------------------------

    /// Start advertising with the supplied payload. Replacing an in-flight
    /// advertisement is allowed: the call stops the current one first.
    virtual bool StartAdvertising(const DekiBLEAdvData& data) = 0;

    virtual void StopAdvertising() = 0;

    virtual bool IsAdvertising() const = 0;

    // -------------------------------------------------------------------------
    // GATT server
    // -------------------------------------------------------------------------

    /// Register the supplied services as a GATT server. The backend fills
    /// `valueHandle` in each DekiBLECharSpec on success. Calling
    /// BuildGattServer again replaces the previously-registered services
    /// (note: most stacks do not actually support re-registration; treat as
    /// build-once and reset by Shutdown -> Initialize if you need to rebuild).
    virtual bool BuildGattServer(DekiBLEServiceSpec* services, uint8_t count) = 0;

    /// Push a notification (or indication, if the characteristic was declared
    /// Indicate) to one connected central.
    virtual bool NotifyValue(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                             const void* data, size_t len) = 0;

    virtual void SetCharWriteCallback(DekiBLECharWriteCb cb, void* user) = 0;

    /// Read callback: backend supplies `out`/`maxLen`; callback returns the
    /// number of bytes written into `out`, or a negative value to NACK the
    /// read. Optional: if no callback is registered, the backend serves a
    /// zero-length value.
    virtual void SetCharReadCallback(DekiBLECharReadCb cb, void* user) = 0;

    virtual void SetConnectionCallback(DekiBLEConnCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // GATT client
    // -------------------------------------------------------------------------

    /// Queue a connection attempt to a remote peripheral. Non-blocking: the
    /// outcome (success + new handle, or failure) arrives via the connection
    /// callback. timeoutMs is the supervision timeout for the connect phase.
    virtual bool Connect(const DekiBLEAddress& addr, uint32_t timeoutMs) = 0;

    virtual void DisconnectClient(DekiBLEConnHandle conn) = 0;

    /// Discover characteristics of a service on the remote. On success,
    /// writes the first characteristic value handle into `outFirstHandle` and
    /// the number discovered into `outCount`. The returned handles are
    /// contiguous in attribute-handle order.
    virtual bool DiscoverService(DekiBLEConnHandle conn, const DekiBLEUUID& service,
                                 DekiBLECharHandle* outFirstHandle,
                                 uint8_t* outCount) = 0;

    virtual bool ReadRemote(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                            uint8_t* out, size_t* len) = 0;

    virtual bool WriteRemote(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                             const void* data, size_t len, bool with_response) = 0;

    /// Subscribe/unsubscribe to notifications by writing the CCCD descriptor
    /// immediately after `handle`. Incoming notifications surface through
    /// the notify callback.
    virtual bool Subscribe(DekiBLEConnHandle conn, DekiBLECharHandle handle,
                           bool enable) = 0;

    virtual void SetNotifyCallback(DekiBLENotifyCb cb, void* user) = 0;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// Tear down the BLE stack. Free advertise/scan/GATT state. Calling
    /// Initialize afterwards restarts from scratch.
    virtual void Shutdown() = 0;
};
