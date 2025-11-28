#include "pch.h"
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
#include <bzlib.h>


using namespace CryptoPP;



EasyPacket::EasyPacket(EasyBuffer* buffer) : m_buffer(buffer)
{

}

bool EasyPacket::MakeEncrypted(const PeerCryptInfo& crypt, EasyBuffer* buffer)
{
    EasyPacket source(buffer);
    try 
    {
        if (buffer->m_payload_size > 0U)
        {
            static IV_t iv(IV_SIZE);
            AutoSeededRandomPool prng;
            prng.GenerateBlock(iv.data(), IV_SIZE);

            // Fill Header
            memcpy(source.SessionID(), &crypt.session_id, sizeof(SessionID_t));
            memcpy(source.SequenceID(), &crypt.sequence_id_out, sizeof(SequenceID_t));

            // Set Key
            GCM<AES>::Encryption enc;
            enc.SetKeyWithIV(crypt.key.data(), KEY_SIZE, (uint8_t*)source.IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedEncryptionFilter aef(enc, new ArraySink(source.Payload(), source.PayloadSize() + TAG_SIZE), false, TAG_SIZE);

            // Fill AAD Channel
            aef.ChannelPut(AAD_CHANNEL, (const byte*)source.SessionID(), sizeof(SessionID_t));
            aef.ChannelPut(AAD_CHANNEL, (const byte*)source.SequenceID(), sizeof(SequenceID_t));
            aef.ChannelMessageEnd(AAD_CHANNEL);

            // Payload
            aef.ChannelPut(DEFAULT_CHANNEL, (const byte*)source.Payload(), buffer->m_payload_size);
            aef.ChannelMessageEnd(DEFAULT_CHANNEL);

            buffer->m_payload_size += TAG_SIZE;

            return true;
        }
        else
        {
            std::cout << "Error MakeDecrypted SizeCheck: " << (buffer->m_payload_size > 0U) << "\n";
            return false;
        }
    }
    catch (...) 
    {
        std::cout << "Error MakeEncrypted Exception!\n";
        return false;
    }
}

bool EasyPacket::MakeDecrypted(const PeerCryptInfo& crypt, EasyBuffer* buffer)
{
    EasyPacket source(buffer);
    try 
    {
        if ((*source.SessionID() == crypt.session_id) && (buffer->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE))
        {
            // Set Key
            GCM<AES>::Decryption dec;
            dec.SetKeyWithIV(crypt.key.data(), KEY_SIZE, (uint8_t*)source.IV(), IV_SIZE);

            // GCM Filter
            AuthenticatedDecryptionFilter adf(dec, new ArraySink(source.Payload(), source.PayloadSize() - TAG_SIZE), AuthenticatedDecryptionFilter::DEFAULT_FLAGS, TAG_SIZE);

            // Fill AAD Channel
            adf.ChannelPut(AAD_CHANNEL, (const byte*)source.SessionID(), sizeof(SessionID_t));
            adf.ChannelPut(AAD_CHANNEL, (const byte*)source.SequenceID(), sizeof(SequenceID_t));
            adf.ChannelMessageEnd(AAD_CHANNEL);

            // Payload
            adf.ChannelPut(DEFAULT_CHANNEL, (const byte*)source.Payload(), source.PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE);
            adf.ChannelMessageEnd(DEFAULT_CHANNEL);

            if (adf.GetLastResult()/* && crypt.sequence_id_in < *(SequenceID())*/)
            {
                buffer->m_payload_size = buffer->m_payload_size - (sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE);
                return true;
            }
            else
            {
                std::cout << "Error MakeDecrypted SequenceInCheck: " /*<< (crypt.sequence_id_in <= *(SequenceID()))*/ << "\n";
                return false;
            }
        }
        else
        {
            std::cout << "Error MakeDecrypted SessionCheck: " << ((*source.SessionID() == crypt.session_id)) << ", SizeCheck: " << (buffer->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE) << "\n";
            return false;
        }
    }
    catch (Exception e) 
    {
        std::cout << "Error MakeDecrypted Exception!\n";
        return false;
    }
}

bool EasyPacket::MakeCompressed(EasyBuffer* in, EasyBuffer* out)
{
    bool ret = false;

    EasyPacket rawPacket(in);
    EasyPacket compressedPacket(out);

    unsigned int compressedSize = static_cast<unsigned int>(in->capacity());
    int result = BZ2_bzBuffToBuffCompress(
        reinterpret_cast<char*>(compressedPacket.Payload()), &compressedSize,
        reinterpret_cast<char*>(rawPacket.Payload()), static_cast<unsigned int>(in->m_payload_size),
        9, 0, 30
    );

    if (result == BZ_OK) 
    {
        memcpy(out->begin(), in->begin(), HeaderSize());
        out->m_payload_size = compressedSize;
        ret = true;
    }
    else
    {
        std::cerr << "Error BZ2Compress | Compression failed: " << result << "\n";
    }

    return ret;
}

bool EasyPacket::MakeDecompressed(EasyBuffer* in, EasyBuffer* out)
{
    bool ret = false;

    EasyPacket decompressedPacket(out);
    EasyPacket compressedPacket(in);

    unsigned int decompressedSize = static_cast<unsigned int>(in->capacity());
    int result = BZ2_bzBuffToBuffDecompress(
        reinterpret_cast<char*>(decompressedPacket.Payload()), &decompressedSize,
        reinterpret_cast<char*>(compressedPacket.Payload()), static_cast<unsigned int>(in->m_payload_size),
        0, 0
    );

    if (result == BZ_OK) 
    {
        memcpy(out->begin(), in->begin(), HeaderSize());
        out->m_payload_size = decompressedSize;
        ret = true;
    }
    else
    {
        std::cerr << "Error BZ2Decompress | Decompression failed: " << result << "\n";
    }

    return ret;
}
