// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "RigidScene.h"

namespace esp {
namespace physics {

RigidScene::RigidScene(scene::SceneNode* rigidBodyNode)
    : RigidBase(rigidBodyNode) {}

bool RigidScene::initialize(const assets::ResourceManager& resMgr,
                            const std::string& handle) {
  if (initializationAttributes_ != nullptr) {
    LOG(ERROR) << "Cannot initialize a RigidScene more than once";
    return false;
  }
  objectMotionType_ = MotionType::STATIC;
  initializationAttributes_ =
      resMgr.getSceneAttributesManager()->getTemplateCopyByHandle(handle);

  return initialization_LibSpecific(resMgr);
}

}  // namespace physics
}  // namespace esp