///essl #version 300 es
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
#endif // GL_ES

const vec3 scene_ambient  = vec3(0.2, 0.2, 0.2);
const vec3 lprod_diffuse  = vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular = vec3(0.7, 0.7, 0.5);
const float shininess     = 64.0;

in vec2 tcoord_i;
in vec3 l_tan_i; // to-light-source vector in tangent space
in vec3 h_tan_i; // half-direction vector in tangent space

out vec4 xx_FragColor;

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
