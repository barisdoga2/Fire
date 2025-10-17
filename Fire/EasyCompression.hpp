#pragma once

#include <iostream>

class EasyBuffer;
class EasyCompression {
public:
    static bool BZ2Compress(uint8_t* in, size_t in_bytes, uint8_t* out, size_t out_capacity, size_t& out_bytes, int level = 9);
    static bool BZ2Decompress(uint8_t* in, size_t in_bytes, uint8_t* out, size_t out_capacity, size_t& out_bytes);

};