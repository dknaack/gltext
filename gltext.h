#ifndef GLTEXT_H
#define GLTEXT_H

#ifndef GLT_API
#define GLT_API extern
#endif /* GLT_API */

typedef struct {
	GLuint codepoint;
	float xMin, xMax;
	float yMin, yMax;
	float bearingX;
	float bearingY;
	float advance;
} GLTglyph;

typedef struct {
	float currentOffsetX;
	float currentOffsetY;
	float currentRowHeight;
	GLuint textureAtlas;
	GLTglyph glyphs[256];
} GLTcache;

typedef struct {
	GLfloat *vertices;
	GLsizei maxVertexCount;
	GLsizei vertexCount;

	GLuint *indices;
	GLsizei maxIndexCount;
	GLsizei indexCount;
} GLTbuffer;

GLT_API void gltPushText(GLTbuffer *b, char *text);
GLT_API void gltDrawText(char *text);
GLT_API void gltDraw(GLTbuffer b);

GLT_API GLuint gltCreateFont(char *filename, int pixelSize);
GLT_API void gltBindFont(GLuint font);

GLT_API void gltOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
GLT_API void gltSetTransform(float *matrix, GLboolean transpose);
GLT_API void gltUseProgram(void);

#endif /* GLTEXT_H */

#ifdef GLT_IMPL

static GLuint gltProgram;
static FT_Face gltFonts[256];
static GLuint gltFontCount = 1;
static GLuint gltCurrentFont;
static GLTcache gltGlobalCache = {0};

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

GLT_API void
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

GLT_API void
gltSetTransform(float *matrix, GLboolean transpose)
{
	gltUseProgram();

	GLuint transformLocation = glGetUniformLocation(gltProgram, "transform");
	glUniformMatrix4fv(transformLocation, 1, transpose, matrix);
}

GLT_API void
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

	gltSetTransform(matrix[0], GL_TRUE);
}

GLT_API void
gltDraw(GLTbuffer b)
{
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), b.vertices);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), b.vertices + 2);
	glDrawElements(GL_TRIANGLES, b.indexCount, GL_UNSIGNED_INT, b.indices);

	b.vertexCount = 0;
	b.indexCount = 0;
}

GLT_API void
gltPushText(GLTbuffer *b, char *text)
{
	if (gltCurrentFont == 0) {
		return;
	}

	GLTcache *cache = &gltGlobalCache;
	if (!cache->textureAtlas) {
		glGenTextures(1, &cache->textureAtlas);
		glBindTexture(GL_TEXTURE_2D, cache->textureAtlas);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 1024, 0,
			GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	FT_Face face = gltFonts[gltCurrentFont];
	for (int c = 32; c < 127; c++) {
		if (cache->glyphs[c].codepoint != 0) {
			continue;
		}

		if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
			continue;
		}

		FT_GlyphSlot glyphSlot = face->glyph;
		FT_Bitmap bitmap = glyphSlot->bitmap;

		if (cache->currentOffsetX + bitmap.width > 1024) {
			cache->currentOffsetX = 0;
			cache->currentOffsetY += cache->currentRowHeight;
			cache->currentRowHeight = 0;
		}

		float width = bitmap.width;
		float height = bitmap.rows;

		GLTglyph *glyph = cache->glyphs + c;
		glyph->codepoint = c;
		glyph->xMin = cache->currentOffsetX;
		glyph->yMin = cache->currentOffsetY;
		glyph->xMax = glyph->xMin + width;
		glyph->yMax = glyph->yMin + height;
		glyph->advance = glyphSlot->advance.x / 64.;
		glyph->bearingX = glyphSlot->bitmap_left;
		glyph->bearingY = glyphSlot->bitmap_top - height;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, cache->currentOffsetX, cache->currentOffsetY,
			width, height, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);

		cache->currentOffsetX += bitmap.width;
		if (bitmap.rows > cache->currentRowHeight) {
			cache->currentRowHeight = bitmap.rows;
		}
	}

	float x = 0;
	float y = 0;
	char *at = text;
	while (*at) {
		int c = *at++;
		if (c < 128) {
			if (b->vertexCount >= b->maxVertexCount) {
				if (b->maxVertexCount == 0) {
					b->maxVertexCount = 1024;
				} else {
					b->maxVertexCount *= 2;
				}

				b->vertices = (GLfloat *)realloc(b->vertices, b->maxVertexCount * sizeof(*b->vertices));
			}

			if (b->indexCount >= b->maxIndexCount) {
				if (b->maxIndexCount == 0) {
					b->maxIndexCount = 1024;
				} else {
					b->maxIndexCount *= 2;
				}

				b->indices = (GLuint *)realloc(b->indices, b->maxIndexCount * sizeof(*b->indices));
			}

			GLTglyph *glyph = &cache->glyphs[c];
			float width = glyph->xMax - glyph->xMin;
			float height = glyph->yMax - glyph->yMin;

			float *vertex = b->vertices + 4 * b->vertexCount;
			unsigned int *index = b->indices + b->indexCount;
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

			*index++ = b->vertexCount + 0;
			*index++ = b->vertexCount + 1;
			*index++ = b->vertexCount + 3;

			*index++ = b->vertexCount + 0;
			*index++ = b->vertexCount + 3;
			*index++ = b->vertexCount + 2;

			x += glyph->advance;
			b->vertexCount += 4;
			b->indexCount += 6;
		}
	}
}

GLT_API void
gltDrawText(char *text)
{
	GLTbuffer b = {0};
	gltPushText(&b, text);
	gltDraw(b);
	free(b.vertices);
	free(b.indices);
}

GLT_API GLuint
gltCreateFont(char *filename, int pixelSize)
{
	static FT_Library ft;
	static GLboolean isInitialized;
	if (!isInitialized) {
		if (FT_Init_FreeType(&ft)) {
			fprintf(stderr, "Failed to initialize FreeType\n");
		}
	}

	if (gltFontCount > 256) {
		return 0;
	}

	GLuint id = gltFontCount++;
	FT_Face *face = &gltFonts[id];
	if (FT_New_Face(ft, filename, 0, face)) {
		fprintf(stderr, "Failed to load font\n");
	}

	FT_Set_Pixel_Sizes(*face, 0, pixelSize);
	return id;
}

GLT_API void
gltBindFont(GLuint font)
{
	gltCurrentFont = font;
}

#endif /* GLT_IMPL */
