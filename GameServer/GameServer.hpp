#pragma once

#include <string>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <EasyUtils.hpp>

#include <iostream>
#include "SqLite3.hpp"
#include "ServerNet.hpp"
#include <BaseServer.hpp>

class ServerConfig
{
public:
    std::string api_ip;
    uint16_t    api_port;
    std::string crt;
    std::string key;
    std::string api;
    std::string api_domain;

    ServerConfig()
    {
        load(GetEXEPath() + "server.config");
    }

private:
    static std::string trim(const std::string& s)
    {
        auto not_space = [](unsigned char c) { return !std::isspace(c); };
        std::string copy = s;
        copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), not_space));
        copy.erase(std::find_if(copy.rbegin(), copy.rend(), not_space).base(), copy.end());
        return copy;
    }

    void load(const std::string& file)
    {
        std::ifstream in(file);
        if (!in)
            throw std::runtime_error("Failed to open config file: " + file);

        std::unordered_map<std::string, std::string> kv;
        std::string line;

        while (std::getline(in, line))
        {
            line = trim(line);

            if (line.empty() || line[0] == '#')
                continue;

            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            kv[key] = value;
        }

        if (kv.count("api_ip"))   api_ip = kv["api_ip"];
        if (kv.count("api_port")) api_port = static_cast<uint16_t>(std::stoi(kv["api_port"]));
        if (kv.count("crt"))      crt = kv["crt"];
        if (kv.count("key"))      key = kv["key"];
        if (kv.count("api"))      api = kv["api"];
        if (kv.count("api_domain"))      api_domain = kv["api_domain"];

        if (api_ip.empty() || api_port == 0 || crt.empty() || key.empty() || api.empty() || api_domain.empty())
        {
            throw std::runtime_error("Missing required configuration keys in " + file);
        }
    }
};

class GameServer : public BaseServer, ServerCallback {
	std::unordered_map<SessionID_t, FireSession*> sessions;
	
    ServerConfig cfg;
	SqLite3 db;

public:
	GameServer();
	~GameServer();

	bool Start(EasyBufferManager* bm);
	void Stop(std::string shutdownMessage = "");

	void ProcessReceived(double dt);
	void Update(double dt) override;

	void BroadcastMessage(std::string broadcastMessage);

	bool OnServerStart() override;
	void OnServerStop(std::string shutdownMessage) override;

	bool OnSessionCreate(const SessionBase& base, EasyBuffer* buffer) override;
	void OnSessionDestroy(const SessionBase& base, std::string disconnectMessage = "") override;
	
	bool OnSessionKeyExpiry(const SessionBase& base) override;
	bool OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase) override;

	void BroadcastWorldState();

	friend class ServerUI;
};

