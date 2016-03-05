//
// Created by ccornell on 3/4/16.
//

#include "button.h"

Button::Button() : r_(1.0), g_(1.0), b_(1.0), half_width_(0), half_height_(0) {}

bool Button::CheckClick(float x, float y) {
  return (x >= x_ - half_width_) && (x <= x_ + half_width_) &&
         (y >= y_ - half_height_) && (y <= y_ + half_height_);
}

void Button::Draw(GLuint shader_program) {
  GLfloat vertices[8];

  vertices[0] = x_ - half_width_;
  vertices[1] = y_ + half_height_;
  vertices[2] = x_ + half_width_;
  vertices[3] = y_ + half_height_;
  vertices[4] = x_ - half_width_;
  vertices[5] = y_ - half_height_;
  vertices[6] = x_ + half_width_;
  vertices[7] = y_ - half_height_;

  GLuint vbo;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLfloat colorBytes[] = {r_, g_, b_, 1.0f};
  GLint colorLocation = glGetUniformLocation(shader_program, "color");
  glUniform4fv(colorLocation, 1, colorBytes);

  GLint posAttrib = glGetAttribLocation(shader_program, "position");
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posAttrib);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDeleteBuffers(1, &vbo);
}