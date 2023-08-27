#include "stdafx.h"
#include "RegularGrid.h"

#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/RenderingParameters.h"
#include "Graphics/Core/OpenGLUtilities.h"
#include "Graphics/Core/ShaderList.h"
#include "tinyply/tinyply.h"

/// Public methods

RegularGrid::RegularGrid(const AABB& aabb, uvec3 subdivisions) :
	_aabb(aabb), _numDivs(subdivisions)
{
	_cellSize = vec3((_aabb.max().x - _aabb.min().x) / float(subdivisions.x), (_aabb.max().y - _aabb.min().y) / float(subdivisions.y), (_aabb.max().z - _aabb.min().z) / float(subdivisions.z));

	this->buildGrid();
}

RegularGrid::RegularGrid(uvec3 subdivisions) : _numDivs(subdivisions)
{
	
}

RegularGrid::~RegularGrid()
{
}

void RegularGrid::exportGrid(bool fillUnderVoxels)
{
	struct VoxelInl
	{
		uvec3	_indices;
		vec3	_scale;
	};

	std::unordered_map<uint16_t, std::vector<VoxelInl>> cubeMap;
	unsigned cellIndex;

	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int z = 0; z < _numDivs.z; ++z)
		{
			if (fillUnderVoxels)
			{
				int y = _numDivs.y - 1, cellContent = 0;
				cellIndex = this->getPositionIndex(x, y, z, _numDivs);

				while (y >= 0 && _grid[cellIndex] == VOXEL_EMPTY)
				{
					cellIndex = this->getPositionIndex(x, y, z, _numDivs);
					--y;
				}

				cellContent = _grid[cellIndex];
				while (y >= 0)
				{
					cubeMap[cellContent].push_back(VoxelInl{ uvec3(x, y, z), vec3(1.0f) });
					--y;
				}
			}
			else
			{
				for (int y = 0; y < _numDivs.y; ++y)
				{
					cellIndex = this->getPositionIndex(x, y, z, _numDivs);
					if (_grid[cellIndex] != VOXEL_EMPTY) cubeMap[_grid[cellIndex]].push_back(VoxelInl{ uvec3(x, y, z), vec3(1.0f) });
				}
			}
		}
	}

	// Cube geometry & topology
	Model3D::ModelComponent* modelComp = Primitives::getCubeModelComponent();
	
	for (auto& pair: cubeMap)
	{
		std::filebuf fileBufferBinary;
		fileBufferBinary.open("Fragments/" + std::to_string(pair.first) + ".ply", std::ios::out | std::ios::binary);

		std::ostream outstreamBinary(&fileBufferBinary);
		if (outstreamBinary.fail()) throw std::runtime_error("Failed to open " + modelComp->_name + ".ply");

		tinyply::PlyFile plyFile;
		std::vector<vec3> position, normal, rgb;
		std::vector<uvec3> triangleMesh;
		vec3 rgbIndex = ColorUtilities::HSVtoRGB(ColorUtilities::getHueValue(pair.first), .99f, .99f);
		
		for (VoxelInl& voxel: pair.second)
		{
			unsigned startIndex = position.size();
			
			for (Model3D::VertexGPUData& vertex: modelComp->_geometry)
			{
				position.push_back(vertex._position * _cellSize * voxel._scale + _aabb.min() + _cellSize * voxel._scale * vec3(voxel._indices.x, voxel._indices.y, voxel._indices.z));
				normal.push_back(vertex._normal);
				rgb.push_back(rgbIndex);
			}

			for (int i = 0; i < modelComp->_triangleMesh.size(); i += 3)
			{
				triangleMesh.push_back(uvec3(modelComp->_triangleMesh[i], modelComp->_triangleMesh[i + 1], modelComp->_triangleMesh[i + 2]) + startIndex);
			}
		}

		plyFile.add_properties_to_element("vertex", { "x", "y", "z" }, tinyply::Type::FLOAT32, position.size(), reinterpret_cast<uint8_t*>(position.data()), tinyply::Type::INVALID, 0);
		plyFile.add_properties_to_element("vertex", { "nx", "ny", "nz" }, tinyply::Type::FLOAT32, normal.size(), reinterpret_cast<uint8_t*>(normal.data()), tinyply::Type::INVALID, 0);
		plyFile.add_properties_to_element("vertex", { "r", "g", "b" }, tinyply::Type::FLOAT32, rgb.size(), reinterpret_cast<uint8_t*>(rgb.data()), tinyply::Type::INVALID, 0);
		plyFile.add_properties_to_element("face", { "vertex_index" }, tinyply::Type::UINT32, triangleMesh.size(), reinterpret_cast<uint8_t*>(triangleMesh.data()), tinyply::Type::UINT8, 3);

		plyFile.write(outstreamBinary, true);
	}

	delete modelComp;
}

