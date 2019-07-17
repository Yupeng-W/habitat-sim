// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "magnum.h"

#include "esp/core/esp.h"
#include "esp/scene/SceneNode.h"

namespace esp {
namespace gfx {

class RenderCamera : public Magnum::SceneGraph::AbstractFeature3D {
 public:
  RenderCamera(scene::SceneNode& node);
  RenderCamera(scene::SceneNode& node,
               const vec3f& eye,
               const vec3f& target,
               const vec3f& up);
  virtual ~RenderCamera() {
    // do nothing, let magnum handle the camera
  }

  // Get the scene node being attached to.
  scene::SceneNode& node() { return object(); }
  const scene::SceneNode& node() const { return object(); }

  // Overloads to avoid confusion
  scene::SceneNode& object() {
    return static_cast<scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }
  const scene::SceneNode& object() const {
    return static_cast<const scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }

  void setProjectionMatrix(int width,
                           int height,
                           float znear,
                           float zfar,
                           float hfov);

  mat4f getProjectionMatrix();

  mat4f getCameraMatrix();

  MagnumCamera& getMagnumCamera();

  void draw(MagnumDrawableGroup& drawables);

 protected:
  MagnumCamera* camera_ = nullptr;

  ESP_SMART_POINTERS(RenderCamera)
};

}  // namespace gfx
}  // namespace esp
