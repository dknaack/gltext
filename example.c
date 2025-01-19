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
    fprintf(stderr, "Error: %s\n", desc);
}

static void
keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
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

	float fontHeight = 48;
	GLuint font = gltCreateFont("OpenSans.ttf", fontHeight);

	while (!glfwWindowShouldClose(window)) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		char text[] = "Hello, world!";
		float textWidth = gltMeasureTextWidth(text);

		float x = floor(0.5f * (w - textWidth));
		float y = floor(0.5f * (h - fontHeight));
		GLTbuffer buffer = {0};
		gltBindFont(font);
		gltPushText(&buffer, x, y, text);
		gltDraw(buffer);

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
