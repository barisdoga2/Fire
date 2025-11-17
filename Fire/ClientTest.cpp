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
    
}

ClientTest::~ClientTest()
{

}

std::string ClientTest::ClientWebRequest(std::string url, std::string username, std::string password)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return "Curl error:Curl init failed!";

    std::string response;
    std::string postData = "username=" + username + "&password=" + password + "&login=1";
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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::string err = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return "Curl error:Server not reachable!";
    }
    else
    {
        curl_easy_cleanup(curl);
        return response;
    }
}

int ClientTest::ClientResetSendSequenceCounter()
{
    client.crypt->sequence_id_out = 0U;
    return 0;
}

int ClientTest::ClientResetReceiveSequenceCounter()
{
    client.crypt->sequence_id_in = 0U;
    return 0;
}

int ClientTest::ClientSend()
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

int ClientTest::ClientReceive()
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

int ClientTest::ClientBoth()
{
    ClientWebRequest("https://barisdoga.com/index.php", "barisdoga", "123");
    ClientSend();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500U));
    ClientReceive();
    return 0;
}

int ClientTest::ClientSR()
{
    ClientSend();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
    ClientReceive();
    return 0;
}

