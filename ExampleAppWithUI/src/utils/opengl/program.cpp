#include "program.hpp"
#include "shader.hpp"

GLProgram::GLProgram(const char* name) {
	this->_id = 0;
	this->_name = name;
}

void GLProgram::create() {
	GL_CALL(this->_id = glCreateProgram());
}

void GLProgram::attach(const GLShader& shader) {
	GL_CALL(glAttachShader(this->_id, shader.id()));
}

bool GLProgram::link() {
	GLint status = GL_FALSE;
	GL_CALL(glLinkProgram(this->_id));
	GL_CALL(glGetProgramiv(this->_id, GL_LINK_STATUS, &status));
	return (status == GL_TRUE);
}

GLsizei GLProgram::infolog(GLchar* buff, GLsizei buffsize) {
	GLsizei n = 0;
	GL_CALL(glGetProgramInfoLog(this->_id, buffsize, &n, buff));
	return n;
}

GLProgram GLProgram::fromShaders(const char* name, const GLShader& vertex, const GLShader& fragment) {
	GLProgram program(name);
	program.create();
	program.attach(vertex);
	program.attach(fragment);
	if (!program.link()) {
		GLchar buffer[512];
		GLsizei n = program.infolog(buffer, 512);
		printf("Unable to link program \"%s\"\nError: %s\n", name, buffer);
		exit(1);
	}
	return program;
}

void GLProgram::bind() {
	GL_CALL(glUseProgram(this->_id));
}

void GLProgram::unbind() {
	GL_CALL(glUseProgram(0));
}

void GLProgram::uniform1u(GLUniformName name, unsigned int value) {
	GLint loc = 0;
	GL_CALL(loc = glGetUniformLocation(this->_id, name));
	GL_CALL(glUniform1ui(loc, value));
}

void GLProgram::uniform1i(GLUniformName name, int value) {
	GLint loc = 0;
	GL_CALL(loc = glGetUniformLocation(this->_id, name));
	GL_CALL(glUniform1i(loc, value));
}

void GLProgram::uniform1f(GLUniformName name, float value) {
	GLint loc = 0;
	GL_CALL(loc = glGetUniformLocation(this->_id, name));
	GL_CALL(glUniform1f(loc, value));
}

void GLProgram::uniformVec3f(GLUniformName name, const glm::vec3& value) {
	GLint loc = 0;
	GL_CALL(loc = glGetUniformLocation(this->_id, name));
	GL_CALL(glUniform3fv(loc, 1, &value[0]));
}

void GLProgram::uniformMat4f(GLUniformName name, const glm::mat4& value) {
	GLint loc = 0;
	GL_CALL(loc = glGetUniformLocation(this->_id, name));
	GL_CALL(glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]));
}
