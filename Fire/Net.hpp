#pragma once

#include <unordered_set>
#include "EasyNet.hpp"
#include "EasySerializer.hpp"

#define CIRCLE_DEF ((PacketID_t)(10U))
#define POLY_DEF ((PacketID_t)(11U))
#define MATERIAL_DEF ((PacketID_t)(12U))
#define WORLD_DEF ((PacketID_t)(13U))
#define HELLO ((PacketID_t)(14U))

#define MAX_SESSIONS ((SessionID_t)(0b00000000'00001111'11111111'11111111))
#define IS_SESSION(x) ((((SessionID_t)x) | ~MAX_SESSIONS) > 0U)

class pHello : public EasySerializeable {
public:
    std::string message;

    pHello(std::string message) : message(message), EasySerializeable(static_cast<PacketID_t>(HELLO))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(message);
    }
};

struct NetPlayer {
    glm::vec2 position;
    std::unordered_set<uint32_t> subscribedChunks;

};

class EasyPeer {
public:
    NetPlayer player;

    EasyPeer(const EasyPeer& peer) : player(peer.player)
    {

    }

    EasyPeer() : player({ { 0.0f, 0.0f }, {} })
    {

    }

};