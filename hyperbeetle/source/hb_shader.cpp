#include "hb_shader.hpp"

#include <glad/gl.h>
#include <memory>

#include <glm/gtc/type_ptr.hpp>

namespace hyperbeetle {
	ShaderProgram::ShaderProgram(unsigned int vert, unsigned int frag) {
		mHandle = glCreateProgram();
		glAttachShader(mHandle, vert);
		glAttachShader(mHandle, frag);
		glLinkProgram(mHandle);
		glDetachShader(mHandle, vert);
		glDetachShader(mHandle, frag);

		int uniformCount = 0;
		glGetProgramiv(mHandle, GL_ACTIVE_UNIFORMS, &uniformCount);

		if (uniformCount != 0)
		{
			GLint maxNameLength = 0;
			glGetProgramiv(mHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
			auto uniform_name = std::make_unique<char[]>(maxNameLength);

			for (GLint i = 0; i < uniformCount; ++i)
			{
				UniformInfo info;

				GLsizei length;
				glGetActiveUniform(mHandle, i, maxNameLength, &length, &info.count, &info.type, uniform_name.get());

				info.location = glGetUniformLocation(mHandle, uniform_name.get());

				mUniforms.emplace(std::make_pair(std::string(uniform_name.get(), length), info));
			}
		}
	}

	void ShaderProgram::Uniform1i(std::string name, int v0) const {
		glUniform1i(GetLocation(name), v0);
	}


	void ShaderProgram::Uniform1f(std::string name, float v0) const {
		glUniform1f(GetLocation(name), v0);
	}

	void ShaderProgram::Uniform2f(std::string name, glm::vec2 const& v0) const {
		glUniform2fv(GetLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::Uniform4f(std::string name, glm::vec4 const& v0) const {
		glUniform4fv(GetLocation(name), 1, glm::value_ptr(v0));
	}

	void ShaderProgram::UniformMat4f(std::string name, glm::mat4 const& v0) const {
		glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(v0));
	}

	void ShaderProgram::Bind() const {
		glUseProgram(mHandle);
	}

	int ShaderProgram::GetLocation(std::string name) const {
		auto it = mUniforms.find(name);
		if (it == mUniforms.end()) return -1;
		return it->second.location;
	}
}