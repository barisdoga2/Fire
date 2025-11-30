#pragma once

#include <glad/glad.h>

#define GL_DEBUG
#ifdef GL_DEBUG
#define GL(_f)                                                     \
  do {                                                             \
    gl##_f;                                                        \
    GLenum gl_err = glGetError();                                  \
    if (gl_err != 0) {                                             \
      std::cout << "GL error 0x" << std::hex << gl_err			   \
                      << " returned from 'gl" << #_f << std::endl; \
    }                                                              \
    assert(gl_err == GL_NO_ERROR);                                 \
  } while (void(0), 0)
#else
#define GL(_f) gl##_f
#endif