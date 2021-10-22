#include "stdafx.h"
#include "PointCloud.h"

#include <filesystem>
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/ShaderList.h"
#include "Graphics/Core/VAO.h"
#include "tinyply.h"

// Initialization of static attributes
const std::string PointCloud::WRITE_POINT_CLOUD_FOLDER = "PointClouds/";

/// Public methods

PointCloud::PointCloud(const std::string& filename, const bool useBinary, const mat4& modelMatrix) :
	Model3D(modelMatrix, 1), _filename(filename), _offset(.0f), _useBinary(useBinary)
{
}

PointCloud::~PointCloud()
{
}

bool PointCloud::load(const mat4& modelMatrix)
{
	if (!_loaded)
	{
		bool success = false, binaryExists = false;

		if (_useBinary && (binaryExists = std::filesystem::exists(_filename + BINARY_EXTENSION)))
		{
			success = this->loadModelFromBinaryFile();
		}

		if (!success)
		{
			success = this->loadModelFromPLY(modelMatrix);
		}

		std::cout << "Number of Points: " << _points.size() << std::endl;

		if (success)
		{
			this->setVAOData();

			if (!binaryExists)
			{
				this->writeToBinary(_filename + BINARY_EXTENSION);
			}
		}

		_loaded = true;

		return true;
	}

	return false;
}

/// [Protected methods]

void PointCloud::computeCloudData()
{
	ModelComponent* modelComp = _modelComp[0];

	// Fill point cloud indices with iota
	modelComp->_pointCloud.resize(_points.size());
	std::iota(modelComp->_pointCloud.begin(), modelComp->_pointCloud.end(), 0);
}

bool PointCloud::loadModelFromBinaryFile()
{
	return this->readBinary(_filename + BINARY_EXTENSION, _modelComp);
}

bool PointCloud::loadModelFromPLY(const mat4& modelMatrix)
{
	std::unique_ptr<std::istream> fileStream;
	std::vector<uint8_t> byteBuffer;
	std::shared_ptr<tinyply::PlyData> plyPoints, plyColors, plyThermals;
	unsigned baseIndex;
	float* pointsRaw;
	uint8_t* colorsRaw, *thermalsRaw;

	try
	{
		const std::string filename = _filename + PLY_EXTENSION;
		bool containsThermal = true;
		fileStream.reset(new std::ifstream(filename, std::ios::binary));

		if (!fileStream || fileStream->fail()) return false;

		fileStream->seekg(0, std::ios::end);
		const float size_mb = fileStream->tellg() * float(1e-6);
		fileStream->seekg(0, std::ios::beg);

		tinyply::PlyFile file;
		file.parse_header(*fileStream);

		try { plyPoints = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
		catch (const std::exception& e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

		try { plyColors = file.request_properties_from_element("vertex", { "red", "green", "blue" }); }
		catch (const std::exception& e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

		//try { plyThermals = file.request_properties_from_element("vertex", { "thermal_red", "thermal_green", "thermal_blue" }); }
		//catch (const std::exception& e) { containsThermal = false; }

		file.read(*fileStream);

		{
			const size_t numPoints = plyPoints->count;
			const size_t numPointsBytes = numPoints * 4 * 3;

			const size_t numColors = plyColors->count;
			const size_t numColorsBytes = numColors * 1 * 3;

			// Allocate space
			_points.resize(numPoints); _rgb.resize(numPoints); _thermal.resize(numPoints);
			pointsRaw = new float[numPoints * 3];
			colorsRaw = new uint8_t[numColors * 3];
			thermalsRaw = (uint8_t*) std::calloc(numPoints * 3, sizeof(uint8_t));

			std::memcpy(pointsRaw, plyPoints->buffer.get(), numPointsBytes);
			std::memcpy(colorsRaw, plyColors->buffer.get(), numColorsBytes);

			if (containsThermal)
			{
				//const size_t numThermalValues = plyThermals->count;
				//const size_t numThermalValuesBytes = numThermalValues * 1 * 3;

				//std::memcpy(thermalsRaw, plyThermals->buffer.get(), numThermalValuesBytes);
			}

			for (unsigned ind = 0; ind < numPoints; ++ind)
			{
				baseIndex = ind * 3;

				_points[ind] = vec4(pointsRaw[baseIndex], pointsRaw[baseIndex + 2], pointsRaw[baseIndex + 1], 1.0f);
				_rgb[ind] = vec3(colorsRaw[baseIndex] / 255.0f, colorsRaw[baseIndex + 1] / 255.0f, colorsRaw[baseIndex + 2] / 255.0f);
				_thermal[ind] = float(colorsRaw[baseIndex] / 255.0f);
				_aabb.update(vec3(_points[ind]));
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Caught tinyply exception: " << e.what() << std::endl;

		return false;
	}

	return true;
}

bool PointCloud::readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		return false;
	}

	size_t numPoints;

	fin.read((char*)&numPoints, sizeof(size_t));
	_points.resize(numPoints); _rgb.resize(numPoints); _thermal.resize(numPoints);
	fin.read((char*)&_points[0], numPoints * sizeof(vec4));
	fin.read((char*)&_rgb[0], numPoints * sizeof(vec3));
	fin.read((char*)&_thermal[0], numPoints * sizeof(float));
	fin.read((char*)&_aabb, sizeof(AABB));

	fin.close();

	return true;
}

void PointCloud::setVAOData()
{
	VAO* vao = new VAO(false);
	ModelComponent* modelComp = _modelComp[0];

	// Refresh point cloud length
	modelComp->_pointCloud.resize(_points.size());
	std::iota(modelComp->_pointCloud.begin(), modelComp->_pointCloud.end(), 0);
	modelComp->_topologyIndicesLength[RendEnum::IBO_POINT_CLOUD] = unsigned(modelComp->_pointCloud.size());

	vao->defineVBO(RendEnum::VBO_COLOR_01, vec3(.0f), GL_FLOAT);
	vao->setVBOData(RendEnum::VBO_POSITION, _points, GL_STATIC_DRAW);
	vao->setVBOData(RendEnum::VBO_COLOR_01, _rgb, GL_STATIC_DRAW);
	vao->setIBOData(RendEnum::IBO_POINT_CLOUD, modelComp->_pointCloud);

	modelComp->_vao = vao;
}

bool PointCloud::writeToBinary(const std::string& filename)
{
	std::ofstream fout(filename, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	const size_t numPoints = _points.size();
	fout.write((char*)&numPoints, sizeof(size_t));
	fout.write((char*)&_points[0], numPoints * sizeof(vec4));
	fout.write((char*)&_rgb[0], numPoints * sizeof(vec3));
	fout.write((char*)&_thermal[0], numPoints * sizeof(float));
	fout.write((char*)&_aabb, sizeof(AABB));

	fout.close();

	return true;
}