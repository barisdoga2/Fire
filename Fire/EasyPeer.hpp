#pragma once

#include <iostream>
#include <queue>
#include <chrono>
#include <unordered_set>
#include <glm/glm.hpp>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"
#include "EasyIpAddress.hpp"

struct NetPlayer {
    glm::vec2 position;
    std::unordered_set<uint32_t> subscribedChunks;

};

class EasyPeer {
public:
    Addr_t addr;
    SessionID_t session_id;
    SequenceID_t sequence_id_in, sequence_id_out;
    std::chrono::high_resolution_clock::time_point lastReceive;

    AES_t secret;
    NetPlayer player;
    std::vector<EasyNetObj*> receive;

    EasyPeer(const EasyPeer& peer)
        : addr(peer.addr), session_id(peer.session_id), sequence_id_in(peer.sequence_id_in), sequence_id_out(peer.sequence_id_out),
        secret(peer.secret), player(peer.player), lastReceive(peer.lastReceive), receive(peer.receive)
    {

    }

    EasyPeer()
        : addr(0), session_id(0), sequence_id_in(0), sequence_id_out(0), secret(Key_t(KEY_SIZE), IV_t(IV_SIZE)), 
        player({{ 0.0f, 0.0f }, {}}), lastReceive(), receive()
    {

    }

    ~EasyPeer()
    {
        
    }

    bool alive()
    {
        return (lastReceive + std::chrono::seconds(10U)) > std::chrono::high_resolution_clock::now();
    }

    bool operator<(const EasyPeer& b) const noexcept
    {
        return session_id < b.session_id;
    }

};