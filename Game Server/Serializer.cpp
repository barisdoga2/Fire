#include "Serializer.hpp"

#include <vector>
#include "../Fire/EasyNet.hpp"
#include "../Fire/EasyPacket.hpp"
#include "World.hpp"


bool MakeDeserialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache)
{
    EasySerializer des(buff);
    des.head = EasyPacket::HeaderSize();
    bool status = true;
    while (des.head < buff->m_payload_size + EasyPacket::HeaderSize() && status)
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

            case LOGIN_RESPONSE:
            {
                sLoginResponse* v = new sLoginResponse(false, "");
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case DISCONNECT_RESPONSE:
            {
                sDisconnectResponse* v = new sDisconnectResponse("");
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case CHAMPION_SELECT_REQUEST:
            {
                sChampionSelectRequest* v = new sChampionSelectRequest(0U);
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case CHAMPION_BUY_REQUEST:
            {
                sChampionBuyRequest* v = new sChampionBuyRequest(0U);
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case CHAMPION_SELECT_RESPONSE:
            {
                sChampionSelectResponse* v = new sChampionSelectResponse(false, "");
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case CHAMPION_BUY_RESPONSE:
            {
                sChampionBuyResponse* v = new sChampionBuyResponse(false, "");
                des.Deserialize(*v);
                peer_cache.push_back(v);
                break;
            }

            case PLAYER_BOOT_INFO:
            {
                sPlayerBootInfo* v = new sPlayerBootInfo(0U, 0U, 0U, 0U, false, { });
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

bool MakeSerialized(EasyBuffer* buff, const std::vector<EasySerializeable*>& peer_cache)
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
