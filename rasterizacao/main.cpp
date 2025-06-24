#include <glm/ext/vector_float4.hpp>
#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>
#include <chrono>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#define MOUSE_ICON_FILE "../mouse_icon.png"

#define WIDTH 1280
#define HEIGHT 900

#define MAX_TRIANGLES 1000
#define MAX_VERTEX_COUNT MAX_TRIANGLES * 3
#define MAX_IDX_COUNT MAX_TRIANGLES * 3

const static char *vertex_shader_source = R"(
  #version 330 core
  layout (location = 0) in vec4 v_pos;
  layout (location = 1) in vec4 v_color;
  uniform mat4 v_transform;
  uniform mat4 v_proj;
  out vec4 color;
  void main()
  {
     gl_Position = v_transform * v_pos;
     color = v_color;
  }
)";

const static char *fragment_shader_source = R"(
  #version 330 core
  in vec4 color;
  uniform vec4 v_bord_color;
  out vec4 FragColor;
  void main()
  {
       FragColor = color;

  }
)";

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

void resize_callback(GLFWwindow* window, int width, int height) {
  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);
}

glm::vec3 scale = glm::vec3(1.0f);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  scale += (yoffset * .5f);
  std::cout << "scale: " << glm::to_string(scale) << std::endl;
}

Vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return Vec2{ .x = xpos, .y = ypos };
}

typedef struct {
  glm::vec4 position;
  glm::vec4 color;
} Vertex;

typedef struct {
  std::vector<int> pixels;
  std::vector<uint32_t> idxs;
  glm::vec3 translate;
  glm::vec3 scale;
  glm::vec4 color;
} Circle;

uint32_t put_vertice(uint32_t idx, Vertex vertices[MAX_VERTEX_COUNT], glm::vec4 pos, glm::vec4 color) {
  vertices[idx].position = pos;
  vertices[idx].color = color;
  return idx;
}

void print_vertex(Vertex v) {
  std::cout << "vertex: " << glm::to_string(v.position) << std::endl;
}

// mouse offset 1 -1
glm::vec3 mouse_to_gl_point(float x, float y) {
  return glm::vec3((2.0f * x) / WIDTH - 1.0f, 1.0f - (2.0f * y) / HEIGHT, 0.0f);
}

uint32_t add_pixel(Vertex *vertices, uint32_t idx, int x, int y, Circle *c) {
    c->pixels.push_back(x);
    c->pixels.push_back(y);
    c->idxs.push_back(idx);
    glm::vec3 pos = mouse_to_gl_point(x, y);
    put_vertice(idx, vertices, glm::vec4(pos.x, pos.y, 0.0f, 1.0f), c->color);
    idx++;
    return idx;
}

uint32_t plot_circle_points(Vertex *vertices, uint32_t idx, Circle *c, int centerX, int centerY, int x, int y) {
  idx = add_pixel(vertices, idx, centerX + x, centerY + y, c);
  idx = add_pixel(vertices, idx, centerX - x, centerY + y, c);
  idx = add_pixel(vertices, idx, centerX + x, centerY - y, c);
  idx = add_pixel(vertices, idx, centerX - x, centerY - y, c);
  idx = add_pixel(vertices, idx, centerX + y, centerY + x, c);
  idx = add_pixel(vertices, idx, centerX - y, centerY + x, c);
  idx = add_pixel(vertices, idx, centerX + y, centerY - x, c);
  idx = add_pixel(vertices, idx, centerX - y, centerY - x, c);
  return idx;
}

uint32_t midpointCircle(Vertex *vertices, uint32_t idx, Circle *c, int centerX, int centerY, int radius) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;

    idx = plot_circle_points(vertices, idx, c, centerX, centerY, x, y);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * x + 1 - 2 * y;
        }
        idx = plot_circle_points(vertices, idx, c, centerX, centerY, x, y);
    }
    return idx;
}

void print_circle(Circle c) {
  std::cout <<  "circle: " << std::endl;
  std::cout << c.idxs.size() << std::endl;
  std::cout << c.pixels.size() << std::endl;
  std::cout <<  "end" << std::endl;
}

