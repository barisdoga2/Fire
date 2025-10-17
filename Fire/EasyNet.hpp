#pragma once

#include <iostream>
#include <vector>

typedef uint64_t Addr_t;
typedef uint32_t SessionID_t;
typedef uint32_t SequenceID_t;
typedef uint16_t PacketID_t;
typedef std::vector<uint8_t> Payload_t;

typedef std::vector<uint8_t> IV_t;
typedef std::vector<uint8_t> Key_t;
typedef std::pair<Key_t, IV_t> AES_t;

#define KEY_SIZE 16U
#define TAG_SIZE 12U
#define IV_SIZE 8U