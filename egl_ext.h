#if PLATFORM_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>

#if EGL_EGLEXT_PROTOTYPES == 0
#if EGL_KHR_image
extern PFNEGLCREATEIMAGEKHRPROC  eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;

#endif /* EGL_KHR_image */
#if __cplusplus
extern "C" {
#endif

void init_egl_ext(void);

#if __cplusplus
}
#endif
#endif /* EGL_EGLEXT_PROTOTYPES */
#endif /* PLATFORM_EGL */
