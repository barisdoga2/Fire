#pragma once

#define KEY_SIZE 16U
#define TAG_SIZE 12U
#define IV_SIZE 8U
#define SERVER_STATISTICS

#define MAX_SESSIONS ((SessionID_t)(0b00000000'00001111'11111111'11111111))
#define IS_SESSION(x) ((((SessionID_t)x) | ~MAX_SESSIONS) > 0U)

#define LOGIN_RESPONSE ((PacketID_t)(110U))
#define DISCONNECT_RESPONSE ((PacketID_t)(111U))
#define CHAMPION_SELECT_REQUEST ((PacketID_t)(112U))
#define CHAMPION_BUY_REQUEST ((PacketID_t)(113U))
#define CHAMPION_SELECT_RESPONSE ((PacketID_t)(114U))
#define CHAMPION_BUY_RESPONSE ((PacketID_t)(115U))
#define PLAYER_BOOT_INFO ((PacketID_t)(116U))
#define HEARTBEAT ((PacketID_t)(117U))
#define LOGIN_REQUEST ((PacketID_t)(118U))
#define LOGOUT_REQUEST ((PacketID_t)(119U))
#define BROADCAST_MESSAGE ((PacketID_t)(120U))




#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>

typedef uint64_t Addr_t;
typedef uint32_t SessionID_t;
typedef uint32_t UserID_t;
typedef uint32_t SequenceID_t;
typedef uint16_t PacketID_t;
typedef std::chrono::high_resolution_clock::time_point Timestamp_t;
typedef std::chrono::milliseconds Millis_t;
typedef std::chrono::microseconds Micros_t;
typedef std::vector<uint8_t> Payload_t;

typedef std::vector<uint8_t> IV_t;
typedef std::vector<uint8_t> Key_t;
using StatisticsCounter_t = std::atomic<size_t>;

using Clock = std::chrono::high_resolution_clock;




class PeerSocketInfo {
public:
    Addr_t addr;

    PeerSocketInfo() : addr(0U)
    {

    }

    PeerSocketInfo(const PeerSocketInfo& sock) : addr(sock.addr)
    {

    }
};

class PeerCryptInfo {
public:
    SessionID_t session_id;
    Key_t key;
    SequenceID_t sequence_id_in;
    SequenceID_t sequence_id_out;

    PeerCryptInfo(const SessionID_t& session_id, const SequenceID_t& sequence_id_in, const SequenceID_t& sequence_id_out, const Key_t& key) : session_id(session_id), sequence_id_in(sequence_id_in), sequence_id_out(sequence_id_out), key(key)
    {

    }

    PeerCryptInfo(const PeerCryptInfo& crypt) : session_id(crypt.session_id), sequence_id_in(crypt.sequence_id_in), sequence_id_out(crypt.sequence_id_out), key(crypt.key)
    {

    }

};


