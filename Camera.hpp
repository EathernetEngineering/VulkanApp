#ifndef _CAMERA_HPP
#define _CAMERA_HPP

#include <glm/glm.hpp>

namespace CEE
{
	class Camera
	{
	public:
		Camera();
		~Camera();

		inline glm::mat4 GetTransformationMatrix() const { return m_TransformationMatrix; }

		inline glm::vec3 GetTranslation() const { return m_Translation; }
		inline float GetRotation() const { return m_RotationAngle; }

		void Translate(glm::vec3 translation);
		void Rotate(float angleInRadians);

	private:
		float m_RotationAngle;
		glm::vec3 m_Translation;
		glm::mat4 m_TransformationMatrix;
	};
}

#endif