void RegularGrid::fill(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, unsigned index, int numSamples)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_REGULAR_GRID);

	// Input data
	uvec3 numDivs		= this->getNumSubdivisions();
	unsigned numCells	= numDivs.x * numDivs.y * numDivs.z;
	unsigned numThreads = faces.size() * numSamples;
	unsigned numGroups	= ComputeShader::getNumGroups(numThreads);

	// Noise buffer
	std::vector<float> noiseBuffer;
	this->fillNoiseBuffer(noiseBuffer, numSamples * 2);

	// Input data
	const GLuint vertexSSBO = ComputeShader::setReadBuffer(vertices, GL_STATIC_DRAW);
	const GLuint faceSSBO	= ComputeShader::setReadBuffer(faces, GL_DYNAMIC_DRAW);
	const GLuint noiseSSBO	= ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);
	const GLuint gridSSBO	= ComputeShader::setReadBuffer(&_grid[0], numCells, GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint>{ vertexSSBO, faceSSBO, noiseSSBO, gridSSBO });
	shader->use();
	shader->setUniform("aabbMin", _aabb.min());
	shader->setUniform("cellSize", _cellSize);
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("numFaces", GLuint(faces.size()));
	shader->setUniform("numSamples", GLuint(numSamples));
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	uint16_t* gridData = ComputeShader::readData(gridSSBO, uint16_t());
	_grid = std::vector<uint16_t>(gridData, gridData + numCells);

	GLuint buffers[] = { vertexSSBO, faceSSBO, noiseSSBO, gridSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::fill(const std::vector<vec4>* vertices, std::vector<float>* thermalValues)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_REGULAR_GRID_POINT_CLOUD);

	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numThreads = vertices->size();
	unsigned numGroups = ComputeShader::getNumGroups(numThreads);

	// Input data
	std::vector<ivec2> thermalAggregation(numCells);
	std::fill(thermalAggregation.begin(), thermalAggregation.end(), ivec2(0));

	const GLuint vertexSSBO = ComputeShader::setReadBuffer(*vertices, GL_STATIC_DRAW);
	const GLuint gridSSBO = ComputeShader::setReadBuffer(&_grid[0], numCells, GL_DYNAMIC_DRAW);
	const GLuint thermalSSBO = ComputeShader::setReadBuffer(*thermalValues, GL_STATIC_DRAW);
	const GLuint thermalAggregationSSBO = ComputeShader::setReadBuffer(thermalAggregation, GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint>{ vertexSSBO, thermalSSBO, gridSSBO, thermalAggregationSSBO });
	shader->use();
	shader->setUniform("aabbMin", _aabb.min());
	shader->setUniform("cellSize", _cellSize);
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("numPoints", GLuint(vertices->size()));
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	uint16_t* gridData = ComputeShader::readData(gridSSBO, uint16_t());
	_grid = std::vector<uint16_t>(gridData, gridData + numCells);

	ivec2* aggregationData = ComputeShader::readData(thermalAggregationSSBO, ivec2());
	for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
	{
		if (aggregationData[cellIdx].y)
		{
			_thermal[cellIdx] = (aggregationData[cellIdx].x / 10000.0f) / aggregationData[cellIdx].y;
		}
	}

	GLuint buffers[] = { vertexSSBO, gridSSBO, thermalSSBO, thermalAggregationSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::fillUnderCloud()
{
	int cellIndex;

	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int z = 0; z < _numDivs.z; ++z)
		{
			int y = _numDivs.y - 1;

			while (y >= 0 && _grid[this->getPositionIndex(x, y, z, _numDivs)] == VOXEL_EMPTY)
			{
				--y;
			}

			if (y >= 0)
			{
				float thermalValue = _thermal[this->getPositionIndex(x, y, z, _numDivs)];
				while (y >= 0)
				{
					cellIndex = this->getPositionIndex(x, y, z, _numDivs);

					_grid[cellIndex] = VOXEL_FREE;
					_thermal[cellIndex] = thermalValue;
					--y;
				}
			}
		}
	}
}

