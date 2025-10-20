#include "Serializer.hpp"

#include <vector>
#include "EasyNet.hpp"
#include "EasyPacket.hpp"
#include "World.hpp"


bool MakeDeserialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer des(buff);
    des.head = EasyPacket::HeaderSize();
    bool status = true;
    while (des.head < buff->m_payload_size)
    {
        PacketID_t packetID = *(PacketID_t*)(buff->begin() + des.head);
        switch (packetID)
        {
        case WORLD_DEF:
        {
            b2::pWorld* v = new b2::pWorld();
            des.Deserialize(*v);
            peer_cache.push_back(v);
            break;
        }

        case MATERIAL_DEF:
        {
            b2::pMaterial* v = new b2::pMaterial();
            des.Deserialize(*v);
            peer_cache.push_back(v);
            break;
        }

        case CIRCLE_DEF:
        {
            b2::pCircleShape* v = new b2::pCircleShape();
            des.Deserialize(*v);
            peer_cache.push_back(v);
            break;
        }

        case POLY_DEF:
        {
            b2::pPolyShape* v = new b2::pPolyShape();
            des.Deserialize(*v);
            peer_cache.push_back(v);
            break;
        }

        case HELLO:
        {
            pHello* v = new pHello("");
            des.Deserialize(*v);
            peer_cache.push_back(v);
            break;
        }

        default:
        {
            status = false;
            break;
        }
        }
    }
    if (!status)
        std::cout << "Error MakeDeserialized! Unknown Packet ID!" << "\n";
    return status;
}

bool MakeSerialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache)
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
