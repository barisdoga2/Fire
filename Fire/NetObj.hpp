#pragma once

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/core/traits.h>
#include <glm/vec3.hpp>



namespace glm {
    template <typename S, glm::length_t L, typename T, glm::qualifier Q>
    void serialize(S& s, glm::vec<L, T, Q>& v) {
        for (glm::length_t i = 0; i < L; ++i)
            s.value4b(v[i]);
    }
}

