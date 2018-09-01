#ifndef pure_macro_H__
#define pure_macro_H__

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifdef DEBUG
	#define DEBUG_LITERAL 1
#else
	#define	DEBUG_LITERAL 0
#endif

#define DEBUG_GL_ERR() \
	if (DEBUG_LITERAL && util::reportGLError(stderr)) { \
		fprintf(stderr, "%s : %d\n", __FUNCTION__, __LINE__); \
		return false; \
	}

#endif // pure_macro_H_
