//#include "EasyPlayground.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <lua.hpp>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/base64.h>
#include <jdbc/cppconn/driver.h>

#include "EasyBuffer.hpp"
#include "EasySocket.hpp"
#include "EasyIpAddress.hpp"
#include "EasyDB.hpp"

using namespace CryptoPP;
using namespace sql;

#define KEY_SIZE 16U
#define TAG_SIZE 12U
#define IV_SIZE 8U

namespace lua {
    EasySocket srv;

    int Bind(lua_State* L)
    {
        unsigned short port = static_cast<unsigned short>(lua_tointeger(L, 1));
        auto res = srv.bind(port);
        std::cout << "Server Bind Port: " << port << ", Result: " << res << std::endl;
        return 0;
    }

    int Unbind(lua_State* L)
    {
        srv.unbind();
        std::cout << "Server Unbind." << std::endl;
        return 0;
    }

    int Blocking(lua_State* L)
    {
        bool block = lua_toboolean(L, 1) > 0;
        srv.setBlocking(block);
        std::cout << "Server Blocking: " << block << std::endl;
        return 0;
    }

    EasyIpAddress recvIP;
    unsigned short recvPort;
    int Recv(lua_State* L)
    {
        std::byte arr[10U];
        size_t recvBytes;
        auto res = srv.receive(arr, sizeof(arr) / sizeof(std::byte), recvBytes, recvIP, recvPort);
        std::string dataStr; for (int i = 0; i < 10; i++)dataStr += std::to_string((int)arr[i]) + "-";
        std::cout << "Server Recv Result: " << res << ", Bytes: " << recvBytes << ", IP: " << recvIP.toString() << ", Port: " << recvPort << ", Data: " << dataStr << std::endl;
        return 0;
    }

    int Send(lua_State* L)
    {
        std::string dstIP = lua_tostring(L, 1);
        unsigned short dstPort = static_cast<unsigned short>(lua_tointeger(L, 2));
        int sendSize = static_cast<int>(lua_tointeger(L, 3));

        std::vector<std::byte> sendBuffer(sendSize);
        EasyIpAddress ip = EasyIpAddress::resolve(dstIP);
        for (int i = 0; i < 10 && sendSize > i; i++) sendBuffer[i] = std::byte(i);
        auto res = srv.send(sendBuffer.data(), sendBuffer.size(), ip, dstPort);
        std::cout << "Server Send Result: " << res << ", Bytes: " << sendBuffer.size() << ", IP: " << ip.toString() << ", Port: " << dstPort << std::endl;
        return 0;
    }

}

namespace Net {

    typedef uint64_t Addr_t;
    typedef uint32_t SessionID_t;
    typedef uint32_t SequenceID_t;
    typedef std::vector<uint8_t> Payload_t;

    typedef std::vector<uint8_t> IV_t;
    typedef std::vector<uint8_t> Key_t;
    typedef std::pair<Key_t, IV_t> AES_t;

#pragma pack(push, 1)
    struct UDPHeader {
    public:
        SessionID_t sessionID;
        SequenceID_t sequenceID;
        IV_t IV;
    };
#pragma pack(pop)

    struct UDPPacket {
    public:
        UDPHeader header;
        Payload_t payload;
    };

    class MyBuffer {
    public:
        uint8_t* ptr;
        size_t size;

    };

    

    class Peer {
        static inline CryptoPP::AutoSeededRandomPool prng;
    public:
        Addr_t addr;

        SessionID_t session_id;
        SequenceID_t sequence_id;

        GCM<AES>::Encryption enc;
        GCM<AES>::Decryption dec;
        AES_t secret;

        Peer(const Peer& peer) : addr(peer.addr), session_id(peer.session_id), sequence_id(peer.sequence_id), secret(peer.secret), enc(peer.enc), dec(peer.dec)
        {
            
        }

        Peer() : addr(0), session_id(0), sequence_id(0), secret(Key_t(KEY_SIZE), IV_t(IV_SIZE))
        {
            
        }

        void NextSequence()
        {
            ++sequence_id;
            prng.GenerateBlock(secret.second.data(), IV_SIZE);
        }

        bool operator==(const Peer& lhs)
        {
            return lhs.addr == addr;
        }

        struct PeerCompare {
            bool operator()(const Peer& a, const Peer& b) const noexcept
            {
                return a.addr == b.addr;
            }
        };
    };

    class MyPacket {
    private:
        const MyBuffer& m_buffer;

    public:
        MyPacket(const MyBuffer& buffer) : m_buffer(buffer)
        {

        }

