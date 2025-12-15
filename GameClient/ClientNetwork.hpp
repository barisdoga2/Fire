#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <EasySocket.hpp>
#include <EasyBuffer.hpp>
#include "EasyNet.hpp"
#include "Config.hpp"
#include <../GameServer/ServerNet.hpp>
#include <EasyIpAddress.hpp>

class ClientSession {
public:
    std::string username{}, password{};
    SessionID_t sid{};
    Addr_t addr{};
    Key_t key{};
    Key_Expt_t key_expiry{};
    SequenceID_t seqid_in{}, seqid_out{};
    Timestamp_t lastReceive{};

    UserID_t uid{};
    EasySocket sck{};
    std::mutex cacheM{};
    std::vector<EasySerializeable*> receiveCache{};
    std::vector<EasySerializeable*> sendCache{};
    UserStats stats{};

    ClientSession() 
    {

    }

    void Stop()
    {
        sid = 0U;
        addr = 0U;
        key = {};
        key_expiry = {};
        seqid_in = 0U;
        seqid_out = 0U;
        lastReceive = {};

        uid = 0U;
        sck = {};
        receiveCache.clear();
        sendCache.clear();

        stats.uid = 0U;
        stats.gametime = 0U;
        stats.golds = 0U;
        stats.diamonds = 0U;
        stats.tutorial_done = 0U;
        stats.champions_owned = {};
    }
};

class ClientCallback {
public:
    virtual void OnLogin() = 0U;
    virtual void OnDisconnect() = 0U;

};

class ClientNetwork {
    EasyBufferManager* bufferMan;
    ClientCallback* cbk;

    bool isInGame{};
    bool isLoggingIn{};
    bool isAuthing{};
    bool isAuth{};
    bool isLoginFailed{};
    bool isChampionSelect{};
    std::string loginStatus{};
    std::thread authThread{};

    bool Auth();

    bool SendOne();

    bool ReceiveOne();

public:
    ClientSession session;

    ClientNetwork(EasyBufferManager* bufferMan, ClientCallback* cbk);

    void Disconnect(std::string serverMessage = "");

    void Stop();

    void Update();

    void Login();

    std::vector<EasySerializeable*>& GetReceiveCache();

    std::vector<EasySerializeable*>& GetSendCache();

    std::string StatusText();

    void ChampionSelect(uint8_t cid);

    bool IsLoggingIn() const;

    bool IsLoginFailed() const;

    bool IsInGame() const;

    bool IsChampionSelect() const;

};
