/* This is free and unencumbered software released into the public domain. */

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <math.h>

#define GLAD_GL_IMPLEMENTATION
#define GLT_IMPL
#include "glad.h"
#include "gltext.h"

static void errorCallback(int err, const char *desc);
static void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods);

static const char *title = "Simple example";

static void
errorCallback(int err, const char *desc)
{
    fprintf(stderr, "GLFW Error: %s\n", desc);
}

static void
keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(win, GLFW_TRUE);
	}
}

int main(void)
{
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
		return 1;
	}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(640, 480, title, NULL, NULL);
    if (!window) {
		glfwTerminate();
		return 1;
	}

	glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);
	gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    glfwSwapInterval(1);

	float fontHeight = 48;
	GLuint font = gltCreateFont("OpenSans.ttf", fontHeight);
	GLTbuffer buffer = {0};

	while (!glfwWindowShouldClose(window)) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		char *text = "Hello, world!";
		float textWidth = gltMeasureTextWidth(text);

		float x = floor(0.5f * (w - textWidth));
		float y = floor(0.5f * (h - fontHeight));
		gltBindFont(font);
		gltPushText(&buffer, x, y, text);
		gltDrawBuffer(buffer);

		glfwSwapBuffers(window);
		glfwPollEvents();
    }

	free(buffer.vertices);
	free(buffer.indices);
    glfwDestroyWindow(window);
    glfwTerminate();
	return 0;
}
