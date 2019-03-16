///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////
// skinning
////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;
in_qualifier vec4 at_Weight;

uniform mat4 bone[32];	// maximum 64 index-able
uniform mat4 mvp;		// mvp to clip space

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
}
