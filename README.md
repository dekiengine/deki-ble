# Deki BLE

Docs: https://dekiengine.github.io/deki-ble/ (components and properties, generated from the code)

Abstract Bluetooth Low Energy peripheral interface for the Deki Engine. Defines `IDekiBLE`: scanning, advertising, GATT server construction, characteristic notify/read/write callbacks, and central-role connections.

This package contains no radio driver of its own. A platform integration package (for example `deki-esp32-integration`) registers the concrete backend.

Part of [Deki Engine](https://github.com/dekiengine/deki-engine).

## Namespace

Types live in `DekiBle`. Scene files store the qualified name, and so does code:

```cpp
using namespace DekiBle;
obj->AddComponent<SomeComponent>();
```

Scenes saved before 0.16.0 used bare names and still load; saving writes the current one.

## Install

Package Manager in the Deki Editor, or `DekiEditor --packages-add deki-ble <project>`.

## Dependencies

| Dependency | Type |
|---|---|
| ESP-IDF | External, only on ESP32 targets |

## License

Apache 2.0. See [LICENSE](LICENSE).