        static bool MakeEncrypted(Peer& peer, MyBuffer& buffer)
        {
            try {
                MyPacket packet(buffer);
                if (buffer.size > 0U)
                {
                    // Fill Header
                    memcpy(packet.SessionID(), &peer.session_id, sizeof(SessionID_t));
                    memcpy(packet.SequenceID(), &peer.sequence_id, sizeof(SequenceID_t));
                    memcpy(packet.IV(), peer.secret.second.data(), IV_SIZE);

                    // Set Key
                    peer.enc.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)packet.IV(), IV_SIZE);

                    // GCM Filter
                    AuthenticatedEncryptionFilter aef(peer.enc, new ArraySink(packet.Payload(), packet.PayloadSize() + TAG_SIZE), false, TAG_SIZE);

                    // Fill AAD Channel
                    aef.ChannelPut(AAD_CHANNEL, (const byte*)packet.SessionID(), sizeof(SessionID_t));
                    aef.ChannelPut(AAD_CHANNEL, (const byte*)packet.SequenceID(), sizeof(SequenceID_t));
                    aef.ChannelMessageEnd(AAD_CHANNEL);

                    // Encrypt Payload
                    aef.ChannelPut(DEFAULT_CHANNEL, (const byte*)packet.Payload(), packet.m_buffer.size);
                    aef.ChannelMessageEnd(DEFAULT_CHANNEL);

                    buffer.size += TAG_SIZE;

                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (const Exception& e) {
                return false;
            }
        }

