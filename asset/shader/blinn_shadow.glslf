///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// quasi-pcf shadowed phong with one light source (no clipping), fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1
#extension GL_OES_standard_derivatives : require

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif
#define in_qualifier varying
#define xx_FragColor gl_FragColor
#define texture texture2D

#else
#define in_qualifier in
out vec4 xx_FragColor;

#endif
const vec4 scene_ambient	= vec4(0.2, 0.2, 0.2, 1.0);
const vec3 lprod_diffuse	= vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular	= vec3(0.7, 0.7, 0.5);
const float shininess		= 64.0;

in_qualifier vec4 p_lit_i;	// vertex position in light projection space
in_qualifier vec3 p_obj_i;	// vertex position in object space
in_qualifier vec3 l_obj_i;	// to-light-source vector in object space
in_qualifier vec3 h_obj_i;	// half-direction vector in object space
in_qualifier vec2 tcoord_i;

uniform sampler2D shadow_map;
uniform sampler2D albedo_map;

void main()
{
	const float shadow_res = 2048.0;

	vec3 p_lit = p_lit_i.xyz / p_lit_i.w;

	vec4 shadow_z = vec4(
		texture(shadow_map, p_lit.xy + vec2(0.707106781,  0.707106781) / shadow_res).x,
		texture(shadow_map, p_lit.xy + vec2(0.707106781, -0.707106781) / shadow_res).x,
		texture(shadow_map, p_lit.xy - vec2(0.707106781,  0.707106781) / shadow_res).x,
		texture(shadow_map, p_lit.xy - vec2(0.707106781, -0.707106781) / shadow_res).x);

	vec4 shadow_y = vec4(
		texture(shadow_map, p_lit.xy + vec2(1.0, 0.0) / shadow_res).x,
		texture(shadow_map, p_lit.xy + vec2(0.0, 1.0) / shadow_res).x,
		texture(shadow_map, p_lit.xy - vec2(1.0, 0.0) / shadow_res).x,
		texture(shadow_map, p_lit.xy - vec2(0.0, 1.0) / shadow_res).x);

	vec3 t = dFdx(p_obj_i);
	vec3 b = dFdy(p_obj_i);
	vec3 n = normalize(cross(t, b));
	vec3 l = normalize(l_obj_i);
	vec3 h = normalize(h_obj_i);

	float lambertian = max(dot(n, l), 0.0);
	float blinnian	 = max(dot(n, h), 0.0);

	vec3 d = lprod_diffuse  * lambertian;
	vec3 s = lprod_specular * pow(blinnian, shininess);

	float p_z_lit = p_lit.z;

	vec4 lit = step(vec4(p_z_lit), shadow_z);
	vec4 lis = step(vec4(p_z_lit), shadow_y);

	float lit_average = dot(lit, vec4(1.0 / 8.0)) +
						dot(lis, vec4(1.0 / 8.0));
	d *= lit_average;
	s *= lit_average;

	vec4 tcolor = texture(albedo_map, tcoord_i);

	// be considerate of the older SIMD architectures and compilers
	xx_FragColor =
		vec4(s, 0.0) +
		vec4(d, 0.0) * tcolor +
		scene_ambient * tcolor;
}
