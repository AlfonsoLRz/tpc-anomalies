#include "stdafx.h"
#include "ShaderList.h"

// [Static members initialization]

std::unordered_map<uint8_t, std::string> ShaderList::COMP_SHADER_SOURCE{
		{RendEnum::BUILD_REGULAR_GRID_POINT_CLOUD, "Assets/Shaders/Compute/Fracturer/buildRegularGridPointCloud"},
		{RendEnum::LOCATE_THERMAL_ANOMALIES_SHADER, "Assets/Shaders/Compute/Fracturer/locateThermalAnomalies" },
};

std::unordered_map<uint8_t, std::string> ShaderList::REND_SHADER_SOURCE {
		{RendEnum::BLUR_SHADER, "Assets/Shaders/Filters/blur"},
		{RendEnum::BLUR_SSAO_SHADER, "Assets/Shaders/2D/blurSSAOShader"},
		{RendEnum::BVH_SHADER, "Assets/Shaders/Lines/bvh"},
		{RendEnum::CLUSTER_SHADER, "Assets/Shaders/Triangles/clusterShader"},
		{RendEnum::DEBUG_QUAD_SHADER, "Assets/Shaders/Triangles/debugQuad"},
		{RendEnum::MULTI_INSTANCE_SHADOWS_SHADER, "Assets/Shaders/Triangles/multiInstanceShadowsShader"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshPosition"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMeshNormal"},
		{RendEnum::MULTI_INSTANCE_TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/multiInstanceTriangleMesh"},
		{RendEnum::NORMAL_MAP_SHADER, "Assets/Shaders/Filters/normalMapShader"},
		{RendEnum::POINT_CLOUD_SHADER, "Assets/Shaders/Points/pointCloud"},
		{RendEnum::POINT_CLOUD_HEIGHT_SHADER, "Assets/Shaders/Points/pointCloudHeight"},
		{RendEnum::TRIANGLE_MESH_SHADER, "Assets/Shaders/Triangles/triangleMesh"},
		{RendEnum::TRIANGLE_MESH_NORMAL_SHADER, "Assets/Shaders/Triangles/triangleMeshNormal"},
		{RendEnum::TRIANGLE_MESH_POSITION_SHADER, "Assets/Shaders/Triangles/triangleMeshPosition"},
		{RendEnum::WIREFRAME_SHADER, "Assets/Shaders/Lines/wireframe"},
		{RendEnum::SHADOWS_SHADER, "Assets/Shaders/Triangles/shadowsShader"},
		{RendEnum::SSAO_SHADER, "Assets/Shaders/2D/ssaoShader"},
		{RendEnum::VERTEX_NORMAL_SHADER, "Assets/Shaders/Lines/vertexNormal"}
};

std::vector<std::unique_ptr<ComputeShader>> ShaderList::_computeShader (RendEnum::numComputeShaderTypes());
std::vector<std::unique_ptr<RenderingShader>> ShaderList::_renderingShader (RendEnum::numRenderingShaderTypes());

/// [Protected methods]

ShaderList::ShaderList()
{
}

/// [Public methods]

ComputeShader* ShaderList::getComputeShader(const RendEnum::CompShaderTypes shader)
{
	const int shaderID = shader;

	if (!_computeShader[shader].get())
	{
		ComputeShader* shader = new ComputeShader();
		shader->createShaderProgram(COMP_SHADER_SOURCE.at(shaderID).c_str());

		_computeShader[shaderID].reset(shader);
	}

	return _computeShader[shaderID].get();
}

RenderingShader* ShaderList::getRenderingShader(const RendEnum::RendShaderTypes shader)
{
	const int shaderID = shader;

	if (!_renderingShader[shader].get())
	{
		RenderingShader* shader = new RenderingShader();
		shader->createShaderProgram(REND_SHADER_SOURCE.at(shaderID).c_str());

		_renderingShader[shaderID].reset(shader);
	}

	return _renderingShader[shader].get();
}
