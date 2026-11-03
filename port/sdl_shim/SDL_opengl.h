// Fake SDL_opengl.h: OpenGL is never enabled in this port.
#ifndef PORT_SDL_OPENGL_SHIM_H
#define PORT_SDL_OPENGL_SHIM_H
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned int GLenum;
#endif
