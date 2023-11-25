#if PLATFORM_GLES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#if GL_GLEXT_PROTOTYPES == 0
#if GL_EXT_discard_framebuffer
PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT;

#endif // GL_EXT_discard_framebuffer
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

#if GL_EXT_discard_framebuffer
	glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXTPROC) eglGetProcAddress("glDiscardFramebufferEXT");

#endif // GL_EXT_discard_framebuffer
#if GL_OES_mapbuffer
	glMapBufferOES         = (PFNGLMAPBUFFEROESPROC)         eglGetProcAddress("glMapBufferOES");
	glUnmapBufferOES       = (PFNGLUNMAPBUFFEROESPROC)       eglGetProcAddress("glUnmapBufferOES");
	glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOESPROC) eglGetProcAddress("glGetBufferPointervOES");

#endif /* GL_OES_mapbuffer */
#if GL_OES_EGL_image
	glEGLImageTargetTexture2DOES           = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)           eglGetProcAddress("glEGLImageTargetTexture2DOES");
	glEGLImageTargetRenderbufferStorageOES = (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");

#endif /* GL_OES_EGL_image */
#if PLATFORM_GL_OES_vertex_array_object
	glBindVertexArrayOES    = (PFNGLBINDVERTEXARRAYOESPROC)    eglGetProcAddress("glBindVertexArrayOES");
	glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC) eglGetProcAddress("glDeleteVertexArraysOES");
	glGenVertexArraysOES    = (PFNGLGENVERTEXARRAYSOESPROC)    eglGetProcAddress("glGenVertexArraysOES");
	glIsVertexArrayOES      = (PFNGLISVERTEXARRAYOESPROC)      eglGetProcAddress("glIsVertexArrayOES");

#endif
#if PLATFORM_GL_KHR_debug
	glDebugMessageControlKHR  = (PFNGLDEBUGMESSAGECONTROLKHRPROC)  eglGetProcAddress("glDebugMessageControlKHR");
	glDebugMessageInsertKHR   = (PFNGLDEBUGMESSAGEINSERTKHRPROC)   eglGetProcAddress("glDebugMessageInsertKHR");
	glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC) eglGetProcAddress("glDebugMessageCallbackKHR");
	glGetDebugMessageLogKHR   = (PFNGLGETDEBUGMESSAGELOGKHRPROC)   eglGetProcAddress("glGetDebugMessageLogKHR");
	glPushDebugGroupKHR       = (PFNGLPUSHDEBUGGROUPKHRPROC)       eglGetProcAddress("glPushDebugGroupKHR");
	glPopDebugGroupKHR        = (PFNGLPOPDEBUGGROUPKHRPROC)        eglGetProcAddress("glPopDebugGroupKHR");
	glObjectLabelKHR          = (PFNGLOBJECTLABELKHRPROC)          eglGetProcAddress("glObjectLabelKHR");
	glGetObjectLabelKHR       = (PFNGLGETOBJECTLABELKHRPROC)       eglGetProcAddress("glGetObjectLabelKHR");
	glObjectPtrLabelKHR       = (PFNGLOBJECTPTRLABELKHRPROC)       eglGetProcAddress("glObjectPtrLabelKHR");
	glGetObjectPtrLabelKHR    = (PFNGLGETOBJECTPTRLABELKHRPROC)    eglGetProcAddress("glGetObjectPtrLabelKHR");
	glGetPointervKHR          = (PFNGLGETPOINTERVKHRPROC)          eglGetProcAddress("glGetPointervKHR");

#endif
}

#endif /* GL_GLEXT_PROTOTYPES */
#endif /* PLATFORM_GLES */
