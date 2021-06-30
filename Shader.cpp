#include "pch.h"
#include "Shader.hpp"

#include <fstream>

namespace CEE
{
	Shader::Shader(vk::Device* device)
		: m_Device(device)
	{

	}

	Shader::~Shader()
	{
		m_Device->destroyShaderModule(m_VertexModule, nullptr);
		m_Device->destroyShaderModule(m_FragmentModule, nullptr);
	}

	vk::Result Shader::CompileShadersFromFiles(std::string vertexFilepath, std::string fragmentFilepath)
	{
		std::string vertexSource;
		std::string fragmentSource;

		std::ifstream vertexFile(vertexFilepath, std::ios::binary);
		if (!vertexFile.is_open())
		{
			fprintf(stderr, "Failed to open vertex source!\n");
			return vk::Result::eErrorUnknown;
		}
		size_t fileSize = vertexFile.tellg();
		vertexFile.seekg(0, std::ios::end);
		fileSize = (size_t)vertexFile.tellg() - fileSize;

		vertexFile.seekg(0, std::ios::beg);

		char* fileBuffer = (char*)malloc(fileSize);
		if (!fileBuffer)
			return vk::Result::eErrorUnknown;

		vertexFile.read(fileBuffer, fileSize);

		vertexSource.reserve(fileSize);
		vertexSource.assign(fileBuffer, fileSize);

		vertexFile.close();

		std::ifstream fragmentFile(fragmentFilepath, std::ios::binary);
		if (!fragmentFile.is_open())
		{
			fprintf(stderr, "Failed to open fragment source!\n");
			return vk::Result::eErrorUnknown;
		}

		fileSize = fragmentFile.tellg();
		fragmentFile.seekg(0, std::ios::end);
		fileSize = (size_t)fragmentFile.tellg() - fileSize;

		fragmentFile.seekg(0, std::ios::beg);

		fileBuffer = (char*)realloc(fileBuffer, fileSize);
		if (!fileBuffer)
			return vk::Result::eErrorUnknown;

		fragmentFile.read(fileBuffer, fileSize);

		fragmentSource.reserve(fileSize);
		fragmentSource.assign(fileBuffer, fileSize);

		fragmentFile.close();

		free(fileBuffer);

		return this->CompileShadersFromGLSL(vertexSource, fragmentSource);
	}

	vk::Result Shader::CompileShadersFromGLSL(std::string vertexSource, std::string fragmentSource)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions compilerOptions;
		compilerOptions.SetSourceLanguage(shaderc_source_language_glsl);
		compilerOptions.SetTargetSpirv(shaderc_spirv_version_1_3);
#if defined(_NDEBUG)
		compilerOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
#else
		compilerOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
#endif
		shaderc::CompilationResult<uint32_t> compilationResult;
		// VERTEX
		{
			compilationResult = compiler.CompileGlslToSpv(vertexSource, shaderc_vertex_shader, "Compiled from hardcoded source", compilerOptions);
			if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				if (compilationResult.GetNumErrors() > 0)
				{
					fprintf(stderr, "Failed to compile vertex shader!\n\tError message: %s\n", compilationResult.GetErrorMessage().c_str());
					return vk::Result::eErrorUnknown;
				}
				else
				{
					fprintf(stderr, "Failed to compile vertex shader!\n\tUnknown error!\n");
					return vk::Result::eErrorUnknown;
				}
			}

			auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
				.setCodeSize(4 * ((uint32_t)(compilationResult.end() - compilationResult.begin())))
				.setPCode(compilationResult.begin());

			auto result = m_Device->createShaderModule(&shaderModuleCreateInfo, nullptr, &m_VertexModule);
			if (result != vk::Result::eSuccess)
				return result;
		}
		// FRAGMENT
		{
			compilationResult = compiler.CompileGlslToSpv(fragmentSource, shaderc_fragment_shader, "Compiled from hardcoded source", compilerOptions);
			if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				if (compilationResult.GetNumErrors() > 0)
				{
					fprintf(stderr, "Failed to compile fragment shader!\n\tError message: %s\n", compilationResult.GetErrorMessage().c_str());
					return vk::Result::eErrorUnknown;
				}
				else
				{
					fprintf(stderr, "Failed to compile fragment shader!\n\tUnknown error!\n");
					return vk::Result::eErrorUnknown;
				}
			}

			auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
				.setCodeSize(4 * ((uint32_t)(compilationResult.end() - compilationResult.begin())))
				.setPCode(compilationResult.begin());

			auto result = m_Device->createShaderModule(&shaderModuleCreateInfo, nullptr, &m_FragmentModule);
			if (result != vk::Result::eSuccess)
				return result;
		}
		return vk::Result::eSuccess;
	}
}
