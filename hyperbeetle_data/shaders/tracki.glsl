in vec3 inPosition = 0;
in vec3 inNormal = 2;
in vec2 inTexCoord = 3;

varying vec2 vTexCoord;
varying vec3 vNormal;
varying vec3 vFromCam;
varying vec3 vRawNormal;

out vec4 outColor = 0;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 ws0;
uniform mat4 ws1;
uniform mat4 ws2;
uniform mat4 ws3;

vec4 quadCurve(vec4 a, vec4 b, vec4 c, float t) {
	vec4 p0 = mix(a, b, t);
	vec4 p1 = mix(b, c, t);
	return mix(p0, p1, t);
}

vec4 cubicCurve(vec4 a, vec4 b, vec4 c, vec4 d, float t) {
	vec4 p0 = quadCurve(a, b, c, t);
	vec4 p1 = quadCurve(b, c, d, t);
	return mix(p0, p1, t);
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

[[vert]] void main(void) {
	vec4 pos = vec4(inPosition.xyz, 1.0);
	float morphVal = -pos.z;
	pos.z = 0;
	//pos.y = 0;

	vec4 t0 = ws0 * pos;
	vec4 t1 = ws1 * pos;
	vec4 t2 = ws2 * pos;
	vec4 t3 = ws3 * pos;
		
	pos = cubicCurve(t0, t1, t2, t3, morphVal);

	gl_Position = uProjection * uView * pos;

	vec3 normal0 = mat3(ws0) * inNormal;
	vec3 normal1 = mat3(ws1) * inNormal;
	vec3 normal2 = mat3(ws2) * inNormal;
	vec3 normal3 = mat3(ws3) * inNormal;

	vNormal = cubicCurve(vec4(normal0, 0.0), vec4(normal1, 0.0), vec4(normal2, 0.0),vec4(normal3, 0.0), morphVal).xyz;
	vRawNormal = inNormal;

	vFromCam = (inverse(uView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - pos.xyz;

}

uniform samplerCube uSkybox;

[[frag]] void main(void) {
	vec3 normFromCam = -normalize(vFromCam);
	vec3 normNormal = normalize(vNormal);

	vec3 reflectedDirection = reflect(normFromCam, normNormal);

	vec3 color = texture(uSkybox, reflectedDirection).rgb;
	color *= max(0.5, dot(normalize(vRawNormal), vec3(0.0, 1.0, 0.0)));

	color.b = mix(color.b, 1.0, 0.2);

	outColor = vec4(color, 1.0);
	outColor.b = mix(outColor.b, 1.0, 0.3);
}