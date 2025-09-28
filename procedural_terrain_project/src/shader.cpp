// shader.cpp (top)
#include "shader.h"
#include <iostream>
#include <vector>

// include glad before glfw
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// ... rest of file ...

unsigned int compileShaderProgram(const char* vertexSrc, const char* fragSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);
    int ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[1024]; glGetShaderInfoLog(vs, 1024, nullptr, buf); std::cerr<<"VS compile error: "<<buf<<"\n"; }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[1024]; glGetShaderInfoLog(fs, 1024, nullptr, buf); std::cerr<<"FS compile error: "<<buf<<"\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char buf[1024]; glGetProgramInfoLog(prog, 1024, nullptr, buf); std::cerr<<"Link error: "<<buf<<"\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}
