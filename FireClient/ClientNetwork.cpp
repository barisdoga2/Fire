#include "ClientNetwork.hpp"

#include <curl/curl.h>


bool ClientNetwork::Auth(std::string url)
{
    static const std::string prefix = "<textarea name=\"jwt\" readonly>";
    static const std::string prefix2 = "<div class=\"msg\">";

    session.sck = {};
    session.sid = 0U;
    session.uid = 0U;
    session.addr = 0U;
    session.key = Key_t(KEY_SIZE);
    session.key_expiry = {};
    session.seqid_in = 0U;
    session.seqid_out = 0U;
    session.lastReceive = {};
    session.receiveCache = {};
    session.sendCache = {};
    session.stats = {};

    loginStatus = "Authenticationg...";

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        loginStatus = "Curl init failed!";
        return false;
    }

    std::string response;
    std::string postData = std::string("username=").append(session.username.c_str()).append("&password=").append(session.password.c_str()).append("&login=1");
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
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, &response, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::string err = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        loginStatus = "Server not reachable!";
        return false;
    }
    else
    {
        curl_easy_cleanup(curl);
    }

    if (size_t index = response.find(prefix); index != std::string::npos)
    {
        loginStatus = "Waiting the game server...";

        response = response.substr(index + prefix.length());
        response = response.substr(0, response.find("</textarea>"));

        session.sid = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        session.uid = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        std::string key = response;

        session.key.reserve(KEY_SIZE);
        std::memcpy(session.key.data(), key.data(), KEY_SIZE);
    }
    else
    {
        if (size_t index = response.find(prefix2); index != std::string::npos)
        {
            response = response.substr(index + prefix2.length());
            response = response.substr(0, response.find("</div>"));
            loginStatus = response;
        }
        else
        {
            loginStatus = "Error while parsing response!";
        }
        return false;
    }
    return true;
}