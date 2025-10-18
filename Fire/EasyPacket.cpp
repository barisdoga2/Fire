#include "EasyPacket.hpp"

#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>

using namespace CryptoPP;



EasyPacket::EasyPacket(EasyBuffer* buffer) : m_buffer(buffer)
{

}

bool EasyPacket::MakeEncrypted(EasyPeer& peer)
{
    try {
        if (m_buffer->m_payload_size > 0U)
        {
            AutoSeededRandomPool prng;
            prng.GenerateBlock(peer.secret.second.data(), IV_SIZE);

            ++peer.sequence_id_out;

            // Fill Header
            memcpy(SessionID(), &peer.session_id, sizeof(SessionID_t));
            memcpy(SequenceID(), &peer.sequence_id_out, sizeof(SequenceID_t));
            memcpy(IV(), peer.secret.second.data(), IV_SIZE);

            // Set Key
            GCM<AES>::Encryption enc;
            enc.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedEncryptionFilter aef(enc, new ArraySink(Payload(), PayloadSize() + TAG_SIZE), false, TAG_SIZE);

            // Fill AAD Channel
            aef.ChannelPut(AAD_CHANNEL, (const byte*)SessionID(), sizeof(SessionID_t));
            aef.ChannelPut(AAD_CHANNEL, (const byte*)SequenceID(), sizeof(SequenceID_t));
            aef.ChannelMessageEnd(AAD_CHANNEL);

            // Encrypt Payload
            aef.ChannelPut(DEFAULT_CHANNEL, (const byte*)Payload(), m_buffer->m_payload_size);
            aef.ChannelMessageEnd(DEFAULT_CHANNEL);

            m_buffer->m_payload_size += TAG_SIZE;

            return true;
        }
        else
        {
            std::cout << "Error MakeDecrypted SizeCheck: " << (m_buffer->m_payload_size > 0U) << "\n";
            return false;
        }
    }
    catch (...) {
        std::cout << "Error MakeEncrypted Exception!\n";
        return false;
    }
}

bool EasyPacket::MakeDecrypted(EasyPeer& peer)
{
    try {
        if ((*SessionID() == peer.session_id) && (m_buffer->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE))
        {
            // Set IV
            memcpy(peer.secret.second.data(), IV(), IV_SIZE);

            // Set Key
            GCM<AES>::Decryption dec;
            dec.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedDecryptionFilter adf(dec, new ArraySink(Payload(), PayloadSize() - TAG_SIZE), AuthenticatedDecryptionFilter::DEFAULT_FLAGS, TAG_SIZE);

            // Fill AAD Channel
            adf.ChannelPut(AAD_CHANNEL, (const byte*)SessionID(), sizeof(SessionID_t));
            adf.ChannelPut(AAD_CHANNEL, (const byte*)SequenceID(), sizeof(SequenceID_t));
            adf.ChannelMessageEnd(AAD_CHANNEL);
            auto ss = PayloadSize();
            auto bb = PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE;
            adf.ChannelPut(DEFAULT_CHANNEL, (const byte*)Payload(), PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE);
            adf.ChannelMessageEnd(DEFAULT_CHANNEL);

            if (adf.GetLastResult() && peer.sequence_id_in < *(SequenceID()))
            {
                peer.sequence_id_in = *(SequenceID());
                m_buffer->m_payload_size = m_buffer->m_payload_size - (sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE);
                return true;
            }
            else
            {
                std::cout << "Error MakeDecrypted SequenceInCheck: " << (peer.sequence_id_in < *(SequenceID())) << "\n";
                return false;
            }
        }
        else
        {
            std::cout << "Error MakeDecrypted SessionCheck: " << ((*SessionID() == peer.session_id)) << ", SizeCheck: " << (m_buffer->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE) << "\n";
            return false;
        }
    }
    catch (Exception e) {
        std::cout << "Error MakeDecrypted Exception!\n";
        return false;
    }
}

bool EasyPacket::MakeCompressed(EasyBuffer* out)
{
    EasyPacket destination(out);
    bool ret = EasyCompression::BZ2Compress(Payload(), PayloadSize(), destination.Payload(), out->capacity(), out->m_payload_size);
    if (ret)
    {
        memcpy(out->begin(), this->m_buffer->begin(), HeaderSize());
    }
    else
    {
        std::cout << "Error MakeCompressed!\n";
    }
    return ret;
}

bool EasyPacket::MakeDecompressed(EasyBuffer* out)
{
    EasyPacket destination(out);
    bool ret = EasyCompression::BZ2Decompress(Payload(), PayloadSize(), destination.Payload(), out->capacity(), out->m_payload_size);
    if (ret)
    {
        memcpy(out->begin(), this->m_buffer->begin(), HeaderSize());
    }
    else
    {
        std::cout << "Error MakeDecompressed!\n";
    }
    return ret;
}

// STATIC
bool EasyPacket::MakeEncrypted(EasyPeer& peer, EasyBuffer* buffer)
{
    EasyPacket enc(buffer);
    return enc.MakeEncrypted(peer);
}

bool EasyPacket::MakeDecrypted(EasyPeer& peer, EasyBuffer* buffer)
{
    EasyPacket dec(buffer);
    return dec.MakeDecrypted(peer);
}

bool EasyPacket::MakeCompressed(EasyBuffer* in, EasyBuffer* out)
{
    EasyPacket source(in);
    return source.MakeCompressed(out);
}

bool EasyPacket::MakeDecompressed(EasyBuffer* in, EasyBuffer* out)
{
    EasyPacket source(in);
    return source.MakeDecompressed(out);
}
// END
