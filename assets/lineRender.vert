#version 330 core

//layout (location = 1) in vec3 vPosition;	// POSITION_INDEX
//in vec3			ciPosition;		// 0

uniform mat4	ciModelViewProjection;
in vec4		ciPosition;
//layout (location=1) in vec3 vPosition;  // POSITION_INDEX

// This is the current velocity of the vertex
//layout (location = 1) in int velocity;			// VELOCITY_INDEX
//layout (location = 2) in int id;			// ID_INDEX



void main(void)
{
	gl_Position = ciModelViewProjection * ciPosition;
}
