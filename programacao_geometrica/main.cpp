#include <iostream>
#include <cstdint>
#include <string.h>
#include <errno.h>
#include <chrono>
#include <thread>

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

#define WIDTH 860
#define HEIGHT 640

// P O Q e a origem, vetor u e v
#define MAX_LINES 2
#define MAX_VERTEX_COUNT MAX_LINES * 2 + 1 + 3
#define MAX_IDX_COUNT MAX_LINES * 2 + 1 + 3

const static char *vertex_shader_source = "#version 330 core\n"
  "layout (location = 0) in vec4 v_pos;\n"
  "layout (location = 1) in vec4 v_color;\n"
  "layout (location = 2) in float v_size;"
  "uniform mat4 v_transform;"
  "out vec4 color;\n"
  "void main()\n"
  "{\n"
  "   gl_Position = v_transform * v_pos;\n"
  "   gl_PointSize = v_size;\n"
  "   color = v_color\n;"
  "}\0";

// (color * v_color) * v_time
const static char *fragment_shader_source = "#version 330 core\n"
  "in vec4 color;\n"
  "out vec4 FragColor;\n"
  "void main()\n"
  "{\n"
  "   FragColor = color;\n"
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

void resize_callback(GLFWwindow* window, int width, int height) {
  glfwGetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);
}

Vec2 get_mouse_pos(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return Vec2{ .x = xpos, .y = ypos };
}

typedef struct {
  float x, y, z, w;
} Position;

typedef struct {
  float r, g, b, a;
} Color;

typedef struct {
  Position position;
  Color color;
  float size;
} Vertex;

typedef struct {
  uint32_t idxs[2]; // line vertexs
} Line;

uint32_t put_vertice(uint32_t idx, Vertex vertices[MAX_VERTEX_COUNT], Position pos, Color color) {
  vertices[idx].position = pos;
  vertices[idx].color = color;
  vertices[idx].size = 10.0f;
  return idx;
}

// mouse offset 1 -1
glm::vec3 mouse_to_gl_point(float x, float y) {
  return glm::vec3((2.0f * x) / WIDTH - 1.0f, 1.0f - (2.0f * y) / HEIGHT, 0.0f);
}

