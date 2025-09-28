#pragma once

// Include glad here so OpenGL types/decls come from glad, not system headers.
#include <glad/glad.h>

// Do not include <GL/gl.h> or <GLFW/glfw3.h> in this header.
// Keep headers minimal to avoid polluting include order.

#include <string>

unsigned int compileShaderProgram(const char* vertexSrc, const char* fragSrc);
