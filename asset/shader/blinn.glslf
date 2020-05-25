///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////
// unshadowed, untextured Blinn for one positional/directional light source
////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1
#extension GL_OES_standard_derivatives : require

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

#define in_qualifier varying
#define xx_FragColor gl_FragColor

#else
#define in_qualifier in
out vec4 xx_FragColor;

#endif
const vec3 scene_ambient    = vec3(0.2, 0.2, 0.2);
const vec3 lprod_diffuse    = vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular   = vec3(0.7, 0.7, 0.5);
const float shininess       = 64.0;

in_qualifier vec3 p_obj_i; // vertex position in object space
in_qualifier vec3 l_obj_i; // to-light-source vector in object space
in_qualifier vec3 h_obj_i; // half-direction vector in object space

void main()
{
	vec3 t = dFdx(p_obj_i);
	vec3 b = dFdy(p_obj_i);
	vec3 n = normalize(cross(t, b));
	vec3 l = normalize(l_obj_i);
	vec3 h = normalize(h_obj_i);

	float lambertian = max(dot(n, l), 0.0);
	float blinnian	 = max(dot(n, h), 0.0);

	vec3 d = lprod_diffuse  * lambertian;
	vec3 s = lprod_specular * pow(blinnian, shininess);

	xx_FragColor = vec4(scene_ambient + d + s, 1.0);
}