void draw_triangles(uint32_t VAO, uint32_t program, Circle c) {
  if (!c.pixels.empty()) {
    print_circle(c);
    glPointSize(2.0f);
    int v_transform = glGetUniformLocation(program, "v_transform");
    int v_proj = glGetUniformLocation(program, "v_proj");

    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, -1.0f, 1.0f);
    
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), c.translate);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), c.scale);
    glm::mat4 transform = translate * scale;
    int v_bord_color = glGetUniformLocation(program, "v_bord_color");
    
    glUniformMatrix4fv(v_transform, 1, GL_FALSE, &transform[0][0]);
    glUniformMatrix4fv(v_proj, 1, GL_FALSE, &projection[0][0]);
    glUniform4f(v_bord_color, -1.0f, -1.0f, -1.0f, -1.0f);
    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_POINTS, c.idxs[0], c.idxs.size());
    //glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
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
  glfwSetScrollCallback(window, scroll_callback);

  uint32_t program;
  int error = compile_shaders(&program);
  if (error != 0) exit(1);
  
  Vertex vertices[MAX_VERTEX_COUNT];
  uint32_t idx = 0;
  Circle circle;

  glm::vec3 translate = glm::vec3(0.0f, 0.f, 0.f);
  uint32_t VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
  glEnableVertexAttribArray(0); // location 0

  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
  glEnableVertexAttribArray(1); // location 1

  glBindBuffer(GL_ARRAY_BUFFER, 0); 

  float start_time = glfwGetTime();
  float delta = 0.0f;
  float total_time = 0.0f;

  float frame_time = 1.0f / 30.0f;
  
  float click_time = 0.0f;
  float threshold = 0.3f; // threshold

  uint32_t total_click = 0;

  uint32_t mode = GL_FILL;

  while (!quit) {
    delta = glfwGetTime() - start_time;
    total_time += delta;
    if (delta < frame_time) {
      double sleep_time = frame_time - delta;
      std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
    }
    start_time = glfwGetTime();

    
    quit = should_quit(window);
    //glfwSwapInterval(1);
    glfwGetWindowSize(window, &width, &height);
    mouse_pos = get_mouse_pos(window);

  
    if (is_key_pressed(window, GLFW_KEY_LEFT)) {
      translate.x -= 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_RIGHT)) {
      translate.x += 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_UP)) {
      translate.y += 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_DOWN)) {
      translate.y -= 0.05f;
      std::cout << "translated: " << glm::to_string(translate) << std::endl;      
    } else if (is_key_pressed(window, GLFW_KEY_1)) {
      if (start_time - click_time > threshold) {
	click_time = start_time;
      
      }
    }

    circle.translate = translate;
    circle.scale = scale;
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_RIGHT)) {
      if (start_time - click_time > threshold) {
	click_time = start_time;
	//if (mode == GL_FILL) mode = GL_LINE;
	//else mode = GL_FILL;
	glClearColor(0.99, 0.3, 0.3, 1.0);
	//glPolygonMode(GL_FRONT_AND_BACK, mode);

	total_click++;
	std::cout << "mouse x: " << mouse_pos.x << std::endl;
	std::cout << "mouse y: " << mouse_pos.y << std::endl;
      }

    } else if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      
      if (start_time - click_time > threshold) {
	click_time = start_time;

	total_click++;
	std::cout << "mouse x: " << mouse_pos.x << std::endl;
	std::cout << "mouse y: " << mouse_pos.y << std::endl;
	glm::vec3 point = mouse_to_gl_point((float)mouse_pos.x, (float)mouse_pos.y);
	glm::vec4 position = glm::vec4((float)point.x, (float)point.y, 0.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.f);

	idx = 0;
	circle.idxs.clear();
	circle.pixels.clear();
	circle.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	idx = midpointCircle(vertices, idx, &circle, (int)mouse_pos.x, (int)mouse_pos.y, 150);

      }
    }
    else {
      // draw
      {
	//glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);

	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    glUseProgram(program);

    //glBindVertexArray(VAO);
    draw_triangles(VAO, program, circle);
    
    //std::cout << "total clicks: " << total_click << std::endl;
    std::cout << "total vertices: " << idx << std::endl;
    //print_circle(circle);
    //std::cout << "total polys: " << polys.size() << std::endl;
    
    glfwSwapBuffers(window);
    glfwPollEvents();
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
