///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// tangential space bump mapping, vertex shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(GL_ES)

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;
in_qualifier vec3 at_Normal;
in_qualifier vec3 at_Tangent;
in_qualifier vec2 at_MultiTexCoord0;

out_qualifier vec2 tcoord_i;
out_qualifier vec3 l_tan_i; // to-light-source vector in tangent space
out_qualifier vec3 h_tan_i; // half-direction vector in tangent space

uniform mat4 mvp;
uniform vec4 lp_obj; // light-source position in object space
uniform vec4 vp_obj; // viewer position in object space

void main()
{
	gl_Position = mvp * vec4(at_Vertex, 1.0);

	tcoord_i = at_MultiTexCoord0;

	vec3 l_obj = normalize(lp_obj.xyz - at_Vertex * lp_obj.w);
	vec3 v_obj = normalize(vp_obj.xyz - at_Vertex * vp_obj.w);
	vec3 h_obj = l_obj + v_obj;

	vec3 t = at_Tangent;
	vec3 b = cross(at_Normal, at_Tangent);
	vec3 n = at_Normal;

	mat3 tbn = mat3(t, b, n);

	l_tan_i = l_obj * tbn;
	h_tan_i = h_obj * tbn;
}
