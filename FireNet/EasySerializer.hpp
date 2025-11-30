#pragma once
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <glm/glm.hpp>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"

class EasySerializer;
class EasySerializeable {
public:
    PacketID_t packetID;
    virtual void Serialize(EasySerializer* ser) = 0U;
    EasySerializeable(PacketID_t packetID) : packetID(packetID)
    {

    }
};

class PacketFactory
{
public:
    using Creator = EasySerializeable * (*)();

    static PacketFactory& Instance()
    {
        static PacketFactory inst;
        return inst;
    }

    void Register(PacketID_t id, Creator creator)
    {
        creators[id] = creator;
    }

    EasySerializeable* Create(PacketID_t id)
    {
        auto it = creators.find(id);
        return (it != creators.end()) ? it->second() : nullptr;
    }

private:
    std::unordered_map<PacketID_t, Creator> creators;
};

#define REGISTER_PACKET(packetClass, packetID) \
    namespace { \
        EasySerializeable* Create##packetClass() { return new packetClass(); } \
        struct packetClass##AutoRegister { \
            packetClass##AutoRegister() { PacketFactory::Instance().Register(packetID, &Create##packetClass); } \
        }; \
        static packetClass##AutoRegister global_##packetClass##AutoRegister; \
    }

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
        //static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

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
static int lastDel = 0;
class sLogoutRequest : public EasySerializeable {
public:

    sLogoutRequest() : EasySerializeable(static_cast<PacketID_t>(LOGOUT_REQUEST))
    {
        
    }

    ~sLogoutRequest()
    {
        lastDel = 1;
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
        lastDel = 2;
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
        lastDel = 3;
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
        lastDel = 4;
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

static inline bool MakeDeserialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer des(buff);
    des.head = EasyPacket::HeaderSize();
    bool status = true;
    while (des.head < buff->m_payload_size + EasyPacket::HeaderSize() && status)
    {
        PacketID_t id = *(PacketID_t*)(buff->begin() + des.head);
        EasySerializeable* packet = PacketFactory::Instance().Create(id);
        if (!packet)
        {
            status = false;
        }
        else
        {
            des.Deserialize(*packet);
            peer_cache.push_back(packet);
        }
    }

    return status;
}

static inline bool MakeSerialized(EasyBuffer* buff, const std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer ser(buff);
    bool status = true;
    ser.head = EasyPacket::HeaderSize();
    for (EasySerializeable* o : peer_cache)
    {
        ser.Serialize(*o);
    }
    buff->m_payload_size = ser.head - EasyPacket::HeaderSize();
    return status;
}