void draw(uint32_t VAO, uint32_t program, uint32_t idx, Vertex *vertices, uint32_t lidx, Line lines[MAX_LINES]) {
  glBindVertexArray(VAO);
  
  glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
  uint32_t v_model = glGetUniformLocation(program, "v_transform");
  glUniformMatrix4fv(v_model, 1, GL_FALSE, &model[0][0]);

  for (uint32_t i = 0; i < lidx; i++) {
    Line line = lines[i];
    glDrawArrays(GL_LINES, line.idxs[0], 2);
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
  
  Line lines[MAX_LINES];
  uint32_t lidx = 0;

  Vertex vertices[MAX_VERTEX_COUNT];
  uint32_t idx = 0;

  put_vertice(idx, vertices, (Position){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f }, (Color){ .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f });
  idx++; // put origin

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

  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size));
  glEnableVertexAttribArray(2); // location 2

  glBindBuffer(GL_ARRAY_BUFFER, 0); 

  glEnable(GL_PROGRAM_POINT_SIZE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);

  float start_time = glfwGetTime();
  float delta = 0.0f;
  float total_time = 0.0f;

  float frame_time = 1.0f / 30.0f;
  
  float click_time = 0.0f;
  float threshold = 0.3f; // threshold

  uint32_t total_click = 0;
  
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


    if (is_mouse_button_pressed(window, GLFW_MOUSE_BUTTON_LEFT)) {
      if (start_time - click_time > threshold) {

	click_time = start_time;
	glClearColor(0.99, 0.3, 0.3, 1.0);

	if (total_click < 3) {

	  glm::vec3 point = mouse_to_gl_point((float)mouse_pos.x, (float)mouse_pos.y);

	  switch (total_click) {
	  case 0: {
	    put_vertice(idx, vertices, (Position){ .x = point.x, .y = point.y, .z = 0.0f, .w = 1.0f }, (Color){ .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f });
	  } break;
	  case 1: {
	    put_vertice(idx, vertices, (Position){ .x = point.x, .y = point.y, .z = 0.0f, .w = 1.0f }, (Color){ .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f });
	  } break;
	  case 2: {
	    put_vertice(idx, vertices, (Position){ .x = point.x, .y = point.y, .z = 0.0f, .w = 1.0f }, (Color){ .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f });
	  } break;
	  }
	  idx++;

	  glBindBuffer(GL_ARRAY_BUFFER, VBO);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	}

	total_click++;
	std::cout << "mouse x: " << mouse_pos.x << std::endl;
	std::cout << "mouse y: " << mouse_pos.y << std::endl;
      }

    } else {
      // draw
      {
	glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      }
    }

    if (idx == 4) {
      // pula a origem
      Vertex P = vertices[1];
      Vertex O = vertices[2];
      Vertex Q = vertices[3];

      glm::vec3 p_pos = glm::vec3(P.position.x, P.position.y, 0.0f);
      glm::vec3 o_pos = glm::vec3(O.position.x, O.position.y, 0.0f);
      glm::vec3 q_pos = glm::vec3(Q.position.x, Q.position.y, 0.0f);

      std::cout << "P: " << glm::to_string(p_pos) << std::endl;
      std::cout << "O: " << glm::to_string(o_pos) << std::endl;
      std::cout << "Q: " << glm::to_string(q_pos) << std::endl;

      glm::vec3 u_coord = p_pos - o_pos;
      std::cout << "vector u: " << glm::to_string(u_coord) << std::endl;


      Vertex u = (Vertex){
	.position = (Position){
	  .x = u_coord.x,
	  .y = u_coord.y,
	  .z = 0.0f,
	  .w = 1.0f,
	},
	.color = (Color){
	  .r = P.color.r - O.color.r,
	  .g = P.color.r - O.color.r,
	  .b = P.color.r - O.color.r,
	  .a = 1.0f	
	}
      };

      uint32_t u_origem = put_vertice(idx, vertices, (Position){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f }, (Color){ .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f });
      idx++; // put origin
      uint32_t u1 = put_vertice(idx, vertices, u.position, u.color);
      idx++; // vertice

      lines[lidx] = (Line){
      	.idxs = { u_origem, u1 }
      };
      lidx++;
      
      glm::vec3 v_coord = q_pos - o_pos;
      std::cout << "vector v: " << glm::to_string(v_coord) << std::endl;
    
      Vertex v = (Vertex){
	.position = (Position){
	  .x = v_coord.x,
	  .y = v_coord.y,
	  .z = 0.0f,
	  .w = 1.0f,
	},
	.color = (Color){
	  .r = Q.color.r - O.color.r,
	  .g = Q.color.r - O.color.r,
	  .b = Q.color.r - O.color.r,
	  .a = 1.0f	
	}
      };

      uint32_t v_origem = put_vertice(idx, vertices, (Position){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f }, (Color){ .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f });
      idx++; // put origin
      uint32_t v1 = put_vertice(idx, vertices, v.position, v.color);
      idx++; // vertice
      
      lines[lidx] = (Line){
      	.idxs = { v_origem, v1 }
      };
      lidx++;
      

      // contas com os valores da screen
      
      float prod_interno = glm::dot(u_coord, v_coord);
      float lens = glm::length(u_coord) * glm::length(v_coord);

      float angle = glm::acos(prod_interno / lens);

      std::cout << "produto interno: " << prod_interno << std::endl;
      std::cout << "magnitude u: " << glm::length(u_coord) << std::endl;
      std::cout << "magnitude v: " << glm::length(v_coord) << std::endl;
      std::cout << prod_interno << "/(" << glm::length(u_coord) << "*" << glm::length(v_coord) << ") =" << " cosΘ" << std::endl;
      std::cout << "arc cos " << prod_interno / lens << " = " << angle << std::endl;

      //glm::vec3 prod_vetorial = glm::cross(u_coord, v_coord);

      glm::vec3 prod_vetorial = glm::vec3(
					  (u_coord.y * v_coord.z) - (u_coord.z * v_coord.y),
					  (v_coord.x * u_coord.z) - (u_coord.x * v_coord.z),
					  (u_coord.x * v_coord.y) - (u_coord.y * v_coord.x));
      
      std::cout << "u x v: " << glm::to_string(prod_vetorial) << std::endl;

      glm::vec3 q_o = p_pos - o_pos;
      glm::vec3 prod_v_qo = glm::cross(v_coord, q_o);

      // area do paralelogramo e igual a norma do produto vetorial
      // b*h = |v_coord x q_o|
      // b = norma do vetor v
      // h = |v_coord x q_o| / |v|
      // h e a distancia do de v do ponto P
      
      //std::cout << "glm u x v: " << glm::to_string(glm_prod_vetorial) << std::endl;
      std::cout << "distancia do ponto P de v: " << glm::length(prod_v_qo) / glm::length(v_coord) << std::endl;

      // marca que 4 pontos foram adicionados para o opengl
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    }
    

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    //glBindVertexArray(VAO);
    draw(VAO, program, idx, vertices, lidx, lines);
    glDrawArrays(GL_POINTS, 0, idx);

    
    // std::cout << "total clicks: " << total_click << std::endl;
    //std::cout << "total vertices: " << idx << std::endl;
    // std::cout << "total lines: " << lidx << std::endl;
    
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
