#include "common.hpp"

#include <stdio.h>
#include <stdlib.h>

#define CASE_STR(value) case value: return #value;
const char* getGLErrorString(GLenum code) {
	switch (code) {
		CASE_STR(GL_INVALID_ENUM)
			CASE_STR(GL_INVALID_VALUE)
			CASE_STR(GL_INVALID_OPERATION)
			CASE_STR(GL_INVALID_FRAMEBUFFER_OPERATION)
			CASE_STR(GL_OUT_OF_MEMORY)
			CASE_STR(GL_STACK_UNDERFLOW)
			CASE_STR(GL_STACK_OVERFLOW)
			CASE_STR(GL_NO_ERROR)
	default: return "<unknown>";
	}
}
#undef CASE_STR

void checkOpenGLError(const char* query, const char* file, int line) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		printf("OpenGL error %s (%08x)\nAt %s:%i\n\"%s\"\n", getGLErrorString(error), error, file, line, query);
		exit(error);
	}
}