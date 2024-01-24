#include <common>

in vec3 inPosition = 0;

varying vec3 vTexCoord;

out vec4 outColor = 0;

uniform mat4 uView;
uniform mat4 uProjection;

uniform samplerCube uReflectionSampler;
uniform sampler2D uNoiseSampler;

uniform float uTime;

float rand(float n){return fract(sin(n) * 43758.5453123);}

[[vert]] void main(void) {
	vTexCoord = inPosition;
	gl_Position = uProjection * vec4( mat3(uView) * inPosition, 1.0);
	gl_Position.z = gl_Position.w;
}

[[frag]] void main(void) {
	vec3 reflectionSample = texture(uReflectionSampler, vTexCoord).xyz;
	outColor = vec4(reflectionSample, 1.0);
}