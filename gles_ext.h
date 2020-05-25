#if PLATFORM_GLES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if GL_GLEXT_PROTOTYPES == 0
#if GL_EXT_discard_framebuffer
extern PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT;

#endif // GL_EXT_discard_framebuffer
#if GL_OES_mapbuffer
extern PFNGLMAPBUFFEROESPROC         glMapBufferOES;
extern PFNGLUNMAPBUFFEROESPROC       glUnmapBufferOES;
extern PFNGLGETBUFFERPOINTERVOESPROC glGetBufferPointervOES;

#endif /* GL_OES_mapbuffer */
#if GL_OES_EGL_image
extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC           glEGLImageTargetTexture2DOES;
extern PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES;

#endif /* GL_OES_EGL_image */
#if PLATFORM_GL_OES_vertex_array_object
extern PFNGLBINDVERTEXARRAYOESPROC    glBindVertexArrayOES;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
extern PFNGLGENVERTEXARRAYSOESPROC    glGenVertexArraysOES;
extern PFNGLISVERTEXARRAYOESPROC      glIsVertexArrayOES;

#endif
#if PLATFORM_GL_KHR_debug
extern PFNGLDEBUGMESSAGECONTROLKHRPROC  glDebugMessageControlKHR;
extern PFNGLDEBUGMESSAGEINSERTKHRPROC   glDebugMessageInsertKHR;
extern PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR;
extern PFNGLGETDEBUGMESSAGELOGKHRPROC   glGetDebugMessageLogKHR;
extern PFNGLPUSHDEBUGGROUPKHRPROC       glPushDebugGroupKHR;
extern PFNGLPOPDEBUGGROUPKHRPROC        glPopDebugGroupKHR;
extern PFNGLOBJECTLABELKHRPROC          glObjectLabelKHR;
extern PFNGLGETOBJECTLABELKHRPROC       glGetObjectLabelKHR;
extern PFNGLOBJECTPTRLABELKHRPROC       glObjectPtrLabelKHR;
extern PFNGLGETOBJECTPTRLABELKHRPROC    glGetObjectPtrLabelKHR;
extern PFNGLGETPOINTERVKHRPROC          glGetPointervKHR;

#endif
#if __cplusplus
extern "C" {
#endif

void init_gles_ext(void);

#if __cplusplus
}
#endif
#endif /* GL_GLEXT_PROTOTYPES */
#endif /* PLATFORM_GLES */
