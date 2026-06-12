#include "render.h"
#include "terminal.h"
#include "font.h"
#include "common.h"

#include <gl/GL.h>
#include "glext.h"

#ifdef _DEBUG
#define LIST_GL_SHADER_FUNCS(X) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLGETINFOLOGARBPROC, glGetInfoLogARB) \
	X(PFNGLGETOBJECTPARAMETERIVARBPROC, glGetObjectParameterivARB) \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \

#else
#define LIST_GL_SHADER_FUNCS(X) \
	X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv) \

#endif

#define LIST_GL_FUNCS(X) \
	LIST_GL_SHADER_FUNCS(X) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
	X(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLUNIFORM1FPROC, glUniform1f) \
	X(PFNGLUNIFORM1IPROC, glUniform1i) \
	X(PFNGLUNIFORM2FPROC, glUniform2f) \
	X(PFNGLUNIFORM3FPROC, glUniform3f) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \

#define X(t, n) static t n = NULL;
LIST_GL_FUNCS(X)
#undef X

#define LIST_TEXTURES(X) \
	X(FontAtlas, A) \
	X(GridGlyph, G) \
	X(GridColor, C) \
	X(GridBg, B) \

enum {
#define X(t, SN) Tex##t,
LIST_TEXTURES(X)
#undef X

	Tex_COUNT
};

static struct GLOBALS__ {
	HDC hdc;
	GLuint textures[Tex_COUNT];

	GLint uniform_resolution;
	GLint uniform_gridSize;
	GLint uniform_topRow;
	GLint uniform_cursor;
} render;

static /*__forceinline*/ void initTexture(GLuint tex, int w, int h, int comp, int type, void* data) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, comp, w, h, 0, GL_RGBA, type, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	*/
}

static void uploadTexture(unsigned int texture_index, int w, int h, void* data) {
	glActiveTexture(GL_TEXTURE0 + texture_index);
	glBindTexture(GL_TEXTURE_2D, render.textures[texture_index]);
	initTexture(render.textures[texture_index], w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

static void loadGLFuncs(void) {
#define X(t, n) n = (t)(void*)wglGetProcAddress(#n);
	LIST_GL_FUNCS(X)
#undef X
}

#ifdef _DEBUG
static GLint compileShader(GLuint type, const char* src) {
	const int fsId = glCreateShader(type);
	glShaderSource(fsId, 1, &src, 0);
	glCompileShader(fsId);

#ifdef _DEBUG
	int result;
	char info[2048];
	glGetObjectParameterivARB(fsId, GL_OBJECT_COMPILE_STATUS_ARB, &result);
	glGetInfoLogARB(fsId, sizeof(info)-1, NULL, (char*)info);
	if (!result)
	{
		MessageBoxA(NULL, info, "COMPILE", 0x00000000L);
		ExitProcess(0);
	}
#endif
	return fsId;
}
#endif

#pragma data_seg(".render.grid.glsl")
#include "grid.h"

//GLint makeProgram(const char* vert, const char* frag) {
static GLint makeProgram(const char* frag) {
#ifdef _DEBUG
	const GLint pid = glCreateProgram();
//	glAttachShader(pid, compileShader(GL_VERTEX_SHADER, vert));
	glAttachShader(pid, compileShader(GL_FRAGMENT_SHADER, frag));
	glLinkProgram(pid);

	return pid;
#else
	return glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &frag);
#endif

}

static void uploadGrid(void) {
	uploadTexture(TexGridGlyph, grid.cols, grid.rows, grid.glyphs);
	uploadTexture(TexGridColor, grid.cols, grid.rows, grid.color);
	uploadTexture(TexGridBg, grid.cols, grid.rows, grid.bg);
}

#ifdef _DEBUG
#include <stdio.h>
static const char *getGlErrorStr(int error) {
	switch (error) {
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM"; break;
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE"; break;
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION"; break;
#ifdef GL_STACK_OVERFLOW
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW"; break;
#endif
#ifdef GL_STACK_UNDERFLOW
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW"; break;
#endif
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY"; break;
#ifdef GL_TABLE_TOO_LARGE
	case GL_TABLE_TOO_LARGE: return "GL_TABLE_TOO_LARGE"; break;
#endif
	case 1286: return "INVALID FRAMEBUFFER"; break;
	};
	return "UNKNOWN";
}

static void checkGLError(const char* file, int line, const char* func) {
	const int glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		char buf[256];
		snprintf(buf, sizeof(buf), "%s:%d (%s)", file, line, func);
		MessageBoxA(NULL, buf, getGlErrorStr(glerror), 0);
		ExitProcess(1);
	}
}

#define GL_CHECK() \
	checkGLError(__FILE__, __LINE__, __func__)

#else // no debug
#define GL_CHECK()
#endif

#pragma data_seg(".render.pfd")
static const PIXELFORMATDESCRIPTOR kPfd = { sizeof(kPfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };

#pragma code_seg(".render.init")
void renderInit(void) {
	render.hdc = GetDC(mainWindow);

	SetPixelFormat(render.hdc, ChoosePixelFormat(render.hdc, &kPfd), &kPfd);
	const HGLRC hglrc = wglCreateContext(render.hdc);
	wglMakeCurrent(render.hdc, hglrc);
	loadGLFuncs();

	glGenTextures(Tex_COUNT, render.textures);

	const GLint prog = makeProgram(grid_glsl);
	glUseProgram(prog);

	render.uniform_resolution = glGetUniformLocation(prog, "R");
	render.uniform_gridSize = glGetUniformLocation(prog, "I");
	render.uniform_topRow = glGetUniformLocation(prog, "T");
	render.uniform_cursor = glGetUniformLocation(prog, "U");

#ifdef _DEBUG
#define X(t, SN) { \
	const GLint loc = glGetUniformLocation(prog, #SN); \
	if (loc < 0) { \
		MessageBoxA(NULL, #t, "uniform not found", 0); \
	} else { \
		glUniform1i(loc, Tex##t); \
	} \
}
#else
#define X(t, SN) \
glUniform1i(glGetUniformLocation(prog, #SN), Tex##t);
#endif
	LIST_TEXTURES(X)
#undef X
	GL_CHECK();

	const GLint uniform_charSize = glGetUniformLocation(prog, "S");
	glUniform2f(uniform_charSize, (float)font.charWidth, (float)font.charHeight);
	GL_CHECK();
}

#pragma code_seg(".render.resize")
void renderResize(int w, int h) {
	glViewport(0, 0, w, h);
	GL_CHECK();
	glUniform2f(render.uniform_resolution, (float)w, (float)h);
	GL_CHECK();
	glUniform2f(render.uniform_gridSize, (float)grid.cols, (float)grid.rows);
	GL_CHECK();
}

#pragma code_seg(".render.paint")
void renderPaint(void) {
	GL_CHECK();
	if (font.dirty) {
		// TODO only upload dirty rect
		uploadTexture(TexFontAtlas, font.atlasWidth, font.atlasHeight, font.atlasBits);
		GL_CHECK();
		font.dirty = 0;
	}
	uploadGrid();
	GL_CHECK();
	grid.dirty = 0;
	GL_CHECK();
	glUniform1f(render.uniform_topRow, (float)grid.top_row);
	GL_CHECK();
	glUniform3f(render.uniform_cursor, (float)grid.cursor.col, (float)grid.cursor.row, (float)grid.cursor.shape);
	GL_CHECK();
	glRects(-1, -1, 1, 1);
	GL_CHECK();
	SwapBuffers(render.hdc);
	GL_CHECK();
}
