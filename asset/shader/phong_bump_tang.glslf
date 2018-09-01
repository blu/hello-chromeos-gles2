///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// tangential space bump mapping, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(GL_ES)
#extension GL_OES_standard_derivatives : require

#if defined(GL_FRAGMENT_PRECISION_HIGH)
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

const vec3 scene_ambient	= vec3(0.2, 0.2, 0.2);
const vec3 lprod_diffuse	= vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular	= vec3(0.7, 0.7, 0.5);
const float shininess		= 64.0;

in_qualifier vec3 p_obj_i;	// vertex position in object space
in_qualifier vec3 n_obj_i;	// vertex normal in object space
in_qualifier vec3 l_obj_i;	// light_source vector in object space
in_qualifier vec3 h_obj_i;	// half-direction vector in object space
in_qualifier vec2 tcoord_i;

uniform sampler2D normal_map;
uniform sampler2D albedo_map;

void main()
{
	vec3 p_dx = dFdx(p_obj_i);
	vec3 p_dy = dFdy(p_obj_i);

	vec2 tc_dx = dFdx(tcoord_i);
	vec2 tc_dy = dFdy(tcoord_i);

	vec3 t = normalize(tc_dy.y * p_dx - tc_dx.y * p_dy);
	vec3 b = normalize(tc_dx.x * p_dy - tc_dy.x * p_dx);

#if 1 // take into account the mesh-supplied normal
	vec3 n = normalize(n_obj_i);

#else
	vec3 n = normalize(cross(p_dx, p_dy));

#endif
	b = normalize(cross(n, t));
	t = cross(b, n);

	mat3 tbn = mat3(t, b, n);

	vec3 l_tan = normalize(l_obj_i) * tbn;
	vec3 h_tan = normalize(h_obj_i) * tbn;

	vec3 bump = normalize(texture(normal_map, tcoord_i).xyz * 2.0 - 1.0);

	vec3 d = lprod_diffuse *      max(dot(l_tan, bump), 0.0);
	vec3 s = lprod_specular * pow(max(dot(h_tan, bump), 0.0), shininess);

#if 1 // apply albedo map sans alpha
	vec3 tcolor = texture(albedo_map, tcoord_i).xyz;

#else
	const vec3 tcolor = vec3(1.0);

#endif
	xx_FragColor = vec4(s + (d + scene_ambient) * tcolor, 1.0);
}
