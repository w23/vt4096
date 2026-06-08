#include "render.h"
#include "terminal.h"
#include "font.h"
#include "common.h"

#include <gl/GL.h>
#include "glext.h"

#define LIST_GL_FUNCS(X) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLDRAWBUFFERSPROC, glDrawBuffers) \
	X(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation) \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
	X(PFNGLGETINFOLOGARBPROC, glGetInfoLogARB) \
	X(PFNGLGETOBJECTPARAMETERIVARBPROC, glGetObjectParameterivARB) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \
	X(PFNGLUNIFORM1FPROC, glUniform1f) \
	X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
	X(PFNGLUNIFORM1IPROC, glUniform1i) \
	X(PFNGLUNIFORM1IVPROC, glUniform1iv) \
	X(PFNGLUNIFORM2FPROC, glUniform2f) \
	X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
	X(PFNGLUNIFORM2IVPROC, glUniform2iv) \
	X(PFNGLUNIFORM3FPROC, glUniform3f) \
	X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
	X(PFNGLUNIFORM3IVPROC, glUniform3iv) \
	X(PFNGLUNIFORM4FPROC, glUniform4f) \
	X(PFNGLUNIFORM4FVPROC, glUniform4fv) \
	X(PFNGLUNIFORM4IVPROC, glUniform4iv) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \

	//X(PFNGLCREATESHADERPROGRAMV, glCreateShaderProgramv) \

#define X(t, n) static t n = NULL;
LIST_GL_FUNCS(X)
#undef X

#define LIST_TEXTURES(X) \
	X(FontAtlas) \
	X(GridChar) \
	X(GridColor) \
	X(GridBg) \

enum {
#define X(t) Tex##t,
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

static const char* kFrag =
"#version 130\n"
#define X(t) "uniform sampler2D " #t ";\n"
LIST_TEXTURES(X)
#undef X
"uniform vec2 resolution;\n"
"uniform vec2 charSize;\n"
"uniform vec2 gridSize;\n"
"uniform float topRow;\n"
"uniform vec3 cursor;\n"
"vec4 tex(sampler2D T, vec2 pix) { return texture(T, pix / textureSize(T, 0)); }\n"
"void main() {\n"
	// in pixels, +.5 sample position included
"	vec2 screen_pix = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);\n"
	// do not draw partial glyps outside of the grid
"	vec2 grid_texel = floor((screen_pix.xy - .5) / charSize);\n"
"	if (any(greaterThanEqual(grid_texel, gridSize))) { gl_FragColor = vec4(0.); return; }\n"

// Exact pixel sample coordinated (+.5) to read from the grid, offset by topRow ring buffer
"	vec2 grid_sample_texel = grid_texel + .5 + vec2(0., topRow);\n"
"	vec4 char_loc = tex(GridChar, grid_sample_texel);\n"
"	vec4 color = tex(GridColor, grid_sample_texel);\n"
"	vec4 bg = tex(GridBg, grid_sample_texel);\n"

// Coordinates within a single on-screen grid char
"	vec2 char_pix = mod(screen_pix.xy, charSize);\n"
// GL coord system to GDI
"	char_pix.y = charSize.y - char_pix.y;\n"

"	vec2 atlasCharSize = charSize;\n"
"	vec2 glyph_offset = char_loc.rg * 255. * atlasCharSize;\n"

"	vec2 atlas_pix = char_pix + glyph_offset;\n"
"	vec4 glyph = tex(FontAtlas, atlas_pix);\n"
//"	vec4 glyph = tex(FontAtlas, gl_FragCoord.xy);\n"
"	gl_FragColor = mix(bg, color, glyph.r);\n"
"	if (cursor.z > 0) {\n"
"		if (all(equal(grid_texel, cursor.xy))) {\n"
"			gl_FragColor.b = 1.;\n"
"		}\n"
"	}\n"
"}\n";

//GLint makeProgram(const char* vert, const char* frag) {
GLint makeProgram(const char* frag) {
	const GLint pid = glCreateProgram();
//	glAttachShader(pid, compileShader(GL_VERTEX_SHADER, vert));
	glAttachShader(pid, compileShader(GL_FRAGMENT_SHADER, frag));
	glLinkProgram(pid);

	return pid;

	// TODO const GLuint pid = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sources);
}

static void uploadGrid(void) {
	uploadTexture(TexGridChar, grid.cols, grid.rows, grid.chars);
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

static const PIXELFORMATDESCRIPTOR kPfd = { sizeof(kPfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };

void renderInit(void) {
	render.hdc = GetDC(mainWindow);

	SetPixelFormat(render.hdc, ChoosePixelFormat(render.hdc, &kPfd), &kPfd);
	HGLRC hglrc = wglCreateContext(render.hdc);
	wglMakeCurrent(render.hdc, hglrc);
	loadGLFuncs();

	glGenTextures(Tex_COUNT, render.textures);
	uploadTexture(TexFontAtlas, font.atlasWidth, font.atlasHeight, font.atlasBits);
	GL_CHECK();

	const GLint prog = makeProgram(kFrag);
	render.uniform_resolution = glGetUniformLocation(prog, "resolution");
	render.uniform_gridSize = glGetUniformLocation(prog, "gridSize");
	render.uniform_topRow = glGetUniformLocation(prog, "topRow");
	render.uniform_cursor = glGetUniformLocation(prog, "cursor");

	glUseProgram(prog);

#ifdef _DEBUG
#define X(t) { \
	const GLint loc = glGetUniformLocation(prog, #t); \
	if (loc < 0) { \
		MessageBoxA(NULL, #t, "uniform not found", 0); \
	} else { \
		glUniform1i(loc, Tex##t); \
	} \
}
#else
#define X(t) \
glUniform1i(glGetUniformLocation(prog, #t), Tex##t);
#endif
	LIST_TEXTURES(X)
#undef X
		GL_CHECK();

	const GLint uniform_charSize = glGetUniformLocation(prog, "charSize");
	glUniform2f(uniform_charSize, (float)font.charWidth, (float)font.charHeight);
}

void renderResize(int w, int h) {
	glViewport(0, 0, w, h);
	glUniform2f(render.uniform_resolution, (float)w, (float)h);
	glUniform2f(render.uniform_gridSize, (float)grid.cols, (float)grid.rows);
}

void renderPaint(void) {
	uploadGrid();
	grid.dirty = 0;
	glUniform1f(render.uniform_topRow, (float)grid.top_row);
	glUniform3f(render.uniform_cursor, (float)grid.cursor.col, (float)grid.cursor.row, (float)grid.cursor.shape);
	glRects(-1, -1, 1, 1);
	SwapBuffers(render.hdc);
}
