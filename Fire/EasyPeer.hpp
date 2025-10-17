#pragma once

#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"
#include "EasyIpAddress.hpp"



class EasyPeer {
public:
    Addr_t addr;
    EasyIpAddress ip;
    unsigned short port;
    std::vector<uint8_t> sockAddr;

    SessionID_t session_id;
    SequenceID_t sequence_id_in;
    SequenceID_t sequence_id_out;

    CryptoPP::GCM<CryptoPP::AES>::Encryption enc;
    CryptoPP::GCM<CryptoPP::AES>::Decryption dec;
    AES_t secret;

    std::vector<EasyBuffer*> receiveBuffer;

    EasyPeer(const EasyPeer& peer)
        : addr(peer.addr), session_id(peer.session_id), sequence_id_in(peer.sequence_id_in), sequence_id_out(peer.sequence_id_out),
        secret(peer.secret), enc(peer.enc), dec(peer.dec), sockAddr(peer.sockAddr), ip(peer.ip), port(peer.port)
    {

    }

    EasyPeer() : addr(0), session_id(0), sequence_id_in(0), sequence_id_out(0), secret(Key_t(KEY_SIZE), IV_t(IV_SIZE)), sockAddr(16U), ip(0), port(0)
    {

    }

    ~EasyPeer()
    {
        
    }

    bool operator<(const EasyPeer& b) const noexcept
    {
        return addr < b.addr;
    }

};