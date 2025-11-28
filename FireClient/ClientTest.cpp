#include "ClientTest.hpp"
#include "EasyPacket.hpp"
#include "EasySocket.hpp"
#include "EasyNet.hpp"

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

uint64_t ClientTest::ClientSend(PeerCryptInfo& crypt, std::vector<EasySerializeable*>& objs)
{
    if (objs.size() == 0U)
        return 0U;

    static EasyBuffer* buffer = bf.Get();
    static EasyBuffer* buffer2 = bf.Get();
    uint64_t retVal = 1U;
    if (buffer && buffer2)
    {
        buffer->reset();
        buffer2->reset();

        bool result = true;

        if (result)
            result = MakeSerialized(buffer, objs);

        if (result)
            result = EasyPacket::MakeCompressed(buffer, buffer2);

        if (result)
            result = EasyPacket::MakeEncrypted(crypt, buffer2);

        if (result)
            ++crypt.sequence_id_out;

        if (result)
        {
            auto res = client.socket->send(buffer2->begin(), sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + buffer2->m_payload_size, EasyIpAddress::resolve(ip), port);
            retVal = 1U;
            for (EasySerializeable* obj : objs)
                delete obj;
            objs.clear();
        }
        else
        {
            retVal = 0U;
        }
    }
    else
    {
        retVal = 0U;
    }
    return retVal;
}

uint64_t ClientTest::ClientReceive(PeerCryptInfo& crypt, std::vector<EasySerializeable*>& objs)
{
    static EasyBuffer* buffer = bf.Get();
    static EasyBuffer* buffer2 = bf.Get();
    uint64_t retVal = 1U;
    if (buffer && buffer2)
    {
        EasyIpAddress receiveIP;
        unsigned short receivePort;
        auto res = client.socket->receive(buffer->begin(), buffer->capacity(), buffer->m_payload_size, receiveIP, receivePort);

        bool status = (res == WSAEISCONN);

        if (status)
            status = EasyPacket::MakeDecrypted(crypt, buffer);

        if (status)
            status = EasyPacket::MakeDecompressed(buffer, buffer2);

        if (status)
            status = MakeDeserialized(buffer2, objs);

        if (status)
            retVal = 1U;
        else
            retVal = 0U;
    }
    else
    {
        retVal = 0U;
    }
    return retVal;
}
