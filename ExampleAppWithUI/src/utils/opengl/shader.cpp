#include "shader.hpp"
#include "program.hpp"

GLShader::GLShader(const char* name, GLenum type) {
	this->_id = 0;
	this->_name = name;
	this->_type = type;
}

void GLShader::create() {
	GL_CALL(this->_id = glCreateShader(this->_type));
}

void GLShader::source(const GLchar* src) {
	GL_CALL(glShaderSource(this->_id, 1, &src, NULL));
}

bool GLShader::compile() {
	GLint status = GL_FALSE;
	GL_CALL(glCompileShader(this->_id));
	GL_CALL(glGetShaderiv(this->_id, GL_COMPILE_STATUS, &status));
	return (status == GL_TRUE);
}

GLsizei GLShader::infolog(GLchar* buff, GLsizei buffsize) {
	GLsizei n = 0;
	GL_CALL(glGetShaderInfoLog(this->_id, buffsize, &n, buff));
	return n;
}

GLShader GLShader::fromSrc(const char* name, const GLchar* src, GLenum type) {
	GLShader shader(name, type);
	shader.create();
	shader.source(src);
	if (!shader.compile()) {
		GLchar buffer[512];
		GLsizei n = shader.infolog(buffer, 512);
		printf("Unable to compile shader \"%s\"\nError: %s\n", name, buffer);
		exit(1);
	}
	return shader;
}
