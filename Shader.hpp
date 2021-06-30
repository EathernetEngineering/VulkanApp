#ifndef _SHADER_HPP
#define _SHADER_HPP

#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>

namespace CEE
{
	class Shader
	{
	public:
		Shader(vk::Device* device);
		~Shader();

		vk::Result CompileShadersFromFiles(std::string vertexFilepath, std::string fragmentFilepath);
		vk::Result CompileShadersFromGLSL(std::string vertexSource, std::string fragmentSource);

		vk::ShaderModule GetVertexModule() const { return m_VertexModule; }
		vk::ShaderModule GetFragmentModule() const { return m_FragmentModule; }

	private:
		vk::Device* m_Device;

		vk::ShaderModule m_VertexModule;
		vk::ShaderModule m_FragmentModule;
	};
}

#endif
