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
#define PLAYER_MOVEMENT                                         ((PacketID_t)(206U))
#define PLAYER_MOVEMENT_PACK                                    ((PacketID_t)(207U))
#define GAME_BOOT                                               ((PacketID_t)(208U))



class UserStats {
public:
    unsigned int uid{};
    unsigned int gametime{};
    unsigned int golds{};
    unsigned int diamonds{};
    bool tutorial_done{};
    std::vector<unsigned int> champions_owned{};
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
    std::string message;
    std::string username;
    unsigned long long timestamp;

    sChatMessage() : message(), username(), timestamp(), EasySerializeable(static_cast<PacketID_t>(CHAT_MESSAGE))
    {

    }

    sChatMessage(std::string message, std::string username, unsigned long long timestamp) : message(message), username(username), timestamp(timestamp), EasySerializeable(static_cast<PacketID_t>(CHAT_MESSAGE))
    {

    }

    ~sChatMessage()
    {
        
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(message);
        ser->Put(username);
        ser->Put(timestamp);
    }
};
REGISTER_PACKET(sChatMessage, CHAT_MESSAGE);

class sPlayerMovement : public EasySerializeable {
public:
    UserID_t userID;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 direction;
    unsigned long long timestamp;

    sPlayerMovement() : userID(), position(), rotation(), direction(), timestamp(), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT))
    {

    }

    sPlayerMovement(const sPlayerMovement& playerMovement) : userID(playerMovement.userID), position(playerMovement.position), rotation(playerMovement.rotation), direction(playerMovement.direction), timestamp(playerMovement.timestamp), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT))
    {

    }

    sPlayerMovement(UserID_t userID, glm::vec3 position, glm::vec3 rotation, glm::vec3 direction, unsigned long long timestamp) : userID(userID), position(position), rotation(rotation), direction(direction), timestamp(timestamp), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT))
    {

    }

    ~sPlayerMovement()
    {
        
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(userID);
        ser->Put(position);
        ser->Put(rotation);
        ser->Put(direction);
        ser->Put(timestamp);
    }
};
REGISTER_PACKET(sPlayerMovement, PLAYER_MOVEMENT);

class sPlayerMovementPack : public EasySerializeable {
public:
    std::vector<sPlayerMovement> movements;

    sPlayerMovementPack() : movements(), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT_PACK))
    {

    }

    sPlayerMovementPack(const sPlayerMovementPack& playerMovementPack) : movements(playerMovementPack.movements), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT_PACK))
    {

    }

    sPlayerMovementPack(const std::vector<sPlayerMovement>& movements) : movements(movements), EasySerializeable(static_cast<PacketID_t>(PLAYER_MOVEMENT_PACK))
    {

    }

    ~sPlayerMovementPack()
    {
        
    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(movements);
    }
};
REGISTER_PACKET(sPlayerMovementPack, PLAYER_MOVEMENT_PACK);

class sGameBoot : public EasySerializeable {
public:

    sGameBoot() : EasySerializeable(static_cast<PacketID_t>(GAME_BOOT))
    {

    }

    ~sGameBoot()
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        
    }
};
REGISTER_PACKET(sGameBoot, GAME_BOOT);