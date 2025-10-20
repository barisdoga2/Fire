#pragma once

#include <iostream>
#include <queue>
#include <unordered_set>
#include <glm/glm.hpp>

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

    EasyPeer() : player({{ 0.0f, 0.0f }, {}})
    {

    }

};