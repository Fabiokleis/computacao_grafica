#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MOUSE_ICON_FILE "mouse_icon.png"

typedef struct {
  double x, y;
} Vec2;

bool is_key_pressed(GLFWwindow *window, int keycode) {
    int state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool is_mouse_button_pressed(GLFWwindow *window, int button) {
    int state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

bool should_quit(GLFWwindow *window) {
  return is_key_pressed(window, GLFW_KEY_ESCAPE) || is_key_pressed(window, GLFW_KEY_Q) || glfwWindowShouldClose(window);
}

Vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return Vec2{ .x = xpos, .y = ypos };
}

void resize_callback(GLFWwindow* window, int width, int height) {
  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);
}

void loop(GLFWwindow *window) {

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  
  int width = 0;
  int height = 0;
  Vec2 mouse_pos = {0};

  int m_width = 0;
  int m_height = 0;
  int channels_in_file = 0;
  
  uint8_t *image_buffer = stbi_load(MOUSE_ICON_FILE, &m_width, &m_height, &channels_in_file, 4);
  if (image_buffer == nullptr) {
    std::cerr << "Could not load image!" << std::endl;
    std::cerr << "STB Image Error: " << stbi_failure_reason() << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }
 
  GLFWimage image;
  image.width = m_width;
  image.height = m_height;
  image.pixels = image_buffer;
  
  GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
  if (cursor == nullptr) {
    std::cerr << "Could not create glfw cursor!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  if (image_buffer) {
    stbi_image_free(image_buffer);
  }
  
  glfwSetCursor(window, cursor);

  
  bool quit = false;
  glfwSetFramebufferSizeCallback(window, resize_callback);

  while (!quit) {
    glfwPollEvents();

    quit = should_quit(window);
    //glfwSwapInterval(1);
    glfwGetWindowSize(window, &width, &height);
    mouse_pos = get_mouse_pos(window);
    // std::cout << "width: " << width << std::endl;
    // std::cout << "height: " << height << std::endl;
    // std::cout << "x: " << mouse_pos.x << std::endl;
    // std::cout << "y: " << mouse_pos.y << std::endl;

    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      glClearColor(0.99, 0.3, 0.3, 1.0);

      /* Clears color buffer to the RGBA defined values. */
      glClear(GL_COLOR_BUFFER_BIT);

      /* Demand to draw to the window.*/
      glfwSwapBuffers(window);      
    } else {
      // draw
      {
	glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);

	/* Clears color buffer to the RGBA defined values. */
	glClear(GL_COLOR_BUFFER_BIT);
      
	/* Demand to draw to the window.*/
	glfwSwapBuffers(window);
      }
    

    }
  }
  glfwDestroyCursor(cursor);
}

int main(void) {
  std::cout << "hello, world!" << std::endl;

  if (!glfwInit()) {
    std::cerr << "Could not initialize glfw!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  glfwInitHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwInitHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwInitHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  const char *title = "main.cpp - pizza";

  GLFWwindow *window = glfwCreateWindow(640, 480, title, nullptr, nullptr);
  
  if (window == nullptr) {
    glfwTerminate();
    std::cerr << "Could not create glfw window!" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    exit(1);
  }

  glfwMakeContextCurrent(window);
  
  uint32_t err = glewInit();
  if (GLEW_OK != err) {
    std::cerr << "GLEW initialization error!" << std::endl;
    std::cerr << "error: " << strerror(errno);
  }

  std::cout << glGetString(GL_VERSION) << std::endl;
  std::cout << glGetString(GL_RENDERER) << std::endl;
  std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  loop(window);

  glfwTerminate();
  return 0;
}
