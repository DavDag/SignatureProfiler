#pragma once

#include "common.hpp"

#include <unordered_map>

class GLShader;

typedef const char* GLUniformName;

class GLProgram {
public:
	GLProgram(): GLProgram(nullptr) { }
	GLProgram(const char* name);
	//
	GLuint id() const { return this->_id; }
	const char* name() const { return this->_name; }
	//
	void create();
	void attach(const GLShader& shader);
	bool link();
	GLsizei infolog(GLchar* buff, GLsizei buffsize);
	//
	void bind();
	void unbind();
	//
	void uniform1u(GLUniformName name, unsigned int value);
	void uniform1i(GLUniformName name, int value);
	void uniform1f(GLUniformName name, float value);
	void uniformVec3f(GLUniformName name, const glm::vec3& value);
	void uniformMat4f(GLUniformName name, const glm::mat4& value);

public:
	static GLProgram fromShaders(const char* name, const GLShader& vertex, const GLShader& fragment);

private:
	GLuint _id;
	const char* _name;
	std::unordered_map<GLUniformName, GLuint> _uniforms;
};
