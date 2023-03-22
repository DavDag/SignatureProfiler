#pragma once

#include "common.hpp"

class GLProgram;

class GLShader {
public:
	GLShader(const char* name, GLenum type);
	//
	GLuint id() const { return this->_id; }
	const char* name() const { return this->_name; }
	GLenum type() const { return this->_type; }
	//
	void create();
	void source(const GLchar* src);
	bool compile();
	GLsizei infolog(GLchar* buff, GLsizei buffsize);

public:
	static GLShader fromSrc(const char* name, const GLchar* src, GLenum type);

private:
	GLuint _id;
	const char* _name;
	GLenum _type;
};
