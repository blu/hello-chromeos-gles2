///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////
// shadowed, textured, skinned phong for one positional/directional light source
////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1
#define in_qualifier attribute
#define out_qualifier varying

#else
#define in_qualifier in
#define out_qualifier out

#endif
in_qualifier vec3 at_Vertex;
in_qualifier vec3 at_Normal;
in_qualifier vec4 at_Weight;
in_qualifier vec2 at_MultiTexCoord0;

out_qualifier vec4 p_lit_i;  // vertex position in light projection space
out_qualifier vec3 p_obj_i;  // vertex position in object space
out_qualifier vec3 l_obj_i;  // to-light-source vector in object space
out_qualifier vec3 h_obj_i;  // half-direction vector in object space
out_qualifier vec2 tcoord_i; // vertex position in texcoord space

uniform mat4 bone[32]; // maximum 64 index-able
uniform mat4 mvp;      // mvp to clip space
uniform mat4 mvp_lit;  // mvp to light clip space
uniform vec4 lp_obj;   // light position in object space
uniform vec4 vp_obj;   // viewer position in object space

void main()
{
	vec4 weight = vec4(at_Weight.xyz, 1.0 - (at_Weight.x + at_Weight.y + at_Weight.z));

	vec4 fndex = mod(at_Weight.w * vec4(1.0, 1.0 / 64.0, 1.0 / 4096.0, 1.0 / 262144.0), vec4(64.0));
	ivec4 index = ivec4(fndex);

	mat4 sum =
		bone[index.x] * weight.x +
		bone[index.y] * weight.y +
		bone[index.z] * weight.z +
		bone[index.w] * weight.w;

	vec3 p_obj = (sum * vec4(at_Vertex, 1.0)).xyz;

	gl_Position = mvp * vec4(p_obj, 1.0);
	p_lit_i = mvp_lit * vec4(p_obj, 1.0);
	p_obj_i = p_obj;

	vec3 l_obj = normalize(lp_obj.xyz - p_obj * lp_obj.w);
	vec3 v_obj = normalize(vp_obj.xyz - p_obj * vp_obj.w);

	l_obj_i = l_obj;
	h_obj_i = l_obj + v_obj;
	tcoord_i = at_MultiTexCoord0;
}
