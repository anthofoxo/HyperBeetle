#include <common>

uniform sampler2D uAuxSampler;


uniform float uDistortion;      // -1.3
uniform float uCubicDistortion; // 0.5
uniform vec2 uOffset; // Varies a bit ??? float2(0.0001, 0.00062)

// y controls output color intensity
uniform vec2 uBlack_InvRange_InvGamma; // float2(0.07843, 1.08511)
uniform vec3 uInvGammas;               // float3(1.00,    1.00,    1.42857)
uniform vec3 uOutputRanges;            // float3(0.81922, 0.96353, 0.92941)
uniform vec3 uOutputBlacks;            // float3(0.18078, 0.03647, 0.07059)

varying vec2 vUv;

out vec4 outColor;

const vec2 kPositions[4] = vec2[]( vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0));

// https://www.shadertoy.com/view/4lSGRw
// https://web.archive.org/web/20190507005611/http://www.francois-tarlier.com/blog/cubic-lens-distortion-shader/
// https://www.ssontech.com/content/lensalg.html
float2 ComputeCubicDistortionUv(float2 uv, float2 offset, float k, float kcube) {
  float2 t = uv - float2(0.5, 0.5);
  float r2 = dot(t.xy, t.xy);
  float f = 1.0 + r2 * (k + kcube * sqrt(r2));
  float2 nUv = f * t + offset;
  nUv += float2(0.5, 0.5);
  // nUv.y = 1.0 - nUv.y;
  return nUv;
}

[[vert]] void main(void) {
	vec2 pos = kPositions[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
	vUv = pos * 0.5 + 0.5;
}

[[frag]] void main(void) {
  vec2 sampleUv = ComputeCubicDistortionUv(vUv, uOffset.xy, uDistortion, uCubicDistortion);
  vec3 colorValue = texture(uAuxSampler, sampleUv.xy).xyz;

  // Extra step, This doesnt appear to always happen, check hashes
  colorValue = log2(colorValue);
  colorValue = colorValue * uInvGammas.xyz;
  colorValue = exp2(colorValue);
  colorValue = min(colorValue, float3(1.0, 1.0, 1.0));
  
  colorValue = colorValue * uOutputRanges.xyz + uOutputBlacks.xyz;
  colorValue = colorValue + -uBlack_InvRange_InvGamma.xxx;
  colorValue = max(colorValue, float3(0.0, 0.0, 0.0));

  outColor = vec4(colorValue * uBlack_InvRange_InvGamma.yyy, 1.0);
}