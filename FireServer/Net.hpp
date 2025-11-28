#pragma once

#include <unordered_set>
#include <vector>
#include <EasyNet.hpp>
#include <EasySerializer.hpp>


#ifndef _DEBUG
#define REMOTE
#endif

#define SERVER_PORT 54000U

#ifdef REMOTE
#define SERVER_IP "31.210.43.142"
#define SERVER_URL "https://barisdoga.com/index.php"
#else
#define SERVER_IP "127.0.0.1"
#define SERVER_URL "http://127.0.0.1/index.php"
#endif


#define CIRCLE_DEF ((PacketID_t)(10U))
#define POLY_DEF ((PacketID_t)(11U))
#define MATERIAL_DEF ((PacketID_t)(12U))
#define WORLD_DEF ((PacketID_t)(13U))

class UserStats {
public:
    unsigned int userID{};
    unsigned int gametime{};
    unsigned int golds{};
    unsigned int diamonds{};
    bool tutorial_done{};
    std::vector<unsigned int> champions_owned{};
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

class sPlayerBootInfo : public EasySerializeable {
public:
    unsigned int userID, diamonds, golds, gametime;
    bool tutorialDone;
    std::vector<unsigned int> championsOwned;

    sPlayerBootInfo() : userID(), diamonds(), golds(), gametime(), tutorialDone(), championsOwned(), EasySerializeable(static_cast<PacketID_t>(PLAYER_BOOT_INFO))
    {

    }

    sPlayerBootInfo(unsigned int userID, unsigned int diamonds, unsigned int golds, unsigned int gametime, bool tutorialDone, std::vector<unsigned int> championsOwned) : userID(userID), diamonds(diamonds), golds(golds), gametime(gametime), tutorialDone(tutorialDone), championsOwned(championsOwned), EasySerializeable(static_cast<PacketID_t>(PLAYER_BOOT_INFO))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(userID);
        ser->Put(diamonds);
        ser->Put(golds);
        ser->Put(gametime);
        ser->Put(tutorialDone);
        ser->Put(championsOwned);
    }
};
REGISTER_PACKET(sPlayerBootInfo, PLAYER_BOOT_INFO);



class sChampionSelectRequest : public EasySerializeable {
public:
    unsigned int championID;

    sChampionSelectRequest() : championID(), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_REQUEST))
    {

    }

    sChampionSelectRequest(unsigned int championID) : championID(championID), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_REQUEST))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(championID);
    }
};
REGISTER_PACKET(sChampionSelectRequest, CHAMPION_SELECT_REQUEST);

class sChampionBuyRequest : public EasySerializeable {
public:
    unsigned int championID;

    sChampionBuyRequest() : championID(), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_REQUEST))
    {

    }

    sChampionBuyRequest(unsigned int championID) : championID(championID), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_REQUEST))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(championID);
    }
};
REGISTER_PACKET(sChampionBuyRequest, CHAMPION_BUY_REQUEST);

class sChampionSelectResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sChampionSelectResponse() : response(), message(), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_RESPONSE))
    {

    }

    sChampionSelectResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};
REGISTER_PACKET(sChampionSelectResponse, CHAMPION_SELECT_RESPONSE);

class sChampionBuyResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sChampionBuyResponse() : response(), message(), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_RESPONSE))
    {

    }

    sChampionBuyResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};
REGISTER_PACKET(sChampionBuyResponse, CHAMPION_BUY_RESPONSE);



class sHearbeat : public EasySerializeable {
public:
    sHearbeat() : EasySerializeable(static_cast<PacketID_t>(HEARTBEAT))
    {

    }

    void Serialize(EasySerializer* ser) override
    {

    }
};
REGISTER_PACKET(sHearbeat, HEARTBEAT);

