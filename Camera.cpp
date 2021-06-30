#include "pch.h"
#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace CEE
{
	Camera::Camera()
	{
		m_RotationAngle = 0.0f;
		m_Translation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_TransformationMatrix = glm::identity<glm::mat4>();
	}

	Camera::~Camera()
	{

	}

	void Camera::Translate(glm::vec3 translation)
	{
		m_Translation += translation;
		glm::mat4 transformation = glm::rotate(glm::identity<glm::mat4>(), m_RotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
		m_TransformationMatrix = glm::translate(glm::identity<glm::mat4>(), m_Translation);
	}

	void Camera::Rotate(float angleinRadians)
	{
		m_RotationAngle += angleinRadians;
		m_TransformationMatrix = glm::rotate(glm::identity<glm::mat4>(), m_RotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
		m_TransformationMatrix *= glm::translate(glm::identity<glm::mat4>(), m_Translation);
	}
}
