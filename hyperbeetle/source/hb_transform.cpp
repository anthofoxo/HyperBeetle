#include "hb_transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace hyperbeetle {
	void Transform::Reset() {
		translation = glm::zero<glm::vec3>();
		orientation = glm::identity<glm::quat>();
		scale = glm::one<glm::vec3>();
	}

	glm::mat4 Transform::Get() const {
		glm::vec3 skew = glm::zero<glm::vec3>();
		glm::vec4 perspective(0.0f, 0.0f, 0.0f, 1.0f);
		return glm::recompose(scale, orientation, translation, skew, perspective);

		// Older method, kept for searchability
		// glm::mat4 mat = glm::identity<glm::mat4>();
		// mat = glm::translate(mat, translation);
		// mat *= glm::toMat4(orientation);
		// mat = glm::scale(mat, scale);
		// return mat;
	}

	glm::mat4 Transform::GetInverse() const {
		return glm::inverse(Get());
	}

	void Transform::Set(glm::mat4 const& mat) {
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(mat, scale, orientation, translation, skew, perspective);
	}

	void Transform::Rotate(float angle, glm::vec3 const& axis) {
		orientation = glm::rotate(orientation, angle, axis);
	}

	Transform Transform::Translate(glm::vec3 const& v) {
		Set(glm::translate(Get(), v));
		return *this;
	}

	glm::vec3 Transform::GetEuler() const {
		return glm::degrees(glm::eulerAngles(orientation));
	}

	Transform Transform::InterpolateLinear(Transform const& a, Transform const& b, float t) {
		Transform transform;
		transform.translation = glm::mix(a.translation, b.translation, t);
		transform.orientation = glm::slerp(a.orientation, b.orientation, t);
		transform.scale = glm::mix(a.scale, b.scale, t);
		return transform;
	}

	Transform Transform::InterpolateQuadratic(Transform const& a, Transform const& b, Transform const& c, float t) {
		return InterpolateLinear(InterpolateLinear(a, b, t), InterpolateLinear(b, c, t), t);
	}

	Transform Transform::InterpolateCubic(Transform const& a, Transform const& b, Transform const& c, Transform const& d, float t) {
		return InterpolateLinear(InterpolateQuadratic(a, b, c, t), InterpolateQuadratic(b, c, d, t), t);
	}
}