void RegularGrid::fillNoiseBuffer(std::vector<float>& noiseBuffer, unsigned numSamples)
{
	noiseBuffer.resize(numSamples);

	for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
	{
		noiseBuffer[sampleIdx] = RandomUtilities::getUniformRandom(.0f, 1.0f);
	}
}

void RegularGrid::locateAnomalies()
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::LOCATE_THERMAL_ANOMALIES_SHADER);
	RenderingParameters* rendParams = Renderer::getInstance()->getRenderingParameters();

	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numCells = numDivs.x * numDivs.y * numDivs.z;
	unsigned numGroups = ComputeShader::getNumGroups(numCells);

	// Input data
	const GLuint gridSSBO = ComputeShader::setReadBuffer(_grid, GL_STATIC_DRAW);
	const GLuint thermalSSBO = ComputeShader::setReadBuffer(_thermal, GL_STATIC_DRAW);
	const GLuint statSSBO = ComputeShader::setWriteBuffer(vec2(), numCells, GL_DYNAMIC_DRAW);
	const GLuint localPeakSSBO = ComputeShader::setWriteBuffer(float(), numCells, GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint>{ gridSSBO, thermalSSBO, statSSBO, localPeakSSBO });
	shader->use();
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("neighbors", GLint(rendParams->_gridNeighbors));
	shader->setUniform("stdFactor", rendParams->_stdFactor);
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	float* peakData = ComputeShader::readData(localPeakSSBO, float());
	_localPeak = std::vector<float>(peakData, peakData + numCells);

	//for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
	//{
	//	if (_grid[cellIdx] != VOXEL_EMPTY)
	//	{
	//		std::cout << stats[cellIdx].x << " " << stats[cellIdx].y << std::endl;
	//	}
	//}

	GLuint buffers[] = { gridSSBO, thermalSSBO, statSSBO, localPeakSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

void RegularGrid::getAABBs(std::vector<AABB>& aabb)
{
	vec3 max, min;
	
	for (int x = 0; x < _numDivs.x; ++x)
	{
		for (int y = 0; y < _numDivs.y; ++y)
		{
			for (int z = 0; z < _numDivs.z; ++z)
			{
				if (_grid[this->getPositionIndex(x, y, z)] != VOXEL_EMPTY)
				{
					min = _aabb.min() + _cellSize * vec3(x, y, z);
					max = min + _cellSize;

					aabb.push_back(AABB(min, max));
				}
			}
		}
	}
}

void RegularGrid::insertPoint(const vec3& position, unsigned index)
{
	uvec3 gridIndex = getPositionIndex(position);

	_grid[this->getPositionIndex(gridIndex.x, gridIndex.y, gridIndex.z)] = index;
}

void RegularGrid::queryCluster(const std::vector<Model3D::VertexGPUData>& vertices, const std::vector<Model3D::FaceGPUData>& faces, std::vector<float>& clusterIdx)
{
	ComputeShader* shader = ShaderList::getInstance()->getComputeShader(RendEnum::ASSIGN_FACE_CLUSTER);

	// Input data
	uvec3 numDivs = this->getNumSubdivisions();
	unsigned numThreads = vertices.size();
	unsigned numGroups = ComputeShader::getNumGroups(numThreads);

	// Input data
	const GLuint vertexSSBO = ComputeShader::setReadBuffer(vertices, GL_STATIC_DRAW);
	const GLuint faceSSBO = ComputeShader::setReadBuffer(faces, GL_DYNAMIC_DRAW);
	const GLuint gridSSBO = ComputeShader::setReadBuffer(_grid, GL_STATIC_DRAW);
	const GLuint clusterSSBO = ComputeShader::setWriteBuffer(float(), vertices.size(), GL_DYNAMIC_DRAW);

	shader->bindBuffers(std::vector<GLuint>{ vertexSSBO, faceSSBO, gridSSBO, clusterSSBO });
	shader->use();
	shader->setUniform("aabbMin", _aabb.min());
	shader->setUniform("cellSize", _cellSize);
	shader->setUniform("gridDims", numDivs);
	shader->setUniform("numVertices", GLuint(vertices.size()));
	shader->execute(numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	float* clusterData = ComputeShader::readData(clusterSSBO, float());
	clusterIdx = std::vector<float>(clusterData, clusterData + vertices.size());

	GLuint buffers[] = { vertexSSBO, faceSSBO, gridSSBO, clusterSSBO };
	glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);
}

// [Protected methods]

uint16_t* RegularGrid::data()
{
	return _grid.data();
}

uint16_t RegularGrid::at(int x, int y, int z) const
{
	return _grid[this->getPositionIndex(x, y, z)];
}

glm::uvec3 RegularGrid::getNumSubdivisions() const
{
	return _numDivs;
}

void RegularGrid::homogenize()
{
	for (unsigned int x = 0; x < _numDivs.x; ++x)
		for (unsigned int y = 0; y < _numDivs.y; ++y)
			for (unsigned int z = 0; z < _numDivs.z; ++z)
				if (this->at(x, y, z) != VOXEL_EMPTY)
					this->set(x, y, z, VOXEL_FREE);
}

bool RegularGrid::isOccupied(int x, int y, int z) const
{
	return this->at(x, y, z) != VOXEL_EMPTY;
}

bool RegularGrid::isEmpty(int x, int y, int z) const
{
	return this->at(x, y, z) == VOXEL_EMPTY;
}

size_t RegularGrid::length() const
{
	return _numDivs.x * _numDivs.y * _numDivs.z;
}

std::vector<float>* RegularGrid::localPeak()
{
	return &_localPeak;
}

void RegularGrid::set(int x, int y, int z, uint8_t i)
{
	_grid[this->getPositionIndex(x, y, z)] = i;
}

std::vector<float>* RegularGrid::thermalData()
{
	return &_thermal;
}

/// Protected methods	

void RegularGrid::buildGrid()
{	
	unsigned numVoxels = _numDivs.x * _numDivs.y * _numDivs.z;

	_grid = std::vector<uint16_t>(numVoxels);
	_thermal = std::vector<float>(numVoxels);
	std::fill(_grid.begin(), _grid.end(), VOXEL_EMPTY);
	std::fill(_thermal.begin(), _thermal.end(), .0f);
}

uvec3 RegularGrid::getPositionIndex(const vec3& position)
{
	unsigned x = (position.x - _aabb.min().x) / _cellSize.x, y = (position.y - _aabb.min().y) / _cellSize.y, z = (position.z - _aabb.min().z) / _cellSize.z;
	unsigned zeroUnsigned = 0;

	return uvec3(glm::clamp(x, zeroUnsigned, _numDivs.x - 1), glm::clamp(y, zeroUnsigned, _numDivs.y - 1), glm::clamp(z, zeroUnsigned, _numDivs.z - 1));
}

unsigned RegularGrid::getPositionIndex(int x, int y, int z) const
{
	return x * _numDivs.y * _numDivs.z + y * _numDivs.z + z;
}

unsigned RegularGrid::getPositionIndex(int x, int y, int z, const uvec3& numDivs)
{
	return x * numDivs.y * numDivs.z + y * numDivs.z + z;
}
