#pragma once

#include "EasyNet.hpp"

#define CIRCLE_DEF ((PacketID_t)(10U))
#define POLY_DEF ((PacketID_t)(11U))
#define MATERIAL_DEF ((PacketID_t)(12U))
#define WORLD_DEF ((PacketID_t)(13U))
#define HELLO ((PacketID_t)(14U))

#define MAX_SESSIONS ((SessionID_t)(0b00000000'00001111'11111111'11111111))
#define IS_SESSION(x) ((((SessionID_t)x) | ~MAX_SESSIONS) > 0U)

class pHello : public EasyNetObj {
public:
    std::string message;

    pHello(std::string message) : message(message), EasyNetObj(static_cast<PacketID_t>(HELLO))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(packetID);
        ser->Put(message);
    }
};