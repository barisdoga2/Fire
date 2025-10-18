#pragma once

#include "EasyNet.hpp"

#define CIRCLE_DEF ((PacketID_t)(10U))
#define POLY_DEF ((PacketID_t)(11U))
#define MATERIAL_DEF ((PacketID_t)(12U))
#define WORLD_DEF ((PacketID_t)(13U))
#define HELLO ((PacketID_t)(14U))

class pHello : public EasyNetObj {
public:
    std::string message;

    pHello(std::string message) : message(message), EasyNetObj(static_cast<PacketID_t>(HELLO))
    {

    }

    void Serialize(EasySerializer* ser)
    {
        EasyNetObj::Serialize(ser);
        ser->Put(message);
    }
};