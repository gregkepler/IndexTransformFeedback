#version 330

uniform mat4	ciModelViewProjection;
uniform float	ciElapsedSeconds;
//uniform float	uScale;
//uniform sampler2D uTex0;


in vec4			ciPosition;		// 0
layout (location=1) in vec4 vPosition;  // POSITION_INDEX
//in vec4 vPosition; // POSITION_INDEX
//layout (location=2) in int vIndex;
//layout (location=3) in vec2 vTexCoord;
//layout (location=4) in int vIndexOffset;


uniform samplerBuffer uTex0;	// 1
uniform samplerBuffer uTex1;	// 2

//in vec2 ciTexCoord0;

out vec4 oColor;
//out vec4 oColor2;
out vec2 oTexCoord;

void main( void )
{
//	vec3 pos = vPosition.xyz;
//	oTexCoord = vTexCoord;
	
//	vec3 position = ciPosition + (texture( uTex0, vTexCoord ).xyz );
	vec4 position = vec4(ciPosition.xyz + vPosition.xyz, 1.0);
//	vec3 position = ciPosition + vec3( 0 );
//	vec3 position = ciPosition + texelFetch( uTex0, vIndex * vIndexOffset ).xyz;
	
//	oColor = texture(uTex0, oTexCoord);
//	oColor = texelFetch( uTex0, vIndex );

	gl_Position = ciModelViewProjection * position;
}
