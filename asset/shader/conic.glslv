///essl #version 300 es
///glsl #version 150

#if GL_ES == 1
#else
#endif

uniform float aspect;

in vec2 at_position;
out vec2 proj_coord_i;

void main()
{
	gl_Position = vec4(at_position.x * aspect, at_position.y, 0.0, 1.0);
	proj_coord_i = at_position;
}
