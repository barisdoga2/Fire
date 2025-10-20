#pragma once

#include <vector>

class EasyBuffer;
class EasyNetObj;
bool MakeDeserialized(EasyBuffer* buff, std::vector<EasyNetObj*>& peer_cache);
bool MakeSerialized(EasyBuffer* buff, std::vector<EasyNetObj*>& peer_cache);

