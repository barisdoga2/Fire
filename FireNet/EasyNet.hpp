#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>

// Time
typedef std::chrono::high_resolution_clock  Clock;
typedef std::chrono::system_clock           SysClock;
typedef Clock::time_point                   Timestamp_t;
typedef std::chrono::milliseconds           Millis_t;
#define SLEEP_MS(x)                         (std::this_thread::sleep_for(std::chrono::milliseconds((x))))

// Framework
typedef uint64_t                  FireSignature_t;
#define FIRE_SIGNATURE            ((FireSignature_t)0x01234567ULL)

// Packet
typedef uint16_t                  PacketID_t;

// Encryption
#define KEY_SIZE                   16U
#define TAG_SIZE                   12U
#define IV_SIZE                    8U
typedef std::vector<uint8_t>       Payload_t;
typedef std::vector<uint8_t>       IV_t;
typedef std::vector<uint8_t>       Key_t;
typedef SysClock::time_point       Key_Expt_t;
typedef uint32_t                   SequenceID_t;

// Session
#define USERNAME_LEGTH             16U
#define PASSWORD_LEGTH             16U
typedef uint32_t                   SessionID_t;
typedef uint64_t                   Addr_t;
typedef uint32_t                   UserID_t;
#define MAX_SESSIONS_1M            ((SessionID_t)(0b00000000'00010000'00000000'00000000)) // 1,048,576
#define MAX_SESSIONS_64            ((SessionID_t)(0b00000000'00000000'00000000'01000000)) // 64
#define MAX_SESSIONS               (MAX_SESSIONS_64)
#define IS_SESSION(x)              ((((SessionID_t)x) | ~MAX_SESSIONS) > 0U)

// Crypt
class CryptData {
public:
    SessionID_t session_id;
    Key_t key;
    SequenceID_t sequence_id_in;
    SequenceID_t sequence_id_out;
	CryptData(const SessionID_t& session_id, const SequenceID_t& sequence_id_in, const SequenceID_t& sequence_id_out, const Key_t& key) : session_id(session_id), sequence_id_in(sequence_id_in), sequence_id_out(sequence_id_out), key(key)
    { }
	CryptData(const CryptData& crypt) : session_id(crypt.session_id), sequence_id_in(crypt.sequence_id_in), sequence_id_out(crypt.sequence_id_out), key(crypt.key)
    { }
};

enum SessionStatus {
    UNSET,
    CONNECTING,
    CONNECTED,
    ADDR_MISMATCH,
    CRYPT_ERR,
    TIMED_OUT,
    SERVER_TIMED_OUT,
    SEQUENCE_MISMATCH,
    RECONNECTED,
    CLIENT_LOGGED_OUT
};

inline static std::string SessionStatus_Str(const SessionStatus& status)
{
    std::string str;
    if (status == UNSET)
        str = "UNSET";
    else if (status == CONNECTING)
        str = "CONNECTING";
    else if (status == CONNECTED)
        str = "CONNECTED";
    else if (status == ADDR_MISMATCH)
        str = "Connected from another client!";
    else if (status == CRYPT_ERR)
        str = "CRYPT_ERR";
    else if (status == TIMED_OUT)
        str = "TIMED_OUT";
    else if (status == SERVER_TIMED_OUT)
        str = "SERVER_TIMED_OUT";
    else if (status == SEQUENCE_MISMATCH)
        str = "SEQUENCE_MISMATCH";
    else if (status == RECONNECTED)
        str = "RECONNECTED";
    else if (status == CLIENT_LOGGED_OUT)
        str = "CLIENT_LOGGED_OUT";
    else
        str = "UNKNOWN";
    return str;
}