        static bool MakeDecrypted(Peer& peer, MyBuffer& buffer)
        {
            try {
                MyPacket packet(buffer);
                if ((*packet.SessionID() == peer.session_id) && (buffer.size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE))
                {
                    // Set Key
                    peer.dec.SetKeyWithIV(peer.secret.first.data(), KEY_SIZE, (uint8_t*)packet.IV(), IV_SIZE);

                    // GCM Filter
                    AuthenticatedDecryptionFilter adf(peer.dec, new ArraySink(packet.Payload(), packet.PayloadSize() - TAG_SIZE), AuthenticatedDecryptionFilter::DEFAULT_FLAGS, TAG_SIZE);

                    // Fill AAD Channel
                    adf.ChannelPut(AAD_CHANNEL, (const byte*)packet.SessionID(), sizeof(SessionID_t));
                    adf.ChannelPut(AAD_CHANNEL, (const byte*)packet.SequenceID(), sizeof(SequenceID_t));
                    adf.ChannelMessageEnd(AAD_CHANNEL);

                    adf.ChannelPut(DEFAULT_CHANNEL, (const byte*)packet.Payload(), packet.PayloadSize() - sizeof(SessionID_t) - sizeof(SequenceID_t) - IV_SIZE);
                    adf.ChannelMessageEnd(DEFAULT_CHANNEL);

                    if (adf.GetLastResult())
                    {
                        buffer.size = buffer.size - (sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + TAG_SIZE);
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
            catch (const Exception& e) {
                return false;
            }
        }

        inline SessionID_t* SessionID() const {
            return (SessionID_t*)(m_buffer.ptr);
        }

        inline SequenceID_t* SequenceID() const {
            return (SequenceID_t*)(m_buffer.ptr + sizeof(SessionID_t));
        }

        inline IV_t* IV() const {
            return (IV_t*)(m_buffer.ptr + sizeof(SessionID_t) + sizeof(SequenceID_t));
        }

        inline uint8_t* Payload() const {
            return m_buffer.ptr + sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE;
        }

        inline size_t PayloadSize() const {
            return m_buffer.size;
        }
    };

    class Server {
    public:
        using IntermediateBufferPool = std::map<Peer, std::vector<BufferType*>, Peer::PeerCompare>;
        const unsigned short port;

        bool running;

        std::thread receive;
        std::thread send;

        EasySocket sock;

        std::mutex intermediateMutex;

        EasyDB db;

        Server() : running(false), port(port)
        {
            {
                Peer peer;

                // Initialize Sample Data
                peer.session_id = 12U;
                peer.sequence_id = 22U;
                peer.secret.first = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
                peer.secret.second = { 0,1,2,3,4,5,6,7 };

                // Generate IV and Increase Sequence Counter
                peer.NextSequence();

                // Create Buffer
                std::vector<uint8_t> rawBuffer(256U);
                MyBuffer buffer;
                buffer.ptr = rawBuffer.data();
                buffer.size = 0U;

                // Create Packet
                MyPacket packet(buffer);

                // Insert Sample Data
                const size_t samplePlainPayloadLength = 5U;
                for (uint8_t i = 0U; i < samplePlainPayloadLength; ++i)
                    packet.Payload()[i] = (uint8_t)i;

                // Encrypt
                buffer.size = samplePlainPayloadLength; // Raw Payload Size
                bool res1 = MyPacket::MakeEncrypted(peer, buffer);
                auto res2 = packet.Payload();
                auto res3 = packet.PayloadSize();


                buffer.size = sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + samplePlainPayloadLength + TAG_SIZE; // Full Size
                bool res4 = MyPacket::MakeDecrypted(peer, buffer);
                auto res5 = packet.Payload();
                auto res6 = packet.PayloadSize();

                std::cout << "";
            }

        }

        ~Server()
        {
            Stop();
        }

        bool Start(unsigned short port = 54000)
        {
            if (running)
                return false;

            if (sock.bind(port) != WSAEISCONN)
                return false;

            running = true;
            receive = std::thread(&Server::Receive, this);
        }

        void Stop()
        {
            if (!running)
                return;

            running = false;

            if (receive.joinable())
                receive.join();
            if (send.joinable())
                send.join();
        }

        void FlushIntermediate(EasyBufferManager& bufferManager, IntermediateBufferPool& intermediateBuffer)
        {
            for (auto it : intermediateBuffer)
            {
                const Peer& peer = it.first;
                for (BufferType* buff : it.second)
                {



                    bufferManager.Free(buff);
                }
            }
            intermediateBuffer.clear();
        }

        bool ReadKeyFromMYSQL_OR_Peers(const uint64_t& session_id, std::vector<std::uint8_t>& key)
        {
            key.resize(16U);
            memset(key.data(), 0U, key.size());
            memset(key.data(), (int)session_id, sizeof(int));
            return true;
        }

        void Receive()
        {
            const double flushPeriod = 1000.0 / 144.0;
            Peer host;
            uint64_t ret;
            EasyBufferManager bufferManager(100U, 1472U);
            BufferType* buff = bufferManager.Get();
            IntermediateBufferPool intermediateBuffer;
            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now(), lastFlush = std::chrono::high_resolution_clock::now();
            while (running)
            {
                currentTime = std::chrono::high_resolution_clock::now();
                if (buff)
                {
                    ret = sock.receive(buff->data(), buff->size(), buff->ptr, host.addr);
                    if (ret == WSAEISCONN && buff->ptr > 8U + 8U + 0U + 16U)
                    {
                        host.session_id = *((uint64_t*)buff->data());
                        host.sequence_id = *((uint64_t*)(buff->data() + 8U));

                        if (ReadKeyFromMYSQL_OR_Peers(host.session_id, host.secret.second))
                        {
                            BufferType* decryptBuff = bufferManager.Get();
                            if (decryptBuff/* && Decrypt(host, buff, decryptBuff)*/)
                            {
                                if (auto it = intermediateBuffer.find(host); it != intermediateBuffer.end())
                                {
                                    it->second.push_back(decryptBuff);
                                }
                                else
                                {
                                    intermediateBuffer.insert({ host, {decryptBuff} });
                                }
                            }
                        }
                        bufferManager.Free(buff);
                        buff = bufferManager.Get();
                    }
                    else
                    {
                        buff->ptr = 0;
                    }
                }
                else
                {
                    buff = bufferManager.Get();
                }

                double sinceLastFlush = std::chrono::duration<double, std::milli>(currentTime - lastFlush).count();
                if (sinceLastFlush >= flushPeriod && intermediateMutex.try_lock())
                {
                    FlushIntermediate(bufferManager, intermediateBuffer);
                    lastFlush = currentTime;
                    intermediateMutex.unlock();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1U));
            }
        }

        void Send()
        {
            while (running)
            {

                std::this_thread::sleep_for(std::chrono::milliseconds(10U));
            }
        }
    };

}

int main()
{
    Net::Server s;

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_register(L, "Bind", lua::Bind);
    lua_register(L, "Unbind", lua::Unbind);
    lua_register(L, "Blocking", lua::Blocking);
    lua_register(L, "Recv", lua::Recv);
    lua_register(L, "Send", lua::Send);

    std::cout << "Lua Interactive Shell (type 'exit' to quit)\n";
    std::string input;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) break;
        if (input == "exit") break;

        // Run the typed Lua command
        if (luaL_dostring(L, input.c_str()) != LUA_OK) {
            std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1); // remove error message
        }
    }

    lua_close(L);
    std::cout << "Hi";

	//if (EasyDisplay display({1024,768}); display.Init())
	//{
	//	if (EasyPlayground playground(display); playground.Init())
	//	{
	//		while (playground.OneLoop());
	//	}
	//}
	return 0;
}

