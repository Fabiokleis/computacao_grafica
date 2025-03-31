#include <iostream>
#include <string.h>
#include <errno.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


bool is_key_pressed(GLFWwindow *window, int keycode) {
    int state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool is_mouse_button_pressed(GLFWwindow *window, int button) {
    int state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

void resize_callback(GLFWwindow* window, int width, int height) {
  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);
}

void draw(GLFWwindow* window) {
  glClearColor(0.2, 0.3, 0.3, 1.0);

  /* Clears color buffer to the RGBA defined values. */
  glClear(GL_COLOR_BUFFER_BIT);


  /* Demand to draw to the window.*/
  glfwSwapBuffers(window);
}

void loop(GLFWwindow *window) {

  int width, height;
  bool quit = false;
  glfwSetFramebufferSizeCallback(window, resize_callback);

  while (!quit) {
    glfwPollEvents();
    quit = is_key_pressed(window, GLFW_KEY_ESCAPE) || glfwWindowShouldClose(window);
    glfwGetWindowSize(window, &width, &height);
    std::cout << "width: " << width << std::endl;
    std::cout << "height: " << height << std::endl;
    draw(window);
  }
}

int main(void) {
  std::cout << "hello, world!" << std::endl;

  if (!glfwInit()) {
    std::cout << "Could not initialize glfw!\n";
    std::cout << "error: " << strerror(errno);
    exit(1);
  }

  glfwInitHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwInitHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwInitHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  const char *title = "main";

  GLFWwindow *window = glfwCreateWindow(640, 480, title, nullptr, nullptr);
  
  if (window == nullptr) {
    glfwTerminate();
    std::cout << "Could not create glfw window!\n";
    std::cout << "error: " << strerror(errno);
    exit(1);
  }

  glfwMakeContextCurrent(window);
  
  uint32_t err = glewInit();
  if (GLEW_OK != err) {
    std::cout << "GLEW initialization error!\n" ;
    std::cout << "error: " << strerror(errno);
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
