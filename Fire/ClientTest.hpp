#pragma once

#include <iostream>
#include "EasyNet.hpp"
#include "EasyPeer.hpp"

class EasySocket;
class ClientPeer : public EasyPeer {
public:
    EasySocket* socket;
    Timestamp_t lastReceive;

    PeerCryptInfo* crypt;

    uint32_t user_id;
 
    ClientPeer(std::string key = "fbdRjmU4rvENzNCy");
    ~ClientPeer();
};

struct lua_State;
class ClientTest {
private:
    static inline ClientTest* instance;

    EasyBufferManager& bf;
    std::string ip;
    unsigned short port;
    ClientPeer client;

public:
    ClientTest(EasyBufferManager& bf, std::string ip, unsigned short port);
    ~ClientTest();

public:
    static int ClientWebRequestS(lua_State* L);
    static int ClientResetSendSequenceCounterS(lua_State* L);
    static int ClientResetReceiveSequenceCounterS(lua_State* L);
    static int ClientSendS(lua_State* L);
    static int ClientReceiveS(lua_State* L);
    static int ClientBothS(lua_State* L);
    static int ClientSRS(lua_State* L);

private:
    int ClientWebRequest(lua_State* L);
    int ClientResetSendSequenceCounter(lua_State* L);
    int ClientResetReceiveSequenceCounter(lua_State* L);
    int ClientSend(lua_State* L);
    int ClientReceive(lua_State* L);
    int ClientBoth(lua_State* L);
    int ClientSR(lua_State* L);

};
