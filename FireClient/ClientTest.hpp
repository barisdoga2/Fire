#pragma once

#include "../FireServer/Net.hpp"

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

public:
    ClientPeer client;

    ClientTest(EasyBufferManager& bf, std::string ip, unsigned short port);
    ~ClientTest();

    std::string ClientWebRequest(std::string url, std::string username, std::string password);
    uint64_t ClientReceive(PeerCryptInfo& crypt, std::vector<EasySerializeable*>& objs);
    uint64_t ClientSend(PeerCryptInfo& crypt, const std::vector<EasySerializeable*>& objs);

};
