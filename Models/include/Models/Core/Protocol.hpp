#pragma once

#include <string>

namespace FalcataIoTServer
{
    enum Protocol : int
    {
        Mqtt = 1,
        UpcUa = 2,
        Modbus = 3,
        DDS = 4,
        WS = 5,
    };

}  // namespace FalcataIoTServer