#pragma once

#include <unordered_set>
#include <vector>
#include <deque>
#include <EasyNet.hpp>
#include <EasySerializer.hpp>

#ifndef _DEBUG
#define REMOTE
#endif

#define SERVER_PORT 54000U
#define SESSION_TIMEOUT 10000U

#ifdef REMOTE
#define SERVER_IP "31.210.43.142"
#define SERVER_URL "https://barisdoga.com/index.php"
#else
#define SERVER_IP "127.0.0.1"
#define SERVER_URL "http://127.0.0.1/index.php"
#endif

#define FIRE_ENCRYPTION true
#define FIRE_COMPRESSION true

#define LOGIN_REQUEST                                           ((PacketID_t)(100U))
#define LOGIN_RESPONSE                                          ((PacketID_t)(101U))
#define LOGOUT_REQUEST                                          ((PacketID_t)(102U))
#define DISCONNECT_RESPONSE                                     ((PacketID_t)(103U))
#define HEARTBEAT                                               ((PacketID_t)(104U))
#define BROADCAST_MESSAGE                                       ((PacketID_t)(105U))

#define CHAMPION_SELECT_REQUEST                                 ((PacketID_t)(200U))
#define CHAMPION_BUY_REQUEST                                    ((PacketID_t)(201U))
#define CHAMPION_SELECT_RESPONSE                                ((PacketID_t)(202U))
#define CHAMPION_BUY_RESPONSE                                   ((PacketID_t)(203U))
#define PLAYER_BOOT_INFO                                        ((PacketID_t)(204U))
#define CHAT_MESSAGE                                            ((PacketID_t)(205U))
#define GAME_BOOT                                               ((PacketID_t)(206U))

#define PLAYER_INPUT                                            ((PacketID_t)(300U))
#define PLAYER_STATE                                            ((PacketID_t)(301U))
#define WORLD_STATE                                             ((PacketID_t)(302U))
#define PLAYER_INFO                                             ((PacketID_t)(303U))

class UserStats {
public:
    unsigned int uid{};
    unsigned int gametime{};
    unsigned int golds{};
    unsigned int diamonds{};
    bool tutorial_done{};
    std::vector<unsigned int> champions_owned{};
};

class sPlayerInput : public EasySerializeable
{
public:
    UserID_t  uid{};
    SequenceID_t  inputSeq{};
    uint32_t  dtMs{};
    glm::vec3 moveDir{};;
    uint64_t  clientTimeMs{};

    sPlayerInput() : EasySerializeable(static_cast<PacketID_t>(PLAYER_INPUT))
    {

    }

    sPlayerInput(UserID_t uid_, SequenceID_t seq_, uint32_t dtMs_, glm::vec3 dir_, uint64_t ctm_) : EasySerializeable(static_cast<PacketID_t>(PLAYER_INPUT)), uid(uid_), inputSeq(seq_), dtMs(dtMs_), moveDir(dir_), clientTimeMs(ctm_)
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(uid);
        ser->Put(inputSeq);
        ser->Put(dtMs);
        ser->Put(moveDir);
        ser->Put(clientTimeMs);
    }
};
REGISTER_PACKET(sPlayerInput, PLAYER_INPUT);

class FireSession {
public:
    std::string username;
    SessionID_t sid;
    UserID_t uid;
    Addr_t addr;
    UserStats stats;
    Timestamp_t recv;

    glm::vec3 position{ 0.f };
    glm::vec3 velocity{ 0.f };
    uint64_t lastInputSeq{ 0 };
    std::deque<sPlayerInput> inputQueue;

    bool logoutRequested{};

    FireSession(std::string username, SessionID_t sid, UserID_t uid, Addr_t addr, UserStats stats, Timestamp_t recv) : username(username), sid(sid), uid(uid), addr(addr), stats(stats), recv(recv), logoutRequested(false)
    {

    }

};

class sLogoutRequest : public EasySerializeable {
public:

    sLogoutRequest() : EasySerializeable(static_cast<PacketID_t>(LOGOUT_REQUEST))
    {

    }

    ~sLogoutRequest()
    {

    }

    void Serialize(EasySerializer* ser) override
    {

    }
};
REGISTER_PACKET(sLogoutRequest, LOGOUT_REQUEST);

class sLoginRequest : public EasySerializeable {
public:
    sLoginRequest() : EasySerializeable(static_cast<PacketID_t>(LOGIN_REQUEST))
    {

    }

    ~sLoginRequest()
    {

    }

    void Serialize(EasySerializer* ser) override
    {

    }
};
REGISTER_PACKET(sLoginRequest, LOGIN_REQUEST);

class sLoginResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sLoginResponse() : response(), message(), EasySerializeable(static_cast<PacketID_t>(LOGIN_RESPONSE))
    {

    }

    ~sLoginResponse()
    {

    }

    sLoginResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(LOGIN_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};
REGISTER_PACKET(sLoginResponse, LOGIN_RESPONSE);

class sDisconnectResponse : public EasySerializeable {
public:
    std::string message;

    sDisconnectResponse() : message(), EasySerializeable(static_cast<PacketID_t>(DISCONNECT_RESPONSE))
    {

    }

    ~sDisconnectResponse()
    {

    }

    sDisconnectResponse(std::string message) : message(message), EasySerializeable(static_cast<PacketID_t>(DISCONNECT_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(message);
    }
};
REGISTER_PACKET(sDisconnectResponse, DISCONNECT_RESPONSE);

class sHearbeat : public EasySerializeable {
public:
    sHearbeat() : EasySerializeable(static_cast<PacketID_t>(HEARTBEAT))
    {

    }

