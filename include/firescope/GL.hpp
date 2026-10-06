#pragma once
#include <cstddef>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <stdexcept>

// Load only the graphics entry points we use. ImGui carries its own loader.
#ifndef APIENTRY
#define APIENTRY
#endif
namespace firescope::gl {
#define FS_GL(ret, name, ...) inline ret (APIENTRY *name)(__VA_ARGS__) = nullptr
FS_GL(unsigned, CreateShader, unsigned);
FS_GL(void, ShaderSource, unsigned, int, const char* const*, const int*);
FS_GL(void, CompileShader, unsigned);
FS_GL(void, GetShaderiv, unsigned, unsigned, int*);
FS_GL(void, GetShaderInfoLog, unsigned, int, int*, char*);
FS_GL(unsigned, CreateProgram, void);
FS_GL(void, AttachShader, unsigned, unsigned);
FS_GL(void, LinkProgram, unsigned);
FS_GL(void, GetProgramiv, unsigned, unsigned, int*);
FS_GL(void, GetProgramInfoLog, unsigned, int, int*, char*);
FS_GL(void, DeleteShader, unsigned);
FS_GL(void, DeleteProgram, unsigned);
FS_GL(void, UseProgram, unsigned);
FS_GL(void, GenVertexArrays, int, unsigned*);
FS_GL(void, BindVertexArray, unsigned);
FS_GL(void, DeleteVertexArrays, int, const unsigned*);
FS_GL(void, GenBuffers, int, unsigned*);
FS_GL(void, BindBuffer, unsigned, unsigned);
FS_GL(void, BufferData, unsigned, ptrdiff_t, const void*, unsigned);
FS_GL(void, DeleteBuffers, int, const unsigned*);
FS_GL(void, VertexAttribPointer, unsigned, int, unsigned, unsigned char, int, const void*);
FS_GL(void, EnableVertexAttribArray, unsigned);
FS_GL(int, GetUniformLocation, unsigned, const char*);
FS_GL(void, UniformMatrix4fv, int, int, unsigned char, const float*);
FS_GL(void, Uniform1i, int, int);
FS_GL(void, Uniform2f, int, float, float);
FS_GL(void, Uniform3f, int, float, float, float);
FS_GL(void, ActiveTexture, unsigned);
#undef FS_GL
inline void load() {
#define FS_LOAD(name) name = reinterpret_cast<decltype(name)>(glfwGetProcAddress("gl" #name)); if (!name) throw std::runtime_error("Missing OpenGL function: gl" #name)
    FS_LOAD(CreateShader); FS_LOAD(ShaderSource); FS_LOAD(CompileShader);
    FS_LOAD(GetShaderiv); FS_LOAD(GetShaderInfoLog); FS_LOAD(CreateProgram);
    FS_LOAD(AttachShader); FS_LOAD(LinkProgram); FS_LOAD(GetProgramiv);
    FS_LOAD(GetProgramInfoLog); FS_LOAD(DeleteShader); FS_LOAD(DeleteProgram);
    FS_LOAD(UseProgram); FS_LOAD(GenVertexArrays); FS_LOAD(BindVertexArray);
    FS_LOAD(DeleteVertexArrays); FS_LOAD(GenBuffers); FS_LOAD(BindBuffer);
    FS_LOAD(BufferData); FS_LOAD(DeleteBuffers); FS_LOAD(VertexAttribPointer);
    FS_LOAD(EnableVertexAttribArray); FS_LOAD(GetUniformLocation);
    FS_LOAD(UniformMatrix4fv); FS_LOAD(Uniform1i); FS_LOAD(ActiveTexture);
    FS_LOAD(Uniform2f); FS_LOAD(Uniform3f);
#undef FS_LOAD
}
}
