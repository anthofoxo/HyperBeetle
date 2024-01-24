#pragma once

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

#include <glad/gl.h>

namespace hyperbeetle {

	class ShaderProgram final {
	public:
		struct UniformInfo final {
			GLint location;
			GLsizei count;
			GLenum type;
		};
	public:
		ShaderProgram() = default;
		ShaderProgram(unsigned int vert, unsigned int frag);

		void Uniform1i(std::string name, int v0) const;
		void Uniform1f(std::string name, float v0) const;
		void Uniform2f(std::string name, glm::vec2 const& v0) const;

		void Uniform4f(std::string name, glm::vec4 const& v0) const;
		void UniformMat4f(std::string name, glm::mat4 const& v0) const;

		void Bind() const;
		int GetLocation(std::string name) const;
	private:
		unsigned int mHandle = 0;
		std::unordered_map<std::string, UniformInfo> mUniforms;
	};
}