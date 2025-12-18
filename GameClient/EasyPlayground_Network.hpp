
void EasyPlayground::ClearPlayers()
{
	for (auto& p : players)
		delete p;
	players.clear();
}

void EasyPlayground::CreateMainPlayer(ClientNetwork* network, UserID_t uid)
{
	player = new MainPlayer(network, Model("MainCharacter"), uid, glm::vec3(0, 0, 0));
	players.push_back(player);
	delete camera;
	camera = new TPCamera(player, {0,1.72f,0});
}

void EasyPlayground::OnDisconnect()
{
	ChatMessage::Clear();
	for (EasyEntity* e : players)
		delete e;
	players.clear();
	player = nullptr;
}

void EasyPlayground::OnLogin()
{
	ChatMessage::Clear();
	for (EasyEntity* e : players)
		delete e;
	players.clear();
}

void EasyPlayground::ProcesPlayer(const std::vector<sPlayerInfo>& infos, const std::vector<sPlayerState>& states)
{
	for (const sPlayerInfo& info : infos)
	{
		auto it = players.begin();
		while (it != players.end())
			if ((*it)->uid == info.uid)
				break;
			else
				it++;
		if (it == players.end())
		{
			if ((network->session.uid == info.uid))
			{
				std::cout << "[EasyPlayground] ProcesPlayer - Init own player.\n";
				CreateMainPlayer(network, info.uid);
			}
			else
			{
				std::cout << "[EasyPlayground] ProcesPlayer - A new player.\n";
				players.push_back(new Player(network, Model("MainCharacter"), info.uid, false, glm::vec3(0, 0, 0)));
			}
		}
	}

	for (const sPlayerState& state : states)
	{
		auto it = players.begin();
		while (it != players.end())
			if ((*it)->uid == state.uid)
				break;
			else
				it++;
		if (it != players.end())
		{
			if (network->session.uid == state.uid)
			{
				// Self data
			}
			else
			{
				(*it)->transform.position = state.position;
			}
		}
	}
}

void EasyPlayground::NetworkUpdate(double _dt)
{
	network->Update();

	if (!network->IsInGame())
		return;

	// Heartbeat
	{
		static Timestamp_t nextHeartbeat = Clock::now();
		Millis_t heartbeatPeriod(1000U);
		if (Clock::now() >= nextHeartbeat)
		{
			nextHeartbeat = Clock::now() + heartbeatPeriod;

			std::cout << "[EasyPlayground] NetworkUpdate - Heartbeat sent.\n";
			network->GetSendCache().push_back(new sHearbeat());
		}
	}

	// Process Received
	{
		std::vector<EasySerializeable*>& receive_cache = network->GetReceiveCache();
		for (auto it = receive_cache.begin(); it != receive_cache.end(); )
		{
			if (auto* disconnectResponse = dynamic_cast<sDisconnectResponse*>(*it); disconnectResponse)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - Disconnect response received.\n";
				network->Disconnect(disconnectResponse->message);
				break; // Disconnect clears the cache already
				//delete disconnectResponse;  // Disconnect clears the cache already
				//it = receive_cache.erase(it);  // Disconnect clears the cache already
			}
			else if (auto* playerState = dynamic_cast<sPlayerState*>(*it); playerState)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - sPlayerState received.\n";

				// ...

				delete playerState;
				it = receive_cache.erase(it);
			}
			else if (auto* heartbeat = dynamic_cast<sHearbeat*>(*it); heartbeat)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - Heartbeat received.\n";

				delete heartbeat;
				it = receive_cache.erase(it);
			}
			else if (auto* broadcastMessage = dynamic_cast<sBroadcastMessage*>(*it); broadcastMessage)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - Broadcast message received.\n";

				ChatMessage::isBroadcastMessage = true;
				ChatMessage::broadcastMessage = broadcastMessage->message;

				delete broadcastMessage;
				it = receive_cache.erase(it);
			}
			else if (auto* chatMessage = dynamic_cast<sChatMessage*>(*it); chatMessage)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - Chat message received.\n";

				ChatMessage::OnChatMessageReceived(chatMessage->username, chatMessage->message, chatMessage->timestamp);

				delete chatMessage;
				it = receive_cache.erase(it);
			}
			else if (auto* gameBoot = dynamic_cast<sGameBoot*>(*it); gameBoot)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - Game Boot received.\n";

				ProcesPlayer(gameBoot->playerInfo, gameBoot->playerState);

				delete gameBoot;
				it = receive_cache.erase(it);
			}
			else if (auto* worldState = dynamic_cast<sWorldState*>(*it); worldState)
			{
				std::cout << "[EasyPlayground] NetworkUpdate - World state received.\n";

				ProcesPlayer({}, worldState->playerState);

				delete worldState;
				it = receive_cache.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

}
