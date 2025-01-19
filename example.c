#define GLFW_INCLUDE_NONE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define GLAD_GL_IMPLEMENTATION
#include "glad.h"

typedef struct {
	float xMin, xMax;
	float yMin, yMax;
	float bearingX;
	float bearingY;
	float advance;
} GLTglyph;

typedef struct {
	GLfloat *vertices;
	GLsizei maxVertexCount;
	GLsizei vertexCount;

	GLuint *indices;
	GLsizei maxIndexCount;
	GLsizei indexCount;
} GLTbuffer;

static void errorCallback(int err, const char *desc);
static void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods);

static const char *title = "Simple example";

static void
errorCallback(int err, const char *desc)
{
    fprintf(stderr, "Error: %s\n", desc);
}

static void
keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
}

static GLuint gltProgram;

static GLuint
gltCreateShader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	int compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == 0) {
		char infoLog[1024] = {0};
		glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
		fprintf(stderr, "compile error: %s\n", infoLog);
	}

	return shader;
}

static GLuint gltProgram;

static void
gltUseProgram(void)
{
	if (!gltProgram) {
		static const char *vertexSource = "#version 430 core\n"
			"layout (location = 0) in vec2 aPos;\n"
			"layout (location = 1) in vec2 aTexCoords;\n"
			"uniform mat4 transform;\n"
			"out vec2 vTexCoords;\n"
			"void main()\n"
			"{\n"
			"    vTexCoords = aTexCoords;\n"
			"    gl_Position = transform * vec4(aPos, 0.0, 1.0);\n"
			"}\n";

		static const char *fragmentSource = "#version 430 core\n"
			"in vec2 vTexCoords;\n"
			"uniform sampler2D textureAtlas;"
			"out vec4 fragColor;\n"
			"void main()\n"
			"{"
			"    fragColor = vec4(1., 1., 1., texture(textureAtlas, vTexCoords).r);\n"
			"}\n";

		GLuint vertexShader = gltCreateShader(GL_VERTEX_SHADER, vertexSource);
		GLuint fragmentShader = gltCreateShader(GL_FRAGMENT_SHADER, fragmentSource);

		gltProgram = glCreateProgram();
		glAttachShader(gltProgram, vertexShader);
		glAttachShader(gltProgram, fragmentShader);
		glLinkProgram(gltProgram);

		int linkStatus = 0;
		glGetProgramiv(gltProgram, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == 0) {
			char infoLog[1024] = {0};
			glGetProgramInfoLog(gltProgram, sizeof(infoLog), NULL, infoLog);
			fprintf(stderr, "link error: %s\n", infoLog);
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	glUseProgram(gltProgram);
}

static void
gltOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
{
	float matrix[4][4] = {0};

	matrix[0][0] = 2. / (right - left);
	matrix[1][1] = 2. / (top - bottom);
	matrix[2][2] = -2. / (zFar - zNear);
	matrix[3][3] = 1.;

	matrix[0][3] = - (right + left) / (right - left);
	matrix[1][3] = - (top + bottom) / (top - bottom);
	matrix[2][3] = - (zFar + zNear) / (zFar - zNear);

	gltUseProgram();
	GLuint transformLocation = glGetUniformLocation(gltProgram, "transform");
	glUniformMatrix4fv(transformLocation, 1, GL_TRUE, &matrix[0][0]);
}

static void
gltDraw(GLTbuffer b)
{
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), b.vertices);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), b.vertices + 2);
	glDrawElements(GL_TRIANGLES, b.indexCount, GL_UNSIGNED_INT, b.indices);
}

