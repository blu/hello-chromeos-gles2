///essl #version 300 es
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// tangential space bump mapping, vertex shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int max_instances = 9;

in vec3 at_Vertex;
in vec3 at_Normal;
in vec3 at_Tangent;
in vec2 at_MultiTexCoord0;

out vec2 tcoord_i;
out vec3 l_tan_i; // to-light-source vector in tangent space
out vec3 h_tan_i; // half-direction vector in tangent space

uniform mat4 mvp[max_instances];
uniform vec4 lp_obj[max_instances]; // light-source position in object space
uniform vec4 vp_obj[max_instances]; // viewer position in object space

void main()
{
	gl_Position = mvp[gl_InstanceID] * vec4(at_Vertex, 1.0);

	tcoord_i = at_MultiTexCoord0;

	vec3 l_obj = normalize(lp_obj[gl_InstanceID].xyz - at_Vertex * lp_obj[gl_InstanceID].w);
	vec3 v_obj = normalize(vp_obj[gl_InstanceID].xyz - at_Vertex * vp_obj[gl_InstanceID].w);
	vec3 h_obj = l_obj + v_obj;

	vec3 t = at_Tangent;
	vec3 b = cross(at_Normal, at_Tangent);
	vec3 n = at_Normal;

	mat3 tbn = mat3(t, b, n);

	l_tan_i = l_obj * tbn;
	h_tan_i = h_obj * tbn;
}
