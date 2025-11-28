#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"

class EasySerializer;
class EasySerializeable {
public:
    PacketID_t packetID;
    virtual void Serialize(EasySerializer* ser) = 0U;
    EasySerializeable(PacketID_t packetID) : packetID(packetID)
    {

    }
};

class EasySerializer {
public:
    uint8_t state = 0U;
    size_t head = 0U;
    EasyBuffer* bf;

    EasySerializer(EasyBuffer* bf) : bf(bf)
    {

    }

    void Serialize(EasySerializeable& serializeable)
    {
        state = 1U;
        Put(serializeable.packetID);
        serializeable.Serialize(this);
    }

    void Deserialize(EasySerializeable& serializeable)
    {
        state = 2U;
        Put(serializeable.packetID);
        serializeable.Serialize(this);
    }

    template <class T>
    void Put(T& v)
    {
        if constexpr (std::is_base_of_v<EasySerializeable, T>)
        {
            v.Serialize(this);
        }
        else
        {
            if (state == 1U)
            {
                memcpy(bf->begin() + head, &v, sizeof(T));
            }
            else if (state == 2U)
            {
                memcpy((uint8_t*)&v, bf->begin() + head, sizeof(T));
            }
        }
        head += sizeof(T);
    }

    template <class T>
    void Put(std::vector<T>& v)
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        size_t size = v.size();
        Put(size);
        v.resize(size);
        for (T& c : v)
            Put(c);
    }

    void Put(glm::vec2& v)
    {
        Put(v.x);
        Put(v.y);
    }

    void Put(std::string& v)
    {

        size_t size = v.length();
        Put(size);
        v.resize(size);
        for (const char& c : v)
            Put(c);
    }

};



class sLoginResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sLoginResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(LOGIN_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};

class sDisconnectResponse : public EasySerializeable {
public:
    std::string message;

    sDisconnectResponse(std::string message) : message(message), EasySerializeable(static_cast<PacketID_t>(DISCONNECT_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(message);
    }
};

class sPlayerBootInfo : public EasySerializeable {
public:
    unsigned int userID, diamonds, golds, gametime;
    bool tutorialDone;
    std::vector<unsigned int> championsOwned;

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



class sChampionSelectRequest : public EasySerializeable {
public:
    unsigned int championID;

    sChampionSelectRequest(unsigned int championID) : championID(championID), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_REQUEST))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(championID);
    }
};

class sChampionBuyRequest : public EasySerializeable {
public:
    unsigned int championID;

    sChampionBuyRequest(unsigned int championID) : championID(championID), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_REQUEST))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(championID);
    }
};

class sChampionSelectResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sChampionSelectResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(CHAMPION_SELECT_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};

class sChampionBuyResponse : public EasySerializeable {
public:
    bool response;
    std::string message;

    sChampionBuyResponse(bool response, std::string message) : response(response), message(message), EasySerializeable(static_cast<PacketID_t>(CHAMPION_BUY_RESPONSE))
    {

    }

    void Serialize(EasySerializer* ser) override
    {
        ser->Put(response);
        ser->Put(message);
    }
};

