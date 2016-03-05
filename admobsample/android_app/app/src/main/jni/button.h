/*
 * Copyright 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <OpenGLES/ES2/gl.h>
#else
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#endif

class Button {
 public:
  Button();
  bool CheckClick(float x, float y);
  void Draw(GLuint shader_program);
  void SetLocation(float x, float y, float width, float height) {
    x_ = x, y_ = y;
    width_ = width;
    height_ = height;
    half_width_ = width_ / 2;
    half_height_ = height_ / 2;
  }
  void SetColor(float r, float g, float b) {
    r_ = r;
    g_ = g;
    b_ = b;
  }

 private:
  float x_, y_, height_, width_;
  float half_width_, half_height_;
  float r_, g_, b_;
};

#endif  // BUTTON_H_
