#pragma once
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <glm/glm.hpp>
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"

class EasySerializer;
class EasySerializeable {
public:
    static inline unsigned long long total_creates{}, total_deletes{};

    PacketID_t packetID;

    EasySerializeable(PacketID_t packetID) : packetID(packetID)
    {
        total_creates++;
    }

    //EasySerializeable(const EasySerializeable& easySerializeable) = delete;

    EasySerializeable() = delete;

    ~EasySerializeable()
    {
        total_deletes++;
    }

    virtual void Serialize(EasySerializer* ser) = 0U;

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
    enum Mode { Write, Read } mode;
    size_t head = 0;
    EasyBuffer* bf;

    EasySerializer(EasyBuffer* bf, Mode m) : bf(bf), mode(m) {}

    template<class T>
    void PutWrite(const T& v)
    {
        memcpy(bf->begin() + head, &v, sizeof(T));
        head += sizeof(T);
    }

    template<class T>
    void PutRead(T& v)
    {
        memcpy(&v, bf->begin() + head, sizeof(T));
        head += sizeof(T);
    }

    template<class T>
    void Put(T& v)
    {
        if constexpr (std::is_base_of_v<EasySerializeable, T>)
        {
            v.Serialize(this);
        }
        else
        {
            (mode == Write) ? PutWrite(v) : PutRead(v);
        }
    }

    // string
    void Put(std::string& s)
    {
        size_t size = s.length();
        if (mode == Write)
        {
            PutWrite(size);
            memcpy(bf->begin() + head, s.data(), size);
            head += size;
        }
        else
        {
            PutRead(size);
            s.resize(size);
            memcpy(s.data(), bf->begin() + head, size);
            head += size;
        }
    }

    // vector
    template<class T>
    void Put(std::vector<T>& vec)
    {
        size_t size = vec.size();
        if (mode == Write)
        {
            PutWrite(size);
            for (auto& x : vec) Put(x);
        }
        else
        {
            PutRead(size);
            vec.resize(size);
            for (auto& x : vec) Put(x);
        }
    }
};

static inline bool MakeDeserialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer des(buff, EasySerializer::Mode::Read);
    des.head = EasyPacket::HeaderSize();
    bool status = true;
    while (des.head < buff->m_payload_size + EasyPacket::HeaderSize() && status)
    {
        PacketID_t id;
        des.PutRead(id);

        EasySerializeable* obj = PacketFactory::Instance().Create(id);
        if (!obj)
        {
            status = false;
        }
        else
        {
            obj->Serialize(&des);
            peer_cache.push_back(obj);
        }
    }

    return status;
}

static inline bool MakeSerialized(EasyBuffer* buff, const std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer ser(buff, EasySerializer::Mode::Write);
    ser.head = EasyPacket::HeaderSize();
    bool status = true;
    for (EasySerializeable* obj : peer_cache)
    {
        ser.PutWrite(obj->packetID);
        obj->Serialize(&ser);
    }
    buff->m_payload_size = ser.head - EasyPacket::HeaderSize();
    return status;
}
