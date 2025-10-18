#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"

class EasySerializer;
class EasySerializeable {
public:
    virtual void Serialize(EasySerializer* ser) = 0U;

};

class EasySerializer {
public:
    uint8_t state = 0U;
    size_t head = 0U;
    EasyBuffer* bf;

    EasySerializer(EasyBuffer* bf) : bf(bf)
    {

    }

    void Serialize(EasySerializeable& serializeable)
    {
        state = 1U;
        serializeable.Serialize(this);
    }

    void Deserialize(EasySerializeable& serializeable)
    {
        state = 2U;
        serializeable.Serialize(this);
    }

    template <class T>
    void Put(T& v)
    {
        if constexpr (std::is_base_of_v<EasySerializeable, T>)
        {
            v.Serialize(this);
        }
        else
        {
            if (state == 1U)
            {
                memcpy(bf->begin() + head, &v, sizeof(T));
            }
            else if (state == 2U)
            {
                memcpy((uint8_t*)&v, bf->begin() + head, sizeof(T));
            }
        }
        head += sizeof(T);
    }

    template <class T>
    void Put(std::vector<T>& v)
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        size_t size = v.size();
        Put(size);
        v.resize(size);
        for (T& c : v)
            Put(c);
    }

    void Put(glm::vec2& v)
    {
        Put(v.x);
        Put(v.y);
    }

    void Put(std::string& v)
    {

        size_t size = v.length();
        Put(size);
        v.resize(size);
        for (const char& c : v)
            Put(c);
    }

};


