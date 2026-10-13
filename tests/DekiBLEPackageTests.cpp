// deki-ble: the BLE driver a platform package makes current, and the package's plugin entry points, which
// register its components with the editor once however often it asks and let
// a reloaded package register them again.
//
// The registration functions reflection codegen generates are stood in for
// here, so the entry file is tested as it is.

#include <gtest/gtest.h>

#include "DekiBLE.h"

#include <deki/PackageConfig.h>
#include <deki/interop/Plugin.h>

#include <cstring>

using namespace DekiBle;

namespace
{
int g_Registered = 0;
}

// Stand-ins for the generated registration functions the entry file calls.
void DekiBLERegisterComponents()
{
    ++g_Registered;
}
int DekiBLEGetAutoComponentCount()
{
    return 3;
}
const Deki::ComponentMeta* DekiBLEGetAutoComponentMeta(int)
{
    return nullptr;
}

// The entry points, from the package's entry file.
extern "C"
{
    int DekiBLEEnsureRegistered(void);
    const char* DekiPluginGetName(void);
    const char* DekiPluginGetVersion(void);
    void DekiPluginShutdown(void);
    int DekiPluginGetComponentCount(void);
    void DekiPluginRegisterComponents(void);
}

namespace
{

class PackageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        DekiPluginShutdown();
        g_Registered = 0;
    }
};

class Fake : public IDekiBLE
{
public:
    const char* GetPackageId() const override { return "ble"; }
    const char* GetPackageName() const override { return "Fake"; }
    void Configure(const Deki::PackageConfig&) override {}
    bool Initialize() override { return true; }
    void Shutdown() override {}
    void Update(float) override {}
    Deki::PackageState GetState() const override { return Deki::PackageState::Running; }
    const char* GetLastError() const override { return ""; }
    bool StartScan(uint16_t, uint16_t, bool, uint32_t) override { return false; }
    void StopScan() override {}
    void SetScanCallback(DekiBLEScanCb, void*) override {}
    bool StartAdvertising(const DekiBLEAdvData&) override { return false; }
    void StopAdvertising() override {}
    bool IsAdvertising() const override { return false; }
    bool BuildGattServer(DekiBLEServiceSpec*, uint8_t) override { return false; }
    bool NotifyValue(DekiBLEConnHandle, DekiBLECharHandle, const void*, size_t) override { return false; }
    void SetCharWriteCallback(DekiBLECharWriteCb, void*) override {}
    void SetCharReadCallback(DekiBLECharReadCb, void*) override {}
    void SetConnectionCallback(DekiBLEConnCb, void*) override {}
    bool Connect(const DekiBLEAddress&, uint32_t) override { return false; }
    void DisconnectClient(DekiBLEConnHandle) override {}
    bool DiscoverService(DekiBLEConnHandle, const DekiBLEUUID&, DekiBLECharHandle*, uint8_t*) override { return false; }
    bool ReadRemote(DekiBLEConnHandle, DekiBLECharHandle, uint8_t*, size_t*) override { return false; }
    bool WriteRemote(DekiBLEConnHandle, DekiBLECharHandle, const void*, size_t, bool) override { return false; }
    bool Subscribe(DekiBLEConnHandle, DekiBLECharHandle, bool) override { return false; }
    void SetNotifyCallback(DekiBLENotifyCb, void*) override {}
};

}  // namespace

TEST_F(PackageTest, TheCurrentDriverIsWhatWasSet)
{
    Fake driver;
    EXPECT_EQ(DekiBLE::GetCurrent(), nullptr);
    DekiBLE::SetCurrent(&driver);
    EXPECT_EQ(DekiBLE::GetCurrent(), &driver);
    DekiBLE::SetCurrent(nullptr);
    EXPECT_EQ(DekiBLE::GetCurrent(), nullptr);
}

TEST_F(PackageTest, ShutdownForgetsTheCurrentDriver)
{
    // The driver lives in the platform package's DLL; kept past shutdown,
    // the editor would call into unloaded code after a reload.
    Fake driver;
    DekiBLE::SetCurrent(&driver);
    DekiPluginShutdown();
    EXPECT_EQ(DekiBLE::GetCurrent(), nullptr);
}

TEST_F(PackageTest, ComponentsAreRegisteredOnce)
{
    EXPECT_EQ(DekiBLEEnsureRegistered(), 3);
    EXPECT_EQ(DekiBLEEnsureRegistered(), 3);
    DekiPluginRegisterComponents();
    EXPECT_EQ(g_Registered, 1);
    EXPECT_EQ(DekiPluginGetComponentCount(), 3);
}

TEST_F(PackageTest, AReloadedPackageRegistersAgain)
{
    DekiBLEEnsureRegistered();
    DekiPluginShutdown();
    DekiBLEEnsureRegistered();
    EXPECT_EQ(g_Registered, 2);
}

TEST_F(PackageTest, ThePluginNamesItselfAndItsVersion)
{
    EXPECT_GT(std::strlen(DekiPluginGetName()), 0u);
    EXPECT_STREQ(DekiPluginGetVersion(), "tests");  // DEKI_PACKAGE_VERSION, as the build sets it
}
