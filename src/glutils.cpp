#include "glutils.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <stdlib.h>
#include <string.h>

#define STR(s) #s

namespace gl {

  GLfloat quad::verts[] = {
    0.f, 0.f,
    1.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
  };

  void quad::draw(int x, int y, int w, int h) {
    static GLuint vaoID = 0;
    static GLuint vboID = 0;
    static bool init = false;

    if(!init) {
      glGenVertexArrays(1, &vaoID);
      glBindVertexArray(vaoID);

      glGenBuffers(1, &vboID);
      glBindBuffer(GL_ARRAY_BUFFER, vboID);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quad::verts), quad::verts, GL_STATIC_DRAW);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      init = true;
    }
    glViewport(x, y, w, h);
    /* glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); */
    glBindVertexArray(vaoID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }

  fbo::fbo(): 
    fboID(0) {
      attachments[GL_COLOR_ATTACHMENT0] = 0;
      attachments[GL_COLOR_ATTACHMENT1] = 0;
      attachments[GL_COLOR_ATTACHMENT2] = 0;
      attachments[GL_COLOR_ATTACHMENT3] = 0;
      attachments[GL_COLOR_ATTACHMENT4] = 0;
      attachments[GL_COLOR_ATTACHMENT5] = 0;
      attachments[GL_COLOR_ATTACHMENT6] = 0;
      attachments[GL_COLOR_ATTACHMENT7] = 0;
      attachments[GL_DEPTH_ATTACHMENT] = 0;
      attachments[GL_STENCIL_ATTACHMENT] = 0;
  }

  void fbo::attach(GLenum target, GLenum format, int w, int h, bool clamp) {
    if(fboID == 0) glGenFramebuffers(1, &fboID);

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexStorage2D(GL_TEXTURE_2D, 1, format, w, h);
    /* glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_FLOAT, nullptr); */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    /* glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Not sure about this */

    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glFramebufferTexture(GL_FRAMEBUFFER, target, texID, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "FBO not initialized properly." << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    attachments[target] = texID;
  }

  void fbo::enable(GLenum target) {
    //glBindTexture isn't necessary here, 
    //glBindFramebuffer and glDrawbuffer take care of that
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glDrawBuffer(target);
    //This clears all color attachments specified by glDrawBuffers
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void fbo::enable_all() {
    GLenum b[] = {
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3,
      GL_COLOR_ATTACHMENT4,
      GL_COLOR_ATTACHMENT5,
      GL_COLOR_ATTACHMENT6,
      GL_COLOR_ATTACHMENT7
    };

    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glDrawBuffers(8, b);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void fbo::disable() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  GLuint fbo::get_ID(GLenum target) {
    return attachments[target];
  }

  shader::shader():
    programID(0),
    tex_unit(0) {
  }

  void shader::load_file(const std::string& vertex_file_path, const std::string& fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open()){
      std::string Line = "";
      while(getline(VertexShaderStream, Line))
        VertexShaderCode += "\n" + Line;
      VertexShaderStream.close();
    }else{
      std::cout << "Impossible to open " << vertex_file_path << ". Are you in the right directory?" << std::endl;
      return;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
      std::string Line = "";
      while(getline(FragmentShaderStream, Line))
        FragmentShaderCode += "\n" + Line;
      FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
      std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
      glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
      printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
      std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
      glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
      printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    programID = glCreateProgram();
    glAttachShader(programID, VertexShaderID);
    glAttachShader(programID, FragmentShaderID);
    glLinkProgram(programID);

    // Check the program
    glGetProgramiv(programID, GL_LINK_STATUS, &Result);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
      std::vector<char> ProgramErrorMessage(InfoLogLength+1);
      glGetProgramInfoLog(programID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
      printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(programID, VertexShaderID);
    glDetachShader(programID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
  }

  //For some very frustrating reason, I cannot declare a global string and manually
  //concatenate the string header, I keep getting segfaults...
  const std::string shader::glsl_version_header = "#version 430 core\n";

  const std::string shader::basic_vert_shader = STR(
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec2 coord;
    out vec2 texCoord;

    void main() {
      texCoord = coord;
      gl_Position = position;
    }
  );

  void shader::load(const std::string& fragment_shader_code, const std::string& vertex_shader_code){
    GLuint vertexID = glCreateShader(GL_VERTEX_SHADER);
    std::string str = glsl_version_header + vertex_shader_code;
    const char* vert_code = str.c_str();
    glShaderSource(vertexID, 1, &vert_code, nullptr);
    glCompileShader(vertexID);

    GLuint fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
    str = glsl_version_header + fragment_shader_code;
    const char* frag_code = str.c_str();
    glShaderSource(fragmentID, 1, &frag_code, nullptr);
    glCompileShader(fragmentID);

    programID = glCreateProgram();
    glAttachShader(programID, vertexID);
    glAttachShader(programID, fragmentID);
    glLinkProgram(programID);

    glDetachShader(programID, vertexID);
    glDetachShader(programID, fragmentID);
    
    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);

  }

  void shader::uniform(const char* name, const glm::mat3& m) {
  GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniformMatrix3fv(i, 1, GL_FALSE, &m[0][0]);
  }

  void shader::uniform(const char* name, const glm::mat4& m) {
  GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniformMatrix4fv(i, 1, GL_FALSE, &m[0][0]);
  }

  void shader::uniform(const char* name, float x) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniform1f(i, x);
  }

  void shader::uniform(const char* name, float x, float y) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniform2f(i, x, y);
  }

  void shader::uniform(const char* name, float x, float y, float z) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniform3f(i, x, y, z);
  }

  void shader::uniform(const char* name, float x, float y, float z, float w) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniform4f(i, x, y, z, w);
  }

  void shader::uniform(const char* name, const glm::vec2& v) {
    uniform(name, v.x, v.y);
  }

  void shader::uniform(const char* name, const glm::vec3& v) {
    uniform(name, v.x, v.y, v.z);
  }

  void shader::uniform(const char* name, const glm::vec4& v) {
    uniform(name, v.x, v.y, v.z, v.w);
  }

  void shader::uniformui(const char* name, unsigned int x) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Uniform '" << name << "' not found." << std::endl;
    glUniform1ui(i, x);
  }

  void shader::subroutine(const char* name, bool fragment) {
    auto sh = fragment ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER;
    GLuint i = glGetSubroutineIndex(programID, sh, name);
    glUniformSubroutinesuiv(sh, 1, &i);
  }

  void shader::texture(const char* name, GLuint id) {
    GLint i = glGetUniformLocation(programID, name);
    if(i == -1) std::cerr << "Texture '" << name << "' not found." << std::endl;
    glActiveTexture(GL_TEXTURE0 + tex_unit);
    glBindTexture(GL_TEXTURE_2D, id);
    glUniform1i(i, tex_unit);
    ++tex_unit;
  }

  void shader::enable() {
    glUseProgram(programID);
    tex_unit = 0;
  }

  void shader::disable() {
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
  }

  void display_info() {
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << "GL Renderer: " << renderer << std::endl;
    std::cout << "GL Vendor: " << vendor << std::endl;
    std::cout << "GL Version: " << version << std::endl;
    std::cout << "GLSL Version: " << glslVersion << std::endl;
    std::cout << std::endl;

    GLint var;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &var);
    std::cout << "GL_MAX_COLOR_ATTACHMENTS: " << var << std::endl;

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &var);
    std::cout << "GL_MAX_TEXTURE_IMAGE_UNITS: " << var << std::endl;

    glGetIntegerv(GL_SAMPLES, &var);
    std::cout << "GL_SAMPLES: " << var << std::endl;

  }

}
