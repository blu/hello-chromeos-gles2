///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// tangential space bump mapping, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(GL_ES)
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

const vec3 scene_ambient  = vec3(0.2, 0.2, 0.2);
const vec3 lprod_diffuse  = vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular = vec3(0.7, 0.7, 0.5);
const float shininess     = 64.0;

in_qualifier vec2 tcoord_i;
in_qualifier vec3 l_tan_i; // to-light-source vector in tangent space
in_qualifier vec3 h_tan_i; // half-direction vector in tangent space

uniform sampler2D normal_map;
uniform sampler2D albedo_map;

void main()
{
	vec3 l_tan = normalize(l_tan_i);
	vec3 h_tan = normalize(h_tan_i);
	vec3 bump = normalize(texture(normal_map, tcoord_i).xyz * 2.0 - 1.0);

	float dp = dot(l_tan, bump);
	float sp = dot(h_tan, bump);

	vec3 d = lprod_diffuse *      max(dp, 0.0);
	vec3 s = lprod_specular * pow(max(sp, 0.0), shininess);

	// apply albedo map sans alpha
	vec3 tcolor = texture(albedo_map, tcoord_i).xyz;

	xx_FragColor = vec4(s + (d + scene_ambient) * tcolor, 1.0);
}
