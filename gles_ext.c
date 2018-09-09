#if PLATFORM_GLES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#if GL_GLEXT_PROTOTYPES == 0
#if GL_OES_mapbuffer
PFNGLMAPBUFFEROESPROC         glMapBufferOES;
PFNGLUNMAPBUFFEROESPROC       glUnmapBufferOES;
PFNGLGETBUFFERPOINTERVOESPROC glGetBufferPointervOES;

#endif /* GL_OES_mapbuffer */
#if GL_OES_EGL_image
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC           glEGLImageTargetTexture2DOES;
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES;

#endif /* GL_OES_EGL_image */
#if PLATFORM_GL_OES_vertex_array_object
PFNGLBINDVERTEXARRAYOESPROC    glBindVertexArrayOES;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
PFNGLGENVERTEXARRAYSOESPROC    glGenVertexArraysOES;
PFNGLISVERTEXARRAYOESPROC      glIsVertexArrayOES;

#endif
#if PLATFORM_GL_KHR_debug
PFNGLDEBUGMESSAGECONTROLKHRPROC  glDebugMessageControlKHR;
PFNGLDEBUGMESSAGEINSERTKHRPROC   glDebugMessageInsertKHR;
PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR;
PFNGLGETDEBUGMESSAGELOGKHRPROC   glGetDebugMessageLogKHR;
PFNGLPUSHDEBUGGROUPKHRPROC       glPushDebugGroupKHR;
PFNGLPOPDEBUGGROUPKHRPROC        glPopDebugGroupKHR;
PFNGLOBJECTLABELKHRPROC          glObjectLabelKHR;
PFNGLGETOBJECTLABELKHRPROC       glGetObjectLabelKHR;
PFNGLOBJECTPTRLABELKHRPROC       glObjectPtrLabelKHR;
PFNGLGETOBJECTPTRLABELKHRPROC    glGetObjectPtrLabelKHR;
PFNGLGETPOINTERVKHRPROC          glGetPointervKHR;

#endif
void init_gles_ext(void) {

#if GL_OES_mapbuffer
	glMapBufferOES         = eglGetProcAddress("glMapBufferOES");
	glUnmapBufferOES       = eglGetProcAddress("glUnmapBufferOES");
	glGetBufferPointervOES = eglGetProcAddress("glGetBufferPointervOES");

#endif /* GL_OES_mapbuffer */
#if GL_OES_EGL_image
	glEGLImageTargetTexture2DOES           = eglGetProcAddress("glEGLImageTargetTexture2DOES");
	glEGLImageTargetRenderbufferStorageOES = eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");

#endif /* GL_OES_EGL_image */
#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES    = eglGetProcAddress("glBindVertexArrayOES");
	glDeleteVertexArraysOES = eglGetProcAddress("glDeleteVertexArraysOES");
	glGenVertexArraysOES    = eglGetProcAddress("glGenVertexArraysOES");
	glIsVertexArrayOES      = eglGetProcAddress("glIsVertexArrayOES");

#endif
#if PLATFORM_GL_KHR_debug
	glDebugMessageControlKHR  = eglGetProcAddress("glDebugMessageControlKHR");
	glDebugMessageInsertKHR   = eglGetProcAddress("glDebugMessageInsertKHR");
	glDebugMessageCallbackKHR = eglGetProcAddress("glDebugMessageCallbackKHR");
	glGetDebugMessageLogKHR   = eglGetProcAddress("glGetDebugMessageLogKHR");
	glPushDebugGroupKHR       = eglGetProcAddress("glPushDebugGroupKHR");
	glPopDebugGroupKHR        = eglGetProcAddress("glPopDebugGroupKHR");
	glObjectLabelKHR          = eglGetProcAddress("glObjectLabelKHR");
	glGetObjectLabelKHR       = eglGetProcAddress("glGetObjectLabelKHR");
	glObjectPtrLabelKHR       = eglGetProcAddress("glObjectPtrLabelKHR");
	glGetObjectPtrLabelKHR    = eglGetProcAddress("glGetObjectPtrLabelKHR");
	glGetPointervKHR          = eglGetProcAddress("glGetPointervKHR");

#endif
}

#endif /* GL_GLEXT_PROTOTYPES */
#endif /* PLATFORM_GLES */
