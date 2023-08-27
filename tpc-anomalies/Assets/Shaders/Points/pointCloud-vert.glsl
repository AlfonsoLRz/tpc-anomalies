#version 450

layout (location = 0) in vec4 vPosition;
layout (location = 9) in vec3 vColor;

uniform mat4 mModelViewProj;
uniform float pointSize;		

out vec3 vertColor;

void main() {
	vertColor = vColor;

	gl_PointSize = pointSize;
	gl_Position = mModelViewProj * vPosition;
}