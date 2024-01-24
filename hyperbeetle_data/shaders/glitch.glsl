const vec2 kPositions[4] = vec2[]( vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0));

uniform float shake_power;// = 0.03;
uniform float shake_rate;// : hint_range( 0.0, 1.0 ) = 0.2;
uniform float shake_speed;// = 5.0;
uniform float shake_block_size;// = 30.5;
uniform float shake_color_rate;// : hint_range( 0.0, 1.0 ) = 0.01;

float random(float seed) {
	return fract(543.2543 * sin(dot(vec2(seed, seed), vec2(3525.46, -54.3415))));
}

varying vec2 vUv;

uniform sampler2D uAuxSampler;
uniform float uTime;

out vec4 outColor = 0;

[[vert]] void main(void) {
	vec2 pos = kPositions[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
	vUv = pos * 0.5 + 0.5;
}

uniform float enable_shift;

[[frag]] void main(void) {
	//float enable_shift = float(random(trunc(uTime * shake_speed)) < shake_rate);

	vec2 fixed_uv = vUv;
	fixed_uv.x += (random((trunc(vUv.y * shake_block_size) / shake_block_size) + uTime) - 0.5) * shake_power * enable_shift;

	vec4 pixel_color = textureLod(uAuxSampler, fixed_uv, 0.0);
	pixel_color.r = mix(pixel_color.r, textureLod(uAuxSampler, fixed_uv + vec2( shake_color_rate, 0.0), 0.0).r, enable_shift);
	pixel_color.b = mix(pixel_color.b, textureLod(uAuxSampler, fixed_uv + vec2( -shake_color_rate, 0.0), 0.0).b, enable_shift);
	outColor = pixel_color;
}