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
     if (v_bord_color.w > 0.0f) {
       FragColor = v_bord_color;
     } else {
       FragColor = color;
     }

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
  std::vector<uint32_t> idxs;
  glm::vec3 translate;
  glm::vec3 scale;
} PolyGon;

uint32_t put_vertice(uint32_t idx, Vertex vertices[MAX_VERTEX_COUNT], glm::vec4 pos, glm::vec4 color) {
  vertices[idx].position = pos;
  vertices[idx].color = color;
  return idx;
}

enum Edge { LEFT, RIGHT, BOTTOM, TOP };

bool point_inside(glm::vec2 p, Edge e, glm::vec2 e_min, glm::vec2 e_max) {
  switch (e) {
  case LEFT: return p.x >= e_min.x;
  case RIGHT: return p.x <= e_max.x;
  case BOTTOM: return p.y >= e_min.y;
  case TOP: return p.y <= e_max.y;
  default:
    return false;
  }
}

// Compute intersection of line segment (v1-v2) with clip edge
glm::vec2 intersec(glm::vec2 v1, glm::vec2 v2, Edge e, glm::vec2 e_min, glm::vec2 e_max) {
  float dx = v2.x - v1.x;
  float dy = v2.y - v1.y;
  float slope = dx / dy;
  
  switch (e) {
  case LEFT:   return glm::vec2(e_min.x, v1.y + (e_min.x - v1.x) / slope);
  case RIGHT:  return glm::vec2(e_max.x, v1.y + (e_max.x - v1.x) / slope);
  case BOTTOM: return glm::vec2(v1.x + (e_min.y - v1.y) * slope, e_min.y);
  case TOP:    return glm::vec2(v1.x + (e_max.y - v1.y) * slope, e_max.y);
  default:     return v1;
  }
}

typedef struct {
  uint32_t idx;
  Vertex vertex;
} Ivertex;

std::vector<Vertex> clip(Vertex v1, std::vector<Vertex> vs, Edge e, glm::vec2 e_min, glm::vec2 e_max) {
  std::vector<Vertex> pv;
  
  for (Vertex v2 : vs) {
    if (point_inside(glm::vec2(v2.position.x, v2.position.y), e, e_min, e_max)) {
      if (!point_inside(v1.position, e, e_min, e_max)) {
	glm::vec2 pos = intersec(glm::vec2(v1.position.x, v1.position.y), glm::vec2(v2.position.x, v2.position.y), e, e_min, e_max);
	pv.push_back((Vertex){ .position = glm::vec4(pos.x, pos.y, 0.0f, 1.0f), .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });
	
      }
      pv.push_back(v2);
    } else if (point_inside(glm::vec2(v1.position.x, v1.position.y), e, e_min, e_max)) {
      glm::vec2 pos = intersec(glm::vec2(v1.position.x, v1.position.y), glm::vec2(v2.position.x, v2.position.y), e, e_min, e_max);
      pv.push_back((Vertex){ .position = glm::vec4(pos.x, pos.y, 0.0f, 1.0f), .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });
    }
    v1 = v2;
  }

  return pv;
}

PolyGon sutherland_hodgman(uint32_t idx, Vertex *vertices, PolyGon p, glm::vec2 e_min, glm::vec2 e_max) {
  PolyGon p_out;
  std::vector<Vertex> verts;

  p_out.translate = p.translate;
  p_out.scale = p.scale;
  
  for (uint32_t i = 0; i < p.idxs.size(); i++) {
    verts.push_back(vertices[p.idxs[i]]);
  }

  verts = clip(verts.back(), verts, LEFT, e_min, e_max);
  verts = clip(verts.back(), verts, RIGHT, e_min, e_max);
  verts = clip(verts.back(), verts, BOTTOM, e_min, e_max);
  verts = clip(verts.back(), verts, TOP, e_min, e_max);

  std::cout << verts.size() << std::endl;
  Vertex v1 = verts[0];

  v1.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  
  p_out.idxs.push_back(idx);
  vertices[idx] = verts[0];
  idx++;

  p_out.idxs.push_back(idx);
  verts[1].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  vertices[idx] = verts[1];
  idx++;

  p_out.idxs.push_back(idx);
  verts[2].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  vertices[idx] = verts[2];
  idx++;

  Vertex last = verts[2];
  for (uint32_t i = p_out.idxs.size()-1; i < verts.size(); ++i) {
    p_out.idxs.push_back(idx);
    vertices[idx] = v1;
    idx++;
    p_out.idxs.push_back(idx);
    vertices[idx] = last;
    idx++;
    p_out.idxs.push_back(idx);
    verts[i].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    vertices[idx] = verts[i];
    idx++;
    last = verts[i];
  }
  
  return p_out;
}


void print_vertex(Vertex v) {
  std::cout << "vertex: " << glm::to_string(v.position) << std::endl;
}


void print_polygon(Vertex *vertices, uint32_t idx, PolyGon p) {
  std::cout << "polygon: " << std::endl;
  for (uint32_t i = 0; i < p.idxs.size(); i++) {
    print_vertex(vertices[p.idxs[i]]);
  }
  std::cout << "finish polygon" << std::endl;
}

// mouse offset 1 -1
glm::vec3 mouse_to_gl_point(float x, float y) {
  return glm::vec3((2.0f * x) / WIDTH - 1.0f, 1.0f - (2.0f * y) / HEIGHT, 0.0f);
}