int main(void)
{
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(640, 480, title, NULL, NULL);
    if (!window) {
		goto error;
	}

    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
		goto error;
	}

    glfwSwapInterval(1);

	/*
	 * Initialize the text API
	 */

	GLuint textureAtlas = 0;
	GLTglyph glyphs[256] = {0};

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		fprintf(stderr, "Failed to initialize FreeType\n");
	}

	FT_Face face;
	if (FT_New_Face(ft, "/usr/share/fonts/TTF/Arial.TTF", 0, &face)) {
		fprintf(stderr, "Failed to load font\n");
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenTextures(1, &textureAtlas);
	glBindTexture(GL_TEXTURE_2D, textureAtlas);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 1024, 0,
		GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float currentOffsetX = 0;
	float currentOffsetY = 0;
	float currentRowHeight = 0;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int c = 32; c < 127; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
			continue;
		}

		FT_GlyphSlot glyphSlot = face->glyph;
		FT_Bitmap bitmap = glyphSlot->bitmap;

		if (currentOffsetX + bitmap.width > 1024) {
			currentOffsetX = 0;
			currentOffsetY += currentRowHeight;
			currentRowHeight = 0;
		}

		float width = bitmap.width;
		float height = bitmap.rows;

		GLTglyph *glyph = glyphs + c;
		glyph->xMin = currentOffsetX;
		glyph->yMin = currentOffsetY;
		glyph->xMax = glyph->xMin + width;
		glyph->yMax = glyph->yMin + height;
		glyph->advance = glyphSlot->advance.x / 64.;
		glyph->bearingX = glyphSlot->bitmap_left;
		glyph->bearingY = glyphSlot->bitmap_top - height;

		glTexSubImage2D(GL_TEXTURE_2D, 0, currentOffsetX, currentOffsetY,
			width, height, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);

		currentOffsetX += bitmap.width;
		if (bitmap.rows > currentRowHeight) {
			currentRowHeight = bitmap.rows;
		}
	}

	while (!glfwWindowShouldClose(window)) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(gltProgram);

		gltOrtho(0, w, 0, h, -1, 1);
		char text[] = "Hello, world! This is a test.";

		GLTbuffer buffer = {0};

		float x = 0;
		float y = 0;

		char *at = text;
		while (*at) {
			int c = *at++;
			if (c < 128) {
				if (buffer.vertexCount >= buffer.maxVertexCount) {
					if (buffer.maxVertexCount == 0) {
						buffer.maxVertexCount = 1024;
					} else {
						buffer.maxVertexCount *= 2;
					}

					buffer.vertices = (GLfloat *)realloc(buffer.vertices, buffer.maxVertexCount * sizeof(*buffer.vertices));
				}

				if (buffer.indexCount >= buffer.maxIndexCount) {
					if (buffer.maxIndexCount == 0) {
						buffer.maxIndexCount = 1024;
					} else {
						buffer.maxIndexCount *= 2;
					}

					buffer.indices = (GLuint *)realloc(buffer.indices, buffer.maxIndexCount * sizeof(*buffer.indices));
				}

				GLTglyph *glyph = &glyphs[c];
				float width = glyph->xMax - glyph->xMin;
				float height = glyph->yMax - glyph->yMin;

				float *vertex = buffer.vertices + 4 * buffer.vertexCount;
				unsigned int *index = buffer.indices + buffer.indexCount;
				float xPos = x + glyph->bearingX;
				float yPos = y + glyph->bearingY;

				*vertex++ = xPos;
				*vertex++ = yPos;
				*vertex++ = glyph->xMin / 1024.;
				*vertex++ = glyph->yMax / 1024.;

				*vertex++ = xPos + width;
				*vertex++ = yPos;
				*vertex++ = glyph->xMax / 1024.;
				*vertex++ = glyph->yMax / 1024.;

				*vertex++ = xPos;
				*vertex++ = yPos + height;
				*vertex++ = glyph->xMin / 1024.;
				*vertex++ = glyph->yMin / 1024.;

				*vertex++ = xPos + width;
				*vertex++ = yPos + height;
				*vertex++ = glyph->xMax / 1024.;
				*vertex++ = glyph->yMin / 1024.;

				*index++ = buffer.vertexCount + 0;
				*index++ = buffer.vertexCount + 1;
				*index++ = buffer.vertexCount + 3;

				*index++ = buffer.vertexCount + 0;
				*index++ = buffer.vertexCount + 3;
				*index++ = buffer.vertexCount + 2;

				x += glyph->advance;
				buffer.vertexCount += 4;
				buffer.indexCount += 6;
			}
		}

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), buffer.vertices);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), buffer.vertices + 2);
		glDrawElements(GL_TRIANGLES, buffer.indexCount, GL_UNSIGNED_INT, buffer.indices);

		glfwSwapBuffers(window);
		glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
error:
	glfwTerminate();
	exit(EXIT_FAILURE);
}
