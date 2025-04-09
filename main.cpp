#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MOUSE_ICON_FILE "mouse_icon.png"

#define WIDTH 640
#define HEIGHT 480


const static char *vertex_shader_source = "#version 330 core\n"
  "layout (location = 0) in vec4 pos;\n"
  "layout (location = 1) in vec4 v_color;\n"
  "uniform mat4 v_translate;"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  "   gl_Position = vec4(v_translate * pos);\n"
      "color = v_color\n;"
  "}\0";

// (color * v_color) * v_time
const static char *fragment_shader_source = "#version 330 core\n"
  "in vec4 color;\n"
  "out vec4 FragColor;\n"
  "uniform vec4 v_color;\n"
  "uniform float v_time;\n"
  "void main()\n"
  "{\n"
  "   FragColor = vec4((color * v_color) * v_time);\n"
  "}\n\0";

int compile_shaders(uint32_t *shader_program) {

  // vertex shader
  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  // check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
      return -1;
    }
  // fragment shader
  uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  // check for shader compile errors
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
      return -1;
    }
  // link shaders
  *shader_program = glCreateProgram();
  glAttachShader(*shader_program, vertex_shader);
  glAttachShader(*shader_program, fragment_shader);
  glLinkProgram(*shader_program);
  // check for linking errors
  glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(*shader_program, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    return -1;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return 0;
}


typedef struct {
  double x, y;
} Vec2;

typedef enum {
  R,
  G,
  B,
  A
} ColorChannel;

typedef struct {
  float x, y, z, w;
} Position;

typedef struct {
  float r, g, b, a;
} Color;

typedef struct {
  Position position;
  Color color;
} Vertex;


typedef struct {
  uint32_t idxs[3]; // triangle vertexs
  glm::mat4 translation;
} Triangle;

bool is_key_pressed(GLFWwindow *window, int keycode) {
    int state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS; // || state == GLFW_REPEAT;
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

#define MAX_TRIANGLES 1000
#define MAX_VERTEX_COUNT MAX_TRIANGLES * 3
#define MAX_IDX_COUNT MAX_TRIANGLES * 3

uint32_t put_vertice(uint32_t idx, Vertex vertices[MAX_VERTEX_COUNT], Position pos, Color color) {
  vertices[idx].position = pos;
  vertices[idx].color = color;
  return idx;
}

Triangle put_triangle(uint32_t *idx, Vertex vertices[MAX_VERTEX_COUNT], Vec2 mouse_pos, Color color) {
  uint32_t idx_v1 = put_vertice(*idx, vertices, (Position){ .x = -0.2, .y = -0.2, .z = 0.0f, .w = 1.0f }, color);
  (*idx)++;
  uint32_t idx_v2 = put_vertice(*idx, vertices, (Position){ .x = 0.2, .y = -0.2, .z = 0.0f, .w = 1.0f }, color);
  (*idx)++;
  uint32_t idx_v3 = put_vertice(*idx, vertices, (Position){ .x = 0.0f, .y = 0.2, .z = 0.0f, .w = 1.0f }, color);
  (*idx)++;

  float x = (((float)mouse_pos.x - (WIDTH/2) + (WIDTH * 0.2f)) / WIDTH);
  float y = (((float)mouse_pos.y - (HEIGHT/2) + (WIDTH * 0.2f)) / HEIGHT);


  return (Triangle){
    .idxs = { idx_v1, idx_v2, idx_v3 },
    .translation = glm::translate(glm::mat4(1.0f), glm::vec3(x, -y, 0.0f)),
  };
}

void draw_triangles(uint32_t VAO, uint32_t program, Vertex *vertices, uint32_t tidx, Triangle triangles[MAX_TRIANGLES]) {

  for (uint32_t i = 0; i < tidx; ++i) {
    Triangle triangle = triangles[i];
    
    int v_translate = glGetUniformLocation(program, "v_translate");
    glUniformMatrix4fv(v_translate, 1, GL_FALSE, &triangle.translation[0][0]);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
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


  uint32_t program;
  int error = compile_shaders(&program);
  if (error != 0) exit(1);


  Triangle triangles[MAX_TRIANGLES];
  uint32_t tidx = 0;

  uint32_t idx = 0;

  Vertex vertices[MAX_VERTEX_COUNT];

  Color color = (Color){ .r = 0.5f, .g = 0.5f, .b = 0.5f, .a = 1.0f };
  
  uint32_t VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0); 


  float start_time = glfwGetTime();
  float delta = 0.0f;
  float total_time = 0.0f;
  float cycle_time = 0.0f;

  ColorChannel selected = R;
  
  while (!quit) {
    glfwPollEvents();

    if (cycle_time >= 4.0f) cycle_time = 0.0f;

    quit = should_quit(window);
    //glfwSwapInterval(1);
    glfwGetWindowSize(window, &width, &height);
    mouse_pos = get_mouse_pos(window);

    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      glClearColor(0.99, 0.3, 0.3, 1.0);
      selected = (ColorChannel)((selected + 1) % 4);

      if (tidx < MAX_TRIANGLES) {
	Triangle triangle = put_triangle(&idx, vertices, mouse_pos, color);
	triangles[tidx++] = triangle;
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
      }

    } else {
      // draw
      {
	glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);
      }
    }
    
    glClear(GL_COLOR_BUFFER_BIT);
  
    /* Clears color buffer to the RGBA defined values. */

    delta = glfwGetTime() - start_time;

    start_time = glfwGetTime();
    total_time += delta;
    cycle_time += delta;
    
    float intensity_value = (sin(cycle_time) / 2.0f) + 0.5f;
    int v_color_location = glGetUniformLocation(program, "v_color");
    int v_time_color_location = glGetUniformLocation(program, "v_time");

    glUseProgram(program);

    switch (selected) {
    case R:
      color.r = intensity_value;
      break;
    case G:
      color.g = intensity_value;
      break;
    case B:
      color.b = intensity_value;
      break;
    case A:
      color.a = intensity_value;
      break;
    }
    glUniform4f(v_color_location, color.r, color.g, color.b, color.a);
    glUniform1f(v_time_color_location, cycle_time);

    /* draw all triangles created */

    
    /* Demand to draw to the window.*/
    //glUseProgram(program);
    //glBindVertexArray(VAO);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    //glDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_INT, 0);

    draw_triangles(VAO, program, vertices, tidx, triangles);
    
    std::cout << "selected channel: " << selected << std::endl;
    std::cout << "mouse x:" << mouse_pos.x << std::endl;
    std::cout << "mouse y:" << mouse_pos.y << std::endl;
    std::cout << "total triangles: " << tidx << std::endl;
    //std::cout << "cicle time: " << cycle_time << std::endl;
    
    glfwSwapBuffers(window);
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

  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, title, nullptr, nullptr);
  
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
