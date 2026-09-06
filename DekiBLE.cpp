#include "DekiBLE.h"

namespace {
    IDekiBLE* s_Current = nullptr;
}

void DekiBLE::SetCurrent(IDekiBLE* driver)
{
    s_Current = driver;
}

IDekiBLE* DekiBLE::GetCurrent()
{
    return s_Current;
}
