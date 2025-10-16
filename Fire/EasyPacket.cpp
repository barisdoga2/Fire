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
            // Fill Header
            memcpy(SessionID(), &peer.session_id, sizeof(SessionID_t));
            memcpy(SequenceID(), &peer.sequence_id, sizeof(SequenceID_t));
            memcpy(IV(), peer.secret.second.data(), IV_SIZE);

            // Set Key
            peer.enc.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedEncryptionFilter aef(peer.enc, new ArraySink(Payload(), PayloadSize() + TAG_SIZE), false, TAG_SIZE);

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
            return false;
        }
    }
    catch (...) {
        return false;
    }
}

bool EasyPacket::MakeDecrypted(EasyPeer& peer)
{
    try {
        if ((*SessionID() == peer.session_id) && (m_buffer->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE))
        {
            // Set Key
            peer.dec.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedDecryptionFilter adf(peer.dec, new ArraySink(Payload(), PayloadSize() - TAG_SIZE), AuthenticatedDecryptionFilter::DEFAULT_FLAGS, TAG_SIZE);

            // Fill AAD Channel
            adf.ChannelPut(AAD_CHANNEL, (const byte*)SessionID(), sizeof(SessionID_t));
            adf.ChannelPut(AAD_CHANNEL, (const byte*)SequenceID(), sizeof(SequenceID_t));
            adf.ChannelMessageEnd(AAD_CHANNEL);
            auto ss = PayloadSize();
            auto bb = PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE;
            adf.ChannelPut(DEFAULT_CHANNEL, (const byte*)Payload(), PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE);
            adf.ChannelMessageEnd(DEFAULT_CHANNEL);

            if (adf.GetLastResult())
            {
                m_buffer->m_payload_size = m_buffer->m_payload_size - (sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    catch (Exception e) {
        return false;
    }
}