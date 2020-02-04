// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
#include "SceneGraph.h"

#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/DebugStl.h>

namespace esp {
namespace scene {

SceneGraph::SceneGraph()
    : rootNode_{world_},
      defaultRenderCameraNode_{rootNode_},
      defaultRenderCamera_{defaultRenderCameraNode_} {
  // For now, just create one drawable group with empty string uuid
  createDrawableGroup(std::string{});
}

// set transformation, projection matrix, viewport to the default camera
void SceneGraph::setDefaultRenderCamera(sensor::VisualSensor& sensor) {
  ASSERT(sensor.isVisualSensor());

  sensor.setTransformationMatrix(defaultRenderCamera_)
      .setProjectionMatrix(defaultRenderCamera_)
      .setViewport(defaultRenderCamera_);
}

bool SceneGraph::isRootNode(SceneNode& node) {
  auto parent = node.parent();
  // if the parent is null, it means the node is the world_ node.
  CORRADE_ASSERT(parent != nullptr,
                 "SceneGraph::isRootNode: the node is illegal.", false);
  return (parent->parent() == nullptr ? true : false);
}

gfx::DrawableGroup* SceneGraph::getDrawableGroup(const std::string& id) {
  auto it = drawableGroups_.find(id);
  return it == drawableGroups_.end() ? nullptr : &it->second;
}

const gfx::DrawableGroup* SceneGraph::getDrawableGroup(
    const std::string& id) const {
  auto it = drawableGroups_.find(id);
  return it == drawableGroups_.end() ? nullptr : &it->second;
}

template <typename... DrawableGroupArgs>
gfx::DrawableGroup* SceneGraph::createDrawableGroup(
    std::string id,
    DrawableGroupArgs&&... args) {
  auto inserted = drawableGroups_.emplace(
      std::piecewise_construct, std::forward_as_tuple(std::move(id)),
      std::forward_as_tuple(std::forward<DrawableGroupArgs>(args)...));
  if (!inserted.second) {
    LOG(ERROR) << "DrawableGroup with ID: " << inserted.first->first
               << " already exists!";
    return nullptr;
  }
  LOG(INFO) << "Created DrawableGroup: " << inserted.first->first;
  return &inserted.first->second;
}

template <typename... DrawableGroupArgs>
gfx::DrawableGroup* SceneGraph::getOrCreateDrawableGroup(
    std::string id,
    DrawableGroupArgs&&... args) {
  return getDrawableGroup(id) ||
         createDrawableGroup(std::move(id),
                             std::forward<DrawableGroupArgs>(args)...)
}

bool SceneGraph::deleteDrawableGroup(const std::string& id) {
  return drawableGroups_.erase(id);
}

}  // namespace scene
}  // namespace esp
