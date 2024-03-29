#pragma once

/**
*	@file GraphicsCoreEnumerations.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 07/10/2019
*/

/**
*	@brief Contains any enum related to rendering process.
*/
struct RendEnum
{
	/// [Geometry and topology]

	// Topology types
	enum IBOTypes: uint8_t
	{
		IBO_POINT_CLOUD,
		IBO_WIREFRAME,
		IBO_TRIANGLE_MESH
	};

	// Geometry types
	enum VBOTypes : uint8_t
	{
		VBO_POSITION,
		VBO_NORMAL,
		VBO_TEXT_COORD,
		VBO_TANGENT,
		VBO_OFFSET,
		VBO_SCALE,
		VBO_ROTATION,
		VBO_INDEX,
		VBO_CLUSTER_ID,
		VBO_COLOR_01,
		VBO_THERMAL_COLOR,
		VBO_LOCAL_PEAK_COLOR
	};

	/**
	*	@return Number of VBO different types.
	*/
	const static GLsizei numIBOTypes() { return IBO_TRIANGLE_MESH + 1; }

	/**
	*	@return Number of VBO different types.
	*/
	const static GLsizei numVBOTypes() { return VBO_LOCAL_PEAK_COLOR + 1; }

	/// [Shaders]

	// Shader implementations
	enum RendShaderTypes : uint8_t
	{
		// General
		WIREFRAME_SHADER,
		TRIANGLE_MESH_SHADER,
		TRIANGLE_MESH_NORMAL_SHADER,
		TRIANGLE_MESH_POSITION_SHADER,
		MULTI_INSTANCE_SHADOWS_SHADER, 
		MULTI_INSTANCE_TRIANGLE_MESH_POSITION_SHADER,
		MULTI_INSTANCE_TRIANGLE_MESH_NORMAL_SHADER, 
		MULTI_INSTANCE_TRIANGLE_MESH_SHADER,
		SHADOWS_SHADER,

		// Geometry
		VERTEX_NORMAL_SHADER,

		// LiDAR Point cloud
		POINT_CLOUD_SHADER,
		POINT_CLOUD_HEIGHT_SHADER,

		// Data structures
		BVH_SHADER,
		CLUSTER_SHADER,

		// SSAO
		BLUR_SSAO_SHADER,
		SSAO_SHADER,

		// Debug quad
		DEBUG_QUAD_SHADER,

		// Filters
		BLUR_SHADER,
		NORMAL_MAP_SHADER
	};

	enum CompShaderTypes : uint8_t
	{
		// VOXELIZATION
		ASSIGN_FACE_CLUSTER,
		BUILD_REGULAR_GRID,
		BUILD_REGULAR_GRID_POINT_CLOUD,
		LOCATE_THERMAL_ANOMALIES_SHADER,
	};

	/**
	*	@return Number of compute shaders.
	*/
	const static GLsizei numComputeShaderTypes() { return LOCATE_THERMAL_ANOMALIES_SHADER + 1; }

	/**
	*	@return Number of rendering shaders.
	*/
	const static GLsizei numRenderingShaderTypes() { return NORMAL_MAP_SHADER + 1; }

	/// [Rendering parameters]

	// Matrices types
	enum MatricesTypes : uint8_t
	{
		MODEL_MATRIX,
		VIEW_MATRIX,
		PROJ_MATRIX,
		VIEW_PROJ_MATRIX,
		BIAS_VIEW_PROJ_MATRIX
	};

	/**
	*	@return Number of rendering shaders.
	*/
	const static GLsizei numMatricesTypes() { return BIAS_VIEW_PROJ_MATRIX + 1; }
};