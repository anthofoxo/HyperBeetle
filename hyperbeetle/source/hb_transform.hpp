#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace hyperbeetle {
	class Transform final {
	public:
		void Reset();
		glm::mat4 Get() const;
		glm::mat4 GetInverse() const;
		void Set(glm::mat4 const& mat);
		void Rotate(float angle, glm::vec3 const& axis);
		Transform Translate(glm::vec3 const& v);
		glm::vec3 GetEuler() const;

		static Transform InterpolateLinear(Transform const& a, Transform const& b, float t);
		static Transform InterpolateQuadratic(Transform const& a, Transform const& b, Transform const& c, float t);
	public:
		glm::vec3 translation = glm::zero<glm::vec3>();
		glm::quat orientation = glm::identity<glm::quat>();
		glm::vec3 scale = glm::one<glm::vec3>();
	};
}