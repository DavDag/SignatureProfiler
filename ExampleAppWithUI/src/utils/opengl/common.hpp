#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _DEBUG
#define GL_CALL(query) do { query; checkOpenGLError(#query, __FILE__, __LINE__); } while(0);
#else
#define GL_CALL(query) query
#endif // _DEBUG

const char* getGLErrorString(GLenum code);
void checkOpenGLError(const char* query, const char* file, int line);
