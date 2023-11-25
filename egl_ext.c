#if PLATFORM_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>

#if EGL_EGLEXT_PROTOTYPES == 0
#if EGL_KHR_image
PFNEGLCREATEIMAGEKHRPROC  eglCreateImageKHR;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;

#endif /* EGL_KHR_image */
#if EGL_KHR_fence_sync
PFNEGLCREATESYNCKHRPROC     eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC    eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
PFNEGLGETSYNCATTRIBKHRPROC  eglGetSyncAttribKHR;

#endif /* EGL_KHR_fence_sync */
void init_egl_ext(void) {
#if EGL_KHR_image
	eglCreateImageKHR  = (PFNEGLCREATEIMAGEKHRPROC)  eglGetProcAddress("eglCreateImageKHR");
	eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");

#endif /* EGL_KHR_image */
#if EGL_KHR_fence_sync
	eglCreateSyncKHR     = (PFNEGLCREATESYNCKHRPROC)     eglGetProcAddress("eglCreateSyncKHR");
	eglDestroySyncKHR    = (PFNEGLDESTROYSYNCKHRPROC)    eglGetProcAddress("eglDestroySyncKHR");
	eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) eglGetProcAddress("eglClientWaitSyncKHR");
	eglGetSyncAttribKHR  = (PFNEGLGETSYNCATTRIBKHRPROC)  eglGetProcAddress("eglGetSyncAttribKHR");

#endif /* EGL_KHR_fence_sync */
}

#endif /* EGL_EGLEXT_PROTOTYPES */
#endif /* PLATFORM_EGL */
