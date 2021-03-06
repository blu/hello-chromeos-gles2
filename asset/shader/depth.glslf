///essl #version 100
///glsl #version 150

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// depth-only fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

#else
#endif

void main()
{
	// output nothing; gl_FragDepth will automatically be filled from gl_FragCoord.z
}
