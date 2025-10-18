#pragma once

#include <iostream>
#include <vector>
#include "EasySerializer.hpp"

typedef uint64_t Addr_t;
typedef uint32_t SessionID_t;
typedef uint32_t SequenceID_t;
typedef uint16_t PacketID_t;
typedef std::vector<uint8_t> Payload_t;

typedef std::vector<uint8_t> IV_t;
typedef std::vector<uint8_t> Key_t;
typedef std::pair<Key_t, IV_t> AES_t;

#define KEY_SIZE 16U
#define TAG_SIZE 12U
#define IV_SIZE 8U

class EasyNetObj : public EasySerializeable {
public:
    bool _baseCalled = false;
    PacketID_t packetID;

    EasyNetObj(PacketID_t packetID) : packetID(packetID)
    {

    }

    EasyNetObj(const EasyNetObj& o) : packetID(o.packetID)
    {

    }

    EasyNetObj() : packetID(0xFFFFU)
    {

    }

    ~EasyNetObj()
    {
        assert(_baseCalled && "Derived class forgot to call EasyNetObj::Serialize!");
    }

    virtual void Serialize(EasySerializer* ser)
    {
        _baseCalled = true;
        ser->Put(packetID);
    }



};
