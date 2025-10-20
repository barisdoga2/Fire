#pragma once


#define KEY_SIZE 16U
#define TAG_SIZE 12U
#define IV_SIZE 8U

#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>

typedef uint64_t Addr_t;
typedef uint32_t SessionID_t;
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



class AES_t {
public:
    Key_t key{};
    IV_t iv{};

    AES_t() : key(KEY_SIZE), iv(IV_SIZE)
    {

    }

    AES_t(const AES_t& aes) : key(aes.key), iv(aes.iv)
    {

    }
};

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
    const SessionID_t session_id;
    const Key_t key;
    SequenceID_t sequence_id_in;
    SequenceID_t sequence_id_out;

    PeerCryptInfo(const SessionID_t& session_id, const SequenceID_t& sequence_id_in, const SequenceID_t& sequence_id_out, const Key_t& key) : session_id(session_id), sequence_id_in(sequence_id_in), sequence_id_out(sequence_id_out), key(key)
    {

    }

    PeerCryptInfo(const PeerCryptInfo& crypt) : session_id(crypt.session_id), sequence_id_in(crypt.sequence_id_in), sequence_id_out(crypt.sequence_id_out), key(crypt.key)
    {

    }

};

class PeerSessionInfo {
public:
    PeerSocketInfo sock;
    PeerCryptInfo crypt;
    Timestamp_t lastReceive;

    PeerSessionInfo(const PeerSessionInfo& info) : sock(info.sock), crypt(info.crypt), lastReceive(info.lastReceive)
    {

    }

    bool Alive(const Timestamp_t& ts)
    {
        return lastReceive + Millis_t(10000U) > ts;
    }
};

class PeerSession {
public:
    PeerSessionInfo info;

    PeerSession() = delete;

    PeerSession(const PeerSessionInfo& info) : info(info)
    {

    }

    bool Alive(const Timestamp_t& ts)
    {
        return info.Alive(ts);
    }
};

class PeerInfo {
public:
    Addr_t addr;
    SessionID_t sessionID;

    PeerInfo(const PeerInfo& info) : addr(info.addr), sessionID(info.sessionID)
    {

    }

    PeerInfo() : addr(0U), sessionID(0U)
    {

    }
};

