#version 150

//uniform float ciElapsedSeconds;
//uniform float uAlpha;

in vec4 oColor;
//in vec4 oColor2;
in vec2 oTexCoord;

out vec4 fragColor;
uniform sampler2D uTex0;

void main(void)
{
//	vec2 uv = (vTexCoord * 2.0) - 1.0;
//	float mixValue = abs( length( uv ) );
//	mixValue = pow( mixValue, 2 );
	
//	fragColor = mix( oColor1, oColor2, mixValue );
//	fragColor.a *= uAlpha;
	
//	fragColor = vec4( (oColor.rgb ) * vec3(0.01), 1.0);
	fragColor = vec4( 0.0f, 0.5f, 1.0f, 1.0f );
}
