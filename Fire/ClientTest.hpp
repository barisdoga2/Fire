#pragma once

#include "Net.hpp"

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
    EasyBufferManager& bf;
    std::string ip;
    unsigned short port;
    ClientPeer client;

public:
    ClientTest(EasyBufferManager& bf, std::string ip, unsigned short port);
    ~ClientTest();

    int ClientWebRequest();
    int ClientResetSendSequenceCounter();
    int ClientResetReceiveSequenceCounter();
    int ClientSend();
    int ClientReceive();
    int ClientBoth();
    int ClientSR();

};
