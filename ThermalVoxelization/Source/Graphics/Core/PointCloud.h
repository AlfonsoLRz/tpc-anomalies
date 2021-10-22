#pragma once

/**
*	@file PointCloud.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 18/10/2021
*/

#include "Graphics/Core/Model3D.h"

/**
*	@brief Point cloud wrapper for PLY files and its binaries.
*/
class PointCloud: public Model3D
{
protected:
	const static std::string	WRITE_POINT_CLOUD_FOLDER;			//!<

protected:
	std::string			_filename;									//!<
	bool				_useBinary;									//!<

	// Spatial information
	AABB				_aabb;										//!<
	vec3				_offset;									//!<
	std::vector<vec4>	_points;									//!<
	std::vector<vec3>	_rgb;										//!<
	std::vector<float>	_thermal;									//!<

protected:
	/**
	*	@brief Computes a triangle mesh buffer composed only by indices.
	*/
	void computeCloudData();

	/**
	*	@brief Fills the content of model component with binary file data.
	*/
	bool loadModelFromBinaryFile();

	/**
	*	@brief Generates geometry via GPU.
	*/
	bool loadModelFromPLY(const mat4& modelMatrix);

	/**
	*	@brief Loads the PLY point cloud from a binary file, if possible.
	*/
	virtual bool readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp);

	/**
	*	@brief Communicates the model structure to GPU for rendering purposes.
	*/
	virtual void setVAOData();

	/**
	*	@brief Writes the model to a binary file in order to fasten the following executions.
	*	@return Success of writing process.
	*/
	virtual bool writeToBinary(const std::string& filename);

public:
	/**
	*	@brief
	*/
	PointCloud(const std::string& filename, const bool useBinary, const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief
	*/
	virtual ~PointCloud();

	/**
	*	@brief Loads the point cloud, either from a binary or a PLY file.
	*	@param modelMatrix Model transformation matrix.
	*	@return True if the point cloud could be properly loaded.
	*/
	virtual bool load(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Updates the current Axis-Aligned Bounding-Box.
	*/
	void updateBoundaries(const vec3& xyz) { _aabb.update(xyz); }

	// Getters

	/**
	*	@return
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@return Path where the point cloud is saved.
	*/
	std::string getFilename() { return _filename; }

	/**
	*	@brief
	*/
	unsigned getNumberOfPoints() { return unsigned(_points.size()); }

	/**
	*	@return
	*/
	std::vector<vec4>* getPoints() { return &_points; }

	/**
	*	@return Temperature values of each point.
	*/
	std::vector<float>* getTemperature() { return &_thermal; }

	/// Setters

	/**
	*	@brief Sets the offset which translates our local system to UTM.
	*/
	void setOffset(const vec3& offset) { _offset = offset; }
};

