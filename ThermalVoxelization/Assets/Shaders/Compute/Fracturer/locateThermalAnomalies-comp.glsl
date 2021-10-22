#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

layout(std430, binding = 0) buffer GridBuffer		{ uint16_t		grid[]; };
layout(std430, binding = 1) buffer ThermalBuffer	{ float			thermalValues[]; };
layout(std430, binding = 2) buffer StatBuffer		{ vec2			stat[]; };
layout(std430, binding = 3) buffer PeakBuffer		{ uint			peak[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform vec3 aabbMin;

void main()
{
	const uvec3 index = gl_GlobalInvocationID.xyz;

	stat[0] = index.zy;

	//vec3 point = points[index].xyz;
	//int thermalValue = floatBitsToInt(thermalValues[index]);
	//uvec3 gridIndex = getPositionIndex(point);
	//uint gridIndexUint = getPositionIndex(gridIndex);

	//grid[gridIndexUint] = VOXEL_FREE;
	//atomicAdd(aggregation[gridIndexUint].x, int(thermalValues[index] * 10000.0f));
	//atomicAdd(aggregation[gridIndexUint].y, 1);
}