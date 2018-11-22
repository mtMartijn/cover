#pragma once
#include <GL/glew.h>

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>

namespace gl {

  // Static object for drawing a quad.
  struct quad {
    static void draw(int x, int y, int w, int h);
    static GLfloat verts[8];
  };

  // Wrapper for an OpenGL framebuffer object.
  class fbo {
  public:
    fbo();
    void attach(GLenum target, GLenum format, int w, int h, bool clamp = false);

    void enable(GLenum target = GL_COLOR_ATTACHMENT0);
    void enable_all();
    void disable();

    GLuint get_ID(GLenum target = GL_COLOR_ATTACHMENT0);

  private:
    GLuint fboID;
    std::map<GLenum, GLuint> attachments;
  };

  // Wrapper for a GLSL shader program.
  class shader {
  public:
    shader();
    ~shader() { glDeleteProgram(programID); }
    void load_file(const std::string& vertex_file_path, const std::string& fragment_file_path);
    void load(const std::string& fragment_shader_code, 
      const std::string& vertex_shader_code = basic_vert_shader);

    void uniform(const char* name, const glm::mat3& m);
    void uniform(const char* name, const glm::mat4& m);
    void uniform(const char* name, float x);
    void uniform(const char* name, float x, float y);
    void uniform(const char* name, float x, float y, float z);
    void uniform(const char* name, float x, float y, float z, float w);
    void uniform(const char* name, const glm::vec2& v);
    void uniform(const char* name, const glm::vec3& v);
    void uniform(const char* name, const glm::vec4& v);
    void uniformui(const char* name, unsigned int x);
    void subroutine(const char* name, bool fragment = true);

    void texture(const char* name, GLuint id);
    GLuint ID() { return programID; }

    void enable();
    void disable();

  private:
    GLuint programID;
    std::map<std::string, GLuint> uniforms;
    int tex_unit;

    static const std::string glsl_version_header;
    static const std::string basic_vert_shader;
  };

  void display_info();

}
