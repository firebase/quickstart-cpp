//
// Created by ccornell on 3/4/16.
//

#include "button.h"
#include <android/log.h>

Button::Button() : r_(1.0), g_(1.0), b_(1.0), half_width_(0), half_height_(0) {}

bool Button::CheckClick(float x, float y) {
  return (x >= x_ - half_width_) && (x <= x_ + half_width_) &&
         (y >= y_ - half_height_) && (y <= y_ + half_height_);
}

unsigned char Button::texture_data_[] = {
    255, 0,   255, 255, 255, 0,   255, 255, 0,   0, 255, 255, 0,   0, 255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 0,   0, 255, 255, 0,   0, 255, 255,
    0,   255, 0,   255, 0,   255, 0,   255, 255, 0, 0,   255, 255, 0, 0,   255,
    0,   255, 0,   255, 0,   255, 0,   255, 255, 0, 0,   255, 255, 0, 0,   255};

void Button::GenerateTexture(const BmpImage& image) {
  glGenTextures(1, &texture_id_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  //  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0,
  //               GL_RGBA, GL_UNSIGNED_BYTE, texture_data_);

  unsigned char* texture_buffer = ImageData::UnpackData(image);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texture_buffer);

  __android_log_print(ANDROID_LOG_INFO, "GMACPP",
                      "-----------Button texture generated, maybe!");

  delete texture_buffer;
}

void Button::Draw(GLuint shader_program) {
  GLfloat vertices[16];

  vertices[0] = x_ - half_width_;
  vertices[1] = y_ + half_height_;
  vertices[2] = 0;
  vertices[3] = 0;

  vertices[4] = x_ + half_width_;
  vertices[5] = y_ + half_height_;
  vertices[6] = 1;
  vertices[7] = 0;

  vertices[8] = x_ - half_width_;
  vertices[9] = y_ - half_height_;
  vertices[10] = 0;
  vertices[11] = 1;

  vertices[12] = x_ + half_width_;
  vertices[13] = y_ - half_height_;
  vertices[14] = 1;
  vertices[15] = 1;

  GLuint vbo;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Uniforms:
  GLfloat colorBytes[] = {r_, g_, b_, 1.0f};
  GLint colorLocation = glGetUniformLocation(shader_program, "color");
  glUniform4fv(colorLocation, 1, colorBytes);

  // texture:
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  GLint texture_uniform_handle =
      glGetUniformLocation(shader_program, "texture_unit_0");
  if (texture_uniform_handle >= 0)
    glUniform1i(texture_uniform_handle, 0);
  else
    __android_log_print(ANDROID_LOG_INFO, "GMACPP", "-----------ERRORZ");

  GLint position_attrib = glGetAttribLocation(shader_program, "aPosition");
  glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, 0);

  void* uv_offset = 0;
  uv_offset += sizeof(float) * 2;

  GLint texture_uv_attrib = glGetAttribLocation(shader_program, "aTexCoord");
  glVertexAttribPointer(texture_uv_attrib, 2, GL_FLOAT, GL_FALSE,
                        sizeof(float) * 4, uv_offset);

  glEnableVertexAttribArray(position_attrib);
  glEnableVertexAttribArray(texture_uv_attrib);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDeleteBuffers(1, &vbo);
}