    ~sHearbeat()
    {

    }

    void Serialize(EasySerializer* ser) override
    {

    }
};
REGISTER_PACKET(sHearbeat, HEARTBEAT);

class sBroadcastMessage : public EasySerializeable {
public:
    std::string message;

    sBroadcastMessage() : message(), EasySerializeable(static_cast<PacketID_t>(BROADCAST_MESSAGE))
    {

    }

    sBroadcastMessage(std::string message) : message(message), EasySerializeable(static_cast<PacketID_t>(BROADCAST_MESSAGE))
    {

    }

    ~sBroadcastMessage()
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(message);
    }
};
REGISTER_PACKET(sBroadcastMessage, BROADCAST_MESSAGE);


class sPlayerBootInfo : public EasySerializeable {
public:
    unsigned int userID, diamonds, golds, gametime;
    bool tutorialDone;
    std::vector<unsigned int> championsOwned;

    sPlayerBootInfo() : userID(), diamonds(), golds(), gametime(), tutorialDone(), championsOwned(), EasySerializeable(static_cast<PacketID_t>(PLAYER_BOOT_INFO))
    {

    }

    ~sPlayerBootInfo()
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

    ~sChampionSelectRequest()
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

    ~sChampionBuyRequest()
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

    ~sChampionSelectResponse()
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

    ~sChampionBuyResponse()
    {
        
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};
REGISTER_PACKET(sChampionBuyResponse, CHAMPION_BUY_RESPONSE);

class sChatMessage : public EasySerializeable {
public:
    std::string username;
    std::string message;
    unsigned long long timestamp;

    sChatMessage() : username(), message(), timestamp(), EasySerializeable(static_cast<PacketID_t>(CHAT_MESSAGE))
    {

    }

    sChatMessage(std::string username, std::string message, unsigned long long timestamp) : username(username), message(message), timestamp(timestamp), EasySerializeable(static_cast<PacketID_t>(CHAT_MESSAGE))
    {

    }

    ~sChatMessage()
    {
        
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(username);
        ser->Put(message);
        ser->Put(timestamp);
    }
};
REGISTER_PACKET(sChatMessage, CHAT_MESSAGE);

class sPlayerState : public EasySerializeable {
public:
    UserID_t uid;
    glm::vec3 position;
    glm::vec3 velocity;
    uint64_t lastInputSequence;
    unsigned long long serverTimestamp;

    sPlayerState() : uid(), position(), velocity(), lastInputSequence(), serverTimestamp(), EasySerializeable(static_cast<PacketID_t>(PLAYER_STATE))
    {

    }

    sPlayerState(UserID_t uid, glm::vec3 position, glm::vec3 velocity, uint64_t lastInputSequence, unsigned long long serverTimestamp) : uid(uid), position(position), velocity(velocity), lastInputSequence(lastInputSequence), serverTimestamp(serverTimestamp), EasySerializeable(static_cast<PacketID_t>(PLAYER_STATE))
    {

    }

    ~sPlayerState()
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(uid);
        ser->Put(position);
        ser->Put(velocity);
        ser->Put(lastInputSequence);
        ser->Put(serverTimestamp);
    }
};
REGISTER_PACKET(sPlayerState, PLAYER_STATE);

class sPlayerInfo : public EasySerializeable {
public:
    UserID_t uid;
    std::string username;

    sPlayerInfo() : uid(), username(), EasySerializeable(static_cast<PacketID_t>(PLAYER_INFO))
    {

    }

    sPlayerInfo(UserID_t uid, std::string username) : uid(uid), username(username), EasySerializeable(static_cast<PacketID_t>(PLAYER_INFO))
    {

    }

    ~sPlayerInfo()
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(uid);
        ser->Put(username);
    }
};
REGISTER_PACKET(sPlayerInfo, PLAYER_INFO);

class sGameBoot : public EasySerializeable {
public:
    std::vector<sPlayerInfo> playerInfo;
    std::vector<sPlayerState> playerState;

    sGameBoot() : playerInfo(), playerState(), EasySerializeable(static_cast<PacketID_t>(GAME_BOOT))
    {

    }

    sGameBoot(const std::unordered_map<SessionID_t, FireSession*> sessions) : playerInfo(), playerState(), EasySerializeable(static_cast<PacketID_t>(GAME_BOOT))
    {
        for (auto& [sid, fs] : sessions)
        {
            playerInfo.push_back({ fs->stats.uid, fs->username });
            playerState.push_back({ fs->stats.uid, fs->position, fs->velocity, fs->lastInputSeq, 0U });
        }
    }

    ~sGameBoot()
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(playerInfo);
        ser->Put(playerState);
    }
};
REGISTER_PACKET(sGameBoot, GAME_BOOT);

class sWorldState : public EasySerializeable {
public:
    std::vector<sPlayerState> playerState;

    sWorldState() : playerState(), EasySerializeable(static_cast<PacketID_t>(WORLD_STATE))
    {

    }

    sWorldState(const std::unordered_map<SessionID_t, FireSession*> sessions) : playerState(), EasySerializeable(static_cast<PacketID_t>(WORLD_STATE))
    {
        for (auto& [sid, fs] : sessions)
        {
            playerState.emplace_back(fs->stats.uid, fs->position, fs->velocity, fs->lastInputSeq, 0U);
        }
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(playerState);
    }
};
REGISTER_PACKET(sWorldState, WORLD_STATE);