void draw_triangles(uint32_t VAO, uint32_t program, std::vector<PolyGon> poly) {
  for (const auto p : poly) {
    int v_transform = glGetUniformLocation(program, "v_transform");
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), p.translate);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), p.scale);
    glm::mat4 transform = translate * scale;
    int v_bord_color = glGetUniformLocation(program, "v_bord_color");
    
    glUniformMatrix4fv(v_transform, 1, GL_FALSE, &transform[0][0]);
    glUniform4f(v_bord_color, -1.0f, -1.0f, -1.0f, -1.0f);
    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, p.idxs[0], p.idxs.size());
  }
  //glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
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
  std::vector<PolyGon> polys;

  glm::vec3 translate = glm::vec3(0.0f, 0.f, 0.f);
  //glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

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

  std::vector<Vertex> vs;
  PolyGon f;
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
      
	print_polygon(vertices, idx, f);
	PolyGon rect = polys[0];
	Vertex min = vertices[rect.idxs[0]];
	Vertex max = vertices[rect.idxs[2]];
	glm::vec4 min_pos = (min.position * rect.scale.x) + glm::vec4(rect.translate.x, rect.translate.y, 0.0f, 0.0f);
	glm::vec4 max_pos = (max.position * rect.scale.x) + glm::vec4(rect.translate.x, rect.translate.y, 0.0f, 0.0f);
	PolyGon out = sutherland_hodgman(idx, vertices, f, glm::vec2(min_pos.x, min_pos.y), glm::vec2(max_pos.x, max_pos.y));
	print_polygon(vertices, idx, out);
	polys[polys.size() - 1] = out;
	 
	f = (PolyGon){
	  .idxs = {},
	  .translate = glm::vec3(0.0f),
	  .scale = glm::vec3(1.0f),
	};
	vs.clear();
      }
    }

    
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
      std::cout << "polys size: " << polys.size() << std::endl;
      if (start_time - click_time > threshold) {
	click_time = start_time;

	total_click++;
	std::cout << "mouse x: " << mouse_pos.x << std::endl;
	std::cout << "mouse y: " << mouse_pos.y << std::endl;
	glm::vec3 point = mouse_to_gl_point((float)mouse_pos.x, (float)mouse_pos.y);
	glm::vec4 position = glm::vec4((float)point.x, (float)point.y, 0.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.f); 
		
	if (idx == 1) {
	  // primeiro triangulo
	  std::vector<uint32_t> idxs;
	  idxs.push_back(idx-1);
	  Vertex v1 = vertices[idx-1];
	  print_vertex(v1);
	  glm::vec4 xmin_ymax_pos = glm::vec4(v1.position.x, position.y, 0.0f, 1.0f);
	  glm::vec4 xmax_ymin_pos = glm::vec4(position.x, v1.position.y, 0.0f, 1.0f);
	  put_vertice(idx, vertices, xmax_ymin_pos, glm::vec4(1.0f, 0.0f, 0.0f, 0.1f));
	  
	  Vertex v2 = vertices[idx];
	  print_vertex(v2);
	  idxs.push_back(idx);
	  idx++;
	  put_vertice(idx, vertices, position, glm::vec4(0.5f, 1.0f, 0.5f, 0.1f));
	  Vertex v3 = vertices[idx];
	  print_vertex(v3);
	  idxs.push_back(idx);
	  idx++;

	  // segundo triangulo  

	  vertices[idx] = v1;
	  idxs.push_back(idx);
	  idx++;
	  vertices[idx] = v3;
	  idxs.push_back(idx);
	  idx++;
	  put_vertice(idx, vertices, glm::vec4(v1.position.x, position.y, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.1f));
	  idxs.push_back(idx);
	  idx++;
	  PolyGon poly = (PolyGon){
	    .idxs = idxs,
	    .translate = translate,
	    .scale = scale,
	  };
	  polys.push_back(poly);
	} else {
	  if (idx == 0) {
	    put_vertice(idx, vertices, position, color);
	    print_vertex(vertices[idx]);
	    idx++;
	  } else {
	    if (vs.size() >= 3) {
	      std::cout << "add next trangle" << std::endl; 
	      Vertex v1 = vs[0];
	      Vertex last = vertices[idx - 1];
	      
	      vertices[idx] = v1;
	      f.idxs.push_back(idx);
	      idx++;
	      vertices[idx] = last;
	      f.idxs.push_back(idx);
	      idx++;
	      put_vertice(idx, vertices, position, color);
	      f.idxs.push_back(idx);
	      idx++;

	      polys[polys.size() - 1] = f;
	    } else {
	      std::cout << vs.size() << std::endl;
	      vs.push_back((Vertex){ .position = position, .color = color});
	      put_vertice(idx, vertices, position, color);
	      print_vertex(vertices[idx]);
	      idx++;
	    }
	  }
	}
      }
    }
    else {
      // draw
      {
	glClearColor(1.0 * (mouse_pos.x/1000.0f), 1.0 * (mouse_pos.y/1000.0f), 1.0 * (((mouse_pos.x + mouse_pos.y) / 2) / 1000.0f), 1.0);
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      }
    }

    if (vs.size() == 3 && polys.size() == 1) {
      std::cout << "add triangle" << std::endl;
      std::vector<uint32_t> iv;
      for (uint32_t i = polys.back().idxs.size(); i < idx; ++i) {
	iv.push_back(i);
      }
      std::cout << iv.size() << std::endl;
      PolyGon poly = (PolyGon){
	.idxs = iv,
	.translate = glm::vec3(0.0f),
	.scale = glm::vec3(1.0f),
      };
      f = poly;
      polys.push_back(poly);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	  
    if (polys.size() > 0) {
      polys[0].translate = translate;
      polys[0].scale = scale;
    }
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    //glBindVertexArray(VAO);
    draw_triangles(VAO, program, polys);
    
    //std::cout << "total clicks: " << total_click << std::endl;
    //std::cout << "total vertices: " << idx << std::endl;
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
