#version 330 core

layout (location = 0) in int vIndex;

uniform int uParticleCount;
uniform int uStride;		// 16
uniform float uMaxDist;		// 100 or so


// This is a TBO that will be bound to the same buffer as the
uniform samplerBuffer tex_position;

uniform float ciElapsedSeconds;

// The outputs of the vertex shader are the same as the inputs
out int tf_index;


void main(void)
{
	int matchIndex = -1;
	int id = int(floor(float(vIndex) / float(uStride)));
	bool even = mod(id, 2) == 0;
	
	if( even ){
		matchIndex = id;
	}else{
	
		int pointIndex = vIndex % uStride;
		vec3 myPos = texelFetch( tex_position, id ).xyz;
		
		int rangeCounter = 0;
		for( int i = 0; i < uParticleCount; i++){
			vec3 comparitorPos = texelFetch( tex_position, i ).xyz;
			float dist = distance( myPos, comparitorPos);
			
			if( dist < uMaxDist ) {
				rangeCounter++;
			}
			
			if( rangeCounter == pointIndex ){
				matchIndex = i;
				break;
			}
		}
	}
	
	
	// correct output
//	tf_index = matchIndex;
	
	
	// debug output
	if( even )
		tf_index = 0;
	else
		tf_index = 100;
	
}
