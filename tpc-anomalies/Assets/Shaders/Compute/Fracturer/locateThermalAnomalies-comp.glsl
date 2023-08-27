#version 450

#extension GL_ARB_compute_variable_group_size : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_variable) in;

#define MIN 0
#define MAX 2

layout(std430, binding = 0) buffer GridBuffer		{ uint16_t		grid[]; };
layout(std430, binding = 1) buffer ThermalBuffer	{ float			thermalValues[]; };
layout(std430, binding = 2) buffer StatBuffer		{ vec2			stat[]; };
layout(std430, binding = 3) buffer PeakBuffer		{ float			peak[]; };

#include <Assets/Shaders/Compute/Fracturer/voxel.glsl>

uniform int neighbors;
uniform uint numCells;
uniform float stdFactor;

bool isValid(ivec3 xyz)
{
	return xyz.x >= 0 && xyz.x < gridDims.x && xyz.y >= 0 && xyz.y < gridDims.y && xyz.z >= 0 && xyz.z < gridDims.z && grid[getPositionIndex(xyz)] != VOXEL_EMPTY;
}

void main()
{
	const uint index = gl_GlobalInvocationID.x;
	if (index >= gridDims.x * gridDims.y * gridDims.z) return;

	float avg = .0f, std = .0f;
	uint avgNeighbors = 0;
	ivec3 index3 = ivec3(getPosition(index));

	stat[index] = vec2(.0f);
	peak[index] = 0;

	if (grid[index] == VOXEL_EMPTY) return;

	for (int x = -neighbors; x < neighbors; ++x)
	{
		for (int y = -neighbors; y < neighbors; ++y)
		{
			for (int z = -neighbors; z < neighbors; ++z)
			{
				ivec3 neighborIdx = index3 + ivec3(x, y, z);

				if (isValid(neighborIdx))
				{
					avg += thermalValues[getPositionIndex(neighborIdx)];
					++avgNeighbors;
				}
			}
		}
	}

	stat[index].x = avg / float(max(1, avgNeighbors));

	for (int x = -neighbors; x < neighbors; ++x)
	{
		for (int y = -neighbors; y < neighbors; ++y)
		{
			for (int z = -neighbors; z < neighbors; ++z)
			{
				ivec3 neighborIdx = index3 + ivec3(x, y, z);

				if (isValid(neighborIdx))
				{
					std += abs(stat[index].x - thermalValues[getPositionIndex(neighborIdx)]);
				}
			}
		}
	}

	stat[index].y = std / float(max(1, avgNeighbors));

	if (thermalValues[index] >= stat[index].x + stdFactor * stat[index].y) peak[index] = MAX;
	if (thermalValues[index] <= stat[index].x - stdFactor * stat[index].y) peak[index] = MIN;
}