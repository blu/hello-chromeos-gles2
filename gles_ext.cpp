#if PLATFORM_GLES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#if GL_GLEXT_PROTOTYPES == 0
#if GL_OES_mapbuffer
PFNGLMAPBUFFEROESPROC         glMapBufferOES         = (PFNGLMAPBUFFEROESPROC)         eglGetProcAddress("glMapBufferOES");
PFNGLUNMAPBUFFEROESPROC       glUnmapBufferOES       = (PFNGLUNMAPBUFFEROESPROC)       eglGetProcAddress("glUnmapBufferOES");
PFNGLGETBUFFERPOINTERVOESPROC glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOESPROC) eglGetProcAddress("glGetBufferPointervOES");

#endif
#if PLATFORM_GL_OES_vertex_array_object
PFNGLBINDVERTEXARRAYOESPROC    glBindVertexArrayOES    = (PFNGLBINDVERTEXARRAYOESPROC)    eglGetProcAddress("glBindVertexArrayOES");
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC) eglGetProcAddress("glDeleteVertexArraysOES");
PFNGLGENVERTEXARRAYSOESPROC    glGenVertexArraysOES    = (PFNGLGENVERTEXARRAYSOESPROC)    eglGetProcAddress("glGenVertexArraysOES");
PFNGLISVERTEXARRAYOESPROC      glIsVertexArrayOES      = (PFNGLISVERTEXARRAYOESPROC)      eglGetProcAddress("glIsVertexArrayOES");

#endif
#if PLATFORM_GL_KHR_debug
PFNGLDEBUGMESSAGECONTROLKHRPROC  glDebugMessageControlKHR  = (PFNGLDEBUGMESSAGECONTROLKHRPROC)  eglGetProcAddress("glDebugMessageControlKHR");
PFNGLDEBUGMESSAGEINSERTKHRPROC   glDebugMessageInsertKHR   = (PFNGLDEBUGMESSAGEINSERTKHRPROC)   eglGetProcAddress("glDebugMessageInsertKHR");
PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC) eglGetProcAddress("glDebugMessageCallbackKHR");
PFNGLGETDEBUGMESSAGELOGKHRPROC   glGetDebugMessageLogKHR   = (PFNGLGETDEBUGMESSAGELOGKHRPROC)   eglGetProcAddress("glGetDebugMessageLogKHR");
PFNGLPUSHDEBUGGROUPKHRPROC       glPushDebugGroupKHR       = (PFNGLPUSHDEBUGGROUPKHRPROC)       eglGetProcAddress("glPushDebugGroupKHR");
PFNGLPOPDEBUGGROUPKHRPROC        glPopDebugGroupKHR        = (PFNGLPOPDEBUGGROUPKHRPROC)        eglGetProcAddress("glPopDebugGroupKHR");
PFNGLOBJECTLABELKHRPROC          glObjectLabelKHR          = (PFNGLOBJECTLABELKHRPROC)          eglGetProcAddress("glObjectLabelKHR");
PFNGLGETOBJECTLABELKHRPROC       glGetObjectLabelKHR       = (PFNGLGETOBJECTLABELKHRPROC)       eglGetProcAddress("glGetObjectLabelKHR");
PFNGLOBJECTPTRLABELKHRPROC       glObjectPtrLabelKHR       = (PFNGLOBJECTPTRLABELKHRPROC)       eglGetProcAddress("glObjectPtrLabelKHR");
PFNGLGETOBJECTPTRLABELKHRPROC    glGetObjectPtrLabelKHR    = (PFNGLGETOBJECTPTRLABELKHRPROC)    eglGetProcAddress("glGetObjectPtrLabelKHR");
PFNGLGETPOINTERVKHRPROC          glGetPointervKHR          = (PFNGLGETPOINTERVKHRPROC)          eglGetProcAddress("glGetPointervKHR");

#endif
#endif // GL_GLEXT_PROTOTYPES
#endif // PLATFORM_GLES
