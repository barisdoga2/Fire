#include "EasyCompression.hpp"
#include "EasyBuffer.hpp"

#include <bzlib.h>



bool EasyCompression::BZ2Compress(uint8_t* in, size_t in_bytes, uint8_t* out, size_t out_capacity, size_t& out_bytes, int level)
{
    bool ret = false;

    unsigned int destLen = (unsigned int)out_capacity;
    int status = BZ2_bzBuffToBuffCompress((char*)out, &destLen, const_cast<char*>((char*)in), (unsigned int)in_bytes, level, 0, 30);
    if (status == BZ_OK)
    {
        out_bytes = destLen;
        ret = true;
    }

    return ret;
}

bool EasyCompression::BZ2Decompress(uint8_t* in, size_t in_bytes, uint8_t* out, size_t out_capacity, size_t& out_bytes)
{
    bool ret = false;

    unsigned int destLen = (unsigned int)out_capacity;
    int status = BZ2_bzBuffToBuffDecompress((char*)out, &destLen, const_cast<char*>((char*)in), (unsigned int)in_bytes, 0, 0);
    if (status == BZ_OK)
    {
        out_bytes = destLen;
        ret = true;
    }

    return ret;
}