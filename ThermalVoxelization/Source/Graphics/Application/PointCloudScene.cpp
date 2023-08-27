#include "stdafx.h"
#include "PointCloudScene.h"

#include <filesystem>
#include <regex>
#include "Geometry/3D/Triangle3D.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/Light.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Utilities/ChronoUtilities.h"

const std::string PointCloudScene::POINT_CLOUD_PATH = "Assets/PointCloud/ThermalPointCloud";

// [Public methods]

PointCloudScene::PointCloudScene() : _aabbRenderer(nullptr), _meshGrid(nullptr), _pointCloud(nullptr)
{
}

PointCloudScene::~PointCloudScene()
{
	delete _aabbRenderer;
	delete _meshGrid;
	delete _pointCloud;
}

void PointCloudScene::exportGrid(bool fillUnderVoxels)
{
	_meshGrid->exportGrid(fillUnderVoxels);
}

void PointCloudScene::rebuildGrid(ivec3 subdivisions)
{
	std::vector<AABB> aabbs;
	std::vector<float> clusterIdx;

	delete _meshGrid;
	_meshGrid = new RegularGrid(_sceneGroup[0]->getAABB(), subdivisions);
	_meshGrid->fill(_pointCloud->getPoints(), _pointCloud->getTemperature());
	if (Renderer::getInstance()->getRenderingParameters()->_fillUnderVoxels) _meshGrid->fillUnderCloud();
	_meshGrid->getAABBs(aabbs);
	_meshGrid->locateAnomalies();

	_aabbRenderer->load(aabbs);
	_aabbRenderer->homogenize();
	_aabbRenderer->setFloatBuffer(_meshGrid->data(), _meshGrid->getNumSubdivisions().x * _meshGrid->getNumSubdivisions().y * _meshGrid->getNumSubdivisions().z, _meshGrid->thermalData(), RendEnum::VBO_THERMAL_COLOR);
	_aabbRenderer->setFloatBuffer(_meshGrid->data(), _meshGrid->getNumSubdivisions().x * _meshGrid->getNumSubdivisions().y * _meshGrid->getNumSubdivisions().z, _meshGrid->localPeak(), RendEnum::VBO_LOCAL_PEAK_COLOR);
}

void PointCloudScene::render(const mat4& mModel, RenderingParameters* rendParams)
{
	SSAOScene::render(mModel, rendParams);
}

// [Protected methods]

void PointCloudScene::correctCameraSystem(Camera* camera, AABB aabb)
{
	camera->setPosition(aabb.center() + aabb.extent() * 1.5f);
	camera->setLookAt(aabb.center());
}

bool PointCloudScene::isExtensionReadable(const std::string& filename)
{
	return filename.find(OBJ_EXTENSION) != std::string::npos;
}

void PointCloudScene::loadDefaultCamera(Camera* camera)
{
	AABB aabb = this->getAABB();

	camera->setLookAt(aabb.center());
	camera->setPosition(aabb.center() + aabb.extent() * 1.5f);
}

void PointCloudScene::loadDefaultLights()
{
	Light* pointLight_01 = new Light();
	pointLight_01->setLightType(Light::POINT_LIGHT);
	pointLight_01->setPosition(vec3(1.64f, 2.0f, -0.12f));
	pointLight_01->setId(vec3(0.35f));
	pointLight_01->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_01));

	Light* pointLight_02 = new Light();
	pointLight_02->setLightType(Light::POINT_LIGHT);
	pointLight_02->setPosition(vec3(-2.86f, 2.0f, -0.13f));
	pointLight_02->setId(vec3(0.35f));
	pointLight_02->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(pointLight_02));

	Light* sunLight = new Light();
	Camera* camera = sunLight->getCamera();
	ShadowMap* shadowMap = sunLight->getShadowMap();
	camera->setBottomLeftCorner(vec2(-7.0f, -7.0f));
	shadowMap->modifySize(4096, 4096);
	sunLight->setLightType(Light::DIRECTIONAL_LIGHT);
	sunLight->setPosition(vec3(.0f, 3.0f, -5.0f));
	sunLight->setDirection(vec3(-0.1, -0.8f, 1.0f));
	sunLight->setId(vec3(0.5f));
	sunLight->setIs(vec3(0.0f));
	sunLight->castShadows(true);
	sunLight->setShadowIntensity(0.0f, 1.0f);
	sunLight->setBlurFilterSize(5);

	_lights.push_back(std::unique_ptr<Light>(sunLight));

	Light* fillLight = new Light();
	fillLight->setLightType(Light::DIRECTIONAL_LIGHT);
	fillLight->setDirection(vec3(-1.0f, 1.0f, 0.0f));
	fillLight->setId(vec3(0.1f));
	fillLight->setIs(vec3(0.0f));

	_lights.push_back(std::unique_ptr<Light>(fillLight));

	Light* rimLight = new Light();
	rimLight->setLightType(Light::RIM_LIGHT);
	rimLight->setIa(vec3(0.015f, 0.015f, 0.05f));

	_lights.push_back(std::unique_ptr<Light>(rimLight));
}

void PointCloudScene::loadCameras()
{
	ivec2 canvasSize = _window->getSize();
	Camera* camera = new Camera(canvasSize[0], canvasSize[1]);
	_cameraManager->insertCamera(camera);
}

void PointCloudScene::loadLights()
{
	this->loadDefaultLights();
}

void PointCloudScene::loadModels()
{
	{
		_pointCloud = new PointCloud(POINT_CLOUD_PATH, true);
		_pointCloud->load();

		Group3D* group = new Group3D();
		group->addComponent(_pointCloud, _pointCloud->getAABB());
		group->registerScene();
		_sceneGroup.push_back(group);

		// Build octree and retrieve AABBs
		_aabbRenderer = new AABBSet();
		_aabbRenderer->load();
		_aabbRenderer->setMaterial(MaterialList::getInstance()->getMaterial(CGAppEnum::MATERIAL_CAD_BLUE));
		
		this->correctCameraSystem(_cameraManager->getActiveCamera(), _pointCloud->getAABB());
		this->rebuildGrid(Renderer::getInstance()->getRenderingParameters()->_gridSubdivisions);
	}
}

// [Rendering]

void PointCloudScene::drawSceneAsPoints(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	_pointCloud->drawAsPoints(shader, shaderType, *matrix);
}

void PointCloudScene::drawSceneAsLines(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
}

void PointCloudScene::drawSceneAsTriangles(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_SHADER)
	{
	}
	else if (shaderType == RendEnum::CLUSTER_SHADER)
	{
		if (!rendParams->_renderVoxelizedMesh)
		{
			for (Group3D* group : _sceneGroup)
			{
				group->drawAsTriangles(shader, shaderType, *matrix);
			}
		}
	}
	else
	{
		if (rendParams->_renderVoxelizedMesh)
		{
			shader->setUniform("outlierColouring", GLuint(rendParams->_renderAnomalies));
			shader->setUniform("thermalColouring", GLuint(rendParams->_renderThermals));
			_aabbRenderer->drawAsTriangles(shader, shaderType, *matrix);
		}
	}
}

void PointCloudScene::drawSceneAsTriangles4Normal(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_NORMAL_SHADER)
	{
		if (!rendParams->_renderVoxelizedMesh)
		{
			for (Group3D* group : _sceneGroup)
				group->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
	else
	{
		if (rendParams->_renderVoxelizedMesh)
			_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
	}
}

void PointCloudScene::drawSceneAsTriangles4Position(RenderingShader* shader, RendEnum::RendShaderTypes shaderType, std::vector<mat4>* matrix, RenderingParameters* rendParams)
{
	if (shaderType == RendEnum::TRIANGLE_MESH_POSITION_SHADER || shaderType == RendEnum::SHADOWS_SHADER)
	{
		if (!rendParams->_renderVoxelizedMesh)
		{
			for (Group3D* group : _sceneGroup)
				group->drawAsTriangles4Shadows(shader, shaderType, *matrix);
		}
	}
	else
	{
		if (rendParams->_renderVoxelizedMesh)
			_aabbRenderer->drawAsTriangles4Shadows(shader, shaderType, *matrix);
	}
}