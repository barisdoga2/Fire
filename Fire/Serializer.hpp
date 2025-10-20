#pragma once

#include <vector>

class EasyBuffer;
class EasySerializeable;
bool MakeDeserialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache);
bool MakeSerialized(EasyBuffer* buff, std::vector<EasySerializeable*>& peer_cache);

