#pragma once

#include <unordered_set>
#include "../Fire/EasyNet.hpp"
#include "../Fire/EasySerializer.hpp"


#ifndef _DEBUG
#define REMOTE
#endif

#define SERVER_PORT 54000U

#ifdef REMOTE
#define SERVER_IP "31.210.43.142"
#else
#define SERVER_IP "127.0.0.1"
#endif

#define SERVER_URL "https://barisdoga.com/index.php"

#define CIRCLE_DEF ((PacketID_t)(10U))
#define POLY_DEF ((PacketID_t)(11U))
#define MATERIAL_DEF ((PacketID_t)(12U))
#define WORLD_DEF ((PacketID_t)(13U))
#define HELLO ((PacketID_t)(14U))



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