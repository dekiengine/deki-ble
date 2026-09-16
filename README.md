# Deki BLE

Documentation: https://dekiengine.github.io/deki-ble/ (components and properties, generated from the code)

Abstract Bluetooth Low Energy peripheral interface for the Deki Engine. Defines `IDekiBLE`: scanning, advertising, GATT server construction, characteristic notify/read/write callbacks, and central-role connections.

This package contains no radio driver of its own. A platform integration package (for example `deki-esp32-integration`) registers the concrete backend.

Part of the [Deki Engine](https://github.com/dekiengine/deki-engine) package ecosystem.

## Namespace

This package's types live in `DekiBle`. Scene files store the qualified
name, so a component is `DekiBle::SomeComponent` there, and code naming one
needs the namespace:

```cpp
using namespace DekiBle;
obj->AddComponent<SomeComponent>();
```

Scenes saved before 0.16.0 used bare names and still load: every component
records what it used to be called, and a save writes the current name.

## Installation

Install via the Package Manager inside the Deki Editor.

## Dependencies

| Dependency | Type |
|---|---|
| ESP-IDF | External, only on ESP32 targets |

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
