#ifndef _OPENGL_H_
#define _OPENGL_H_

#if defined( __APPLE__ ) && !defined(__MACOSX)

#define OPENGLES 2
#include <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#define PRECISION "precision lowp float;"
#define PRECISION_HIGH "precision highp float;"

#elif defined(linux) || defined(__linux) || defined(__linux__)

#define OPENGLES 0
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define PRECISION "precision lowp float;"
#define PRECISION_HIGH "precision highp float;"

#else

#define OPENGLES 0
#include <GL/glew.h>
#define PRECISION 
#define PRECISION_HIGH 

#endif

#endif // _OPENGL_H_
