#include "ClientTest.hpp"
#include "EasyPacket.hpp"
#include "EasySocket.hpp"
#include "Serializer.hpp"
#include "Net.hpp"

#include <curl/curl.h>

ClientPeer::ClientPeer(std::string key) : user_id(0U), socket(new EasySocket()), EasyPeer(), crypt(nullptr)
{

}

ClientPeer::~ClientPeer()
{
    if (crypt)
        delete crypt;
    delete socket;
}

ClientTest::ClientTest(EasyBufferManager& bf, std::string ip, unsigned short port) : bf(bf), ip(ip), port(port)
{
    ClientTest::instance = this;
}

ClientTest::~ClientTest()
{

}

int ClientTest::ClientWebRequest(lua_State* L)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Failed to initialize CURL\n";
        return 1;
    }

    std::string response;
    std::string url = "https://barisdoga.com/index.php";
    std::string postData = "username=barisdoga&password=123&login=1";
    std::string prefix = "<textarea name=\"jwt\" readonly>";
    std::string prefix2 = "<div class=\"msg\">";
    auto writeLambda = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t
        {
            auto* out = static_cast<std::string*>(userdata);
            out->append(ptr, size * nmemb);
            return size * nmemb;
        };

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeLambda);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::cout << "Error! Login failed! CURL error: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
        if (size_t index = response.find(prefix); index != std::string::npos)
        {
            response = response.substr(index + prefix.length());
            response = response.substr(0, response.find("</textarea>"));
            SessionID_t sessionID = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
            response = response.substr(response.find_first_of(":") + 1U);
            uint32_t userID = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
            response = response.substr(response.find_first_of(":") + 1U);
            std::string key = response;

            Key_t key_t(KEY_SIZE);
            memcpy(key_t.data(), key.data(), KEY_SIZE);
            client.crypt = new PeerCryptInfo(sessionID, 0U, 0U, key_t);

            std::cout << "Login OK! SessionID: " << sessionID << ", UserID: " << userID << ", Key: " << key << std::endl;
        }
        else
        {
            if (size_t index = response.find(prefix2); index != std::string::npos)
            {
                response = response.substr(index + prefix2.length());
                response = response.substr(0, response.find("</div>"));
                std::cout << "Error! Login failed! Message: " << response << std::endl;
            }
            else
            {
                std::cout << "Error! Login response parse failed!" << std::endl;
            }
        }
    }

    curl_easy_cleanup(curl);
    return 0;
}

int ClientTest::ClientResetSendSequenceCounter(lua_State* L)
{
    client.crypt->sequence_id_out = 0U;
    return 0;
}

int ClientTest::ClientResetReceiveSequenceCounter(lua_State* L)
{
    client.crypt->sequence_id_in = 0U;
    return 0;
}

int ClientTest::ClientSend(lua_State* L)
{
    EasyBuffer* buff = bf.Get();
    EasyBuffer* buff2 = bf.Get();

    std::string str = "Hi! I'm client.";
    if (buff && buff2)
    {
        bool result = true;
        buff->reset();
        buff2->reset();

        // Sample Payload
        std::vector<EasySerializeable*> objs;
        static int i = 0;
        objs.emplace_back(new pHello(str + std::to_string(++i)));

        if (result)
            result = MakeSerialized(buff, objs);

        // Prepare Packet
        if (result)
            result = EasyPacket::MakeCompressed(buff, buff2);

        if (!client.crypt)
        {
            std::string key_str = "fbdRjmU4rvENzNCy";
            Key_t key(KEY_SIZE);
            memcpy(key.data(), key_str.data(), key_str.size());
            client.crypt = new PeerCryptInfo(2U, 0U, 0U, key);
        }

        if (result)
            result = EasyPacket::MakeEncrypted(*client.crypt, buff2);

        if (result)
            ++client.crypt->sequence_id_out;

        // Send Packet
        if (result)
        {
            auto res = client.socket->send(buff2->begin(), sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + buff2->m_payload_size, EasyIpAddress::resolve(ip), port);
            std::cout << "Client Packet Send! Result: " << res << std::endl;
        }
        else
        {
            std::cout << "Client Packet Send! Error while packet create! " << std::endl;
        }
    }
    bf.Free(buff);
    bf.Free(buff2);
    return 0;
}

int ClientTest::ClientReceive(lua_State* L)
{
    EasyBuffer* buff = bf.Get();
    EasyBuffer* buff2 = bf.Get();
    if (buff && buff2)
    {
        EasyIpAddress receiveIP;
        unsigned short receivePort;
        auto res = client.socket->receive(buff->begin(), buff->capacity(), buff->m_payload_size, receiveIP, receivePort);

        std::vector<EasySerializeable*> objs;

        bool status = (res == WSAEISCONN);

        if (status)
            status = EasyPacket::MakeDecrypted(*client.crypt, buff);

        if (status)
            status = EasyPacket::MakeDecompressed(buff, buff2);

        if (status)
            status = MakeDeserialized(buff2, objs);

        if (status)
        {
            for (EasySerializeable* o : objs)
                if (o->packetID == HELLO)
                    std::cout << "Client Received Hello Packet. Message: " << (*(pHello*)o).message << "\n";
        }
        else
        {
            std::cout << "Client Error While Processing Received Packet!" << std::endl;
        }
    }
    bf.Free(buff);
    bf.Free(buff2);
    return 0;
}

int ClientTest::ClientBoth(lua_State* L)
{
    ClientWebRequest(L);
    ClientSend(L);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500U));
    ClientReceive(L);
    return 0;
}

int ClientTest::ClientSR(lua_State* L)
{
    ClientSend(L);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
    ClientReceive(L);
    return 0;
}

int ClientTest::ClientWebRequestS(lua_State* L) { return instance->ClientWebRequest(L); }
int ClientTest::ClientResetSendSequenceCounterS(lua_State* L) { return instance->ClientResetSendSequenceCounter(L); }
int ClientTest::ClientResetReceiveSequenceCounterS(lua_State* L) { return instance->ClientResetReceiveSequenceCounter(L); }
int ClientTest::ClientSendS(lua_State* L) { return instance->ClientSend(L); }
int ClientTest::ClientReceiveS(lua_State* L) { return instance->ClientReceive(L); }
int ClientTest::ClientBothS(lua_State* L) { return instance->ClientBoth(L); }
int ClientTest::ClientSRS(lua_State* L) { return instance->ClientSR(L); }