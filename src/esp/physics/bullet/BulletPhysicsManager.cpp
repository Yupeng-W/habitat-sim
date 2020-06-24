// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

//#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
//#include "BulletCollision/Gimpact/btGImpactShape.h"

#include "BulletPhysicsManager.h"
#include "BulletRigidObject.h"
#include "esp/assets/ResourceManager.h"

namespace esp {
namespace physics {

BulletPhysicsManager::~BulletPhysicsManager() {
  LOG(INFO) << "Deconstructing BulletPhysicsManager";

  existingObjects_.clear();
  staticSceneObject_.reset(nullptr);
}

bool BulletPhysicsManager::initPhysicsFinalize() {
  activePhysSimLib_ = BULLET;

  //! We can potentially use other collision checking algorithms, by
  //! uncommenting the line below
  // btGImpactCollisionAlgorithm::registerAlgorithm(&bDispatcher_);
  bWorld_ = std::make_shared<btMultiBodyDynamicsWorld>(
      &bDispatcher_, &bBroadphase_, &bSolver_, &bCollisionConfig_);

  debugDrawer_.setMode(
      Magnum::BulletIntegration::DebugDraw::Mode::DrawWireframe |
      Magnum::BulletIntegration::DebugDraw::Mode::DrawConstraints);
  bWorld_->setDebugDrawer(&debugDrawer_);

  // currently GLB meshes are y-up
  bWorld_->setGravity(btVector3(physicsManagerAttributes->getVec3("gravity")));

  //! Create new scene node
  staticSceneObject_ = physics::BulletRigidScene::create_unique(
      &physicsNode_->createChild(), bWorld_);

  return true;
}

// Bullet Mesh conversion adapted from:
// https://github.com/mosra/magnum-integration/issues/20
bool BulletPhysicsManager::addSceneFinalize(const std::string& handle) {
  //! Initialize scene
  bool sceneSuccess = staticSceneObject_->initialize(resourceManager_, handle);

  return sceneSuccess;
}

bool BulletPhysicsManager::makeAndAddRigidObject(int newObjectID,
                                                 const std::string& handle,
                                                 scene::SceneNode* objectNode) {
  auto ptr = physics::BulletRigidObject::create_unique(objectNode, bWorld_);
  bool objSuccess = ptr->initialize(resourceManager_, handle);
  if (objSuccess) {
    existingObjects_.emplace(newObjectID, std::move(ptr));
  }
  return objSuccess;
}

//! Check if mesh primitive is compatible with physics
bool BulletPhysicsManager::isMeshPrimitiveValid(
    const assets::CollisionMeshData& meshData) {
  if (meshData.primitive == Magnum::MeshPrimitive::Triangles) {
    //! Only triangle mesh works
    return true;
  } else {
    switch (meshData.primitive) {
      case Magnum::MeshPrimitive::Lines:
        LOG(ERROR) << "Invalid primitive: Lines";
        break;
      case Magnum::MeshPrimitive::Points:
        LOG(ERROR) << "Invalid primitive: Points";
        break;
      case Magnum::MeshPrimitive::LineLoop:
        LOG(ERROR) << "Invalid primitive Line loop";
        break;
      case Magnum::MeshPrimitive::LineStrip:
        LOG(ERROR) << "Invalid primitive Line Strip";
        break;
      case Magnum::MeshPrimitive::TriangleStrip:
        LOG(ERROR) << "Invalid primitive Triangle Strip";
        break;
      case Magnum::MeshPrimitive::TriangleFan:
        LOG(ERROR) << "Invalid primitive Triangle Fan";
        break;
      default:
        LOG(ERROR) << "Invalid primitive " << int(meshData.primitive);
    }
    LOG(ERROR) << "Cannot load collision mesh, skipping";
    return false;
  }
}

void BulletPhysicsManager::setGravity(const Magnum::Vector3& gravity) {
  bWorld_->setGravity(btVector3(gravity));
  // After gravity change, need to reactive all bullet objects
  for (std::map<int, physics::RigidObject::uptr>::iterator it =
           existingObjects_.begin();
       it != existingObjects_.end(); ++it) {
    it->second->setActive();
  }
}

Magnum::Vector3 BulletPhysicsManager::getGravity() const {
  return Magnum::Vector3(bWorld_->getGravity());
}

void BulletPhysicsManager::stepPhysics(double dt) {
  // We don't step uninitialized physics sim...
  if (!initialized_) {
    return;
  }
  if (dt <= 0) {
    dt = fixedTimeStep_;
  }

  // set specified control velocities
  for (auto& objectItr : existingObjects_) {
    VelocityControl::ptr velControl = objectItr.second->getVelocityControl();
    if (objectItr.second->getMotionType() == MotionType::KINEMATIC) {
      // kinematic velocity control intergration
      if (velControl->controllingAngVel || velControl->controllingLinVel) {
        objectItr.second->setRigidState(velControl->integrateTransform(
            dt, objectItr.second->getRigidState()));
        objectItr.second->setActive();
      }
    } else if (objectItr.second->getMotionType() == MotionType::DYNAMIC) {
      if (velControl->controllingLinVel) {
        if (velControl->linVelIsLocal) {
          setLinearVelocity(objectItr.first,
                            objectItr.second->node().rotation().transformVector(
                                velControl->linVel));
        } else {
          setLinearVelocity(objectItr.first, velControl->linVel);
        }
      }
      if (velControl->controllingAngVel) {
        if (velControl->angVelIsLocal) {
          setAngularVelocity(
              objectItr.first,
              objectItr.second->node().rotation().transformVector(
                  velControl->angVel));
        } else {
          setAngularVelocity(objectItr.first, velControl->angVel);
        }
      }
    }
  }

  // ==== Physics stepforward ======
  // NOTE: worldTime_ will always be a multiple of sceneMetaData_.timestep
  int numSubStepsTaken =
      bWorld_->stepSimulation(dt, /*maxSubSteps*/ 10000, fixedTimeStep_);
  worldTime_ += numSubStepsTaken * fixedTimeStep_;
}

void BulletPhysicsManager::setMargin(const int physObjectID,
                                     const double margin) {
  assertIDValidity(physObjectID);
  static_cast<BulletRigidObject*>(existingObjects_.at(physObjectID).get())
      ->setMargin(margin);
}

void BulletPhysicsManager::setSceneFrictionCoefficient(
    const double frictionCoefficient) {
  staticSceneObject_->setFrictionCoefficient(frictionCoefficient);
}

void BulletPhysicsManager::setSceneRestitutionCoefficient(
    const double restitutionCoefficient) {
  staticSceneObject_->setRestitutionCoefficient(restitutionCoefficient);
}

double BulletPhysicsManager::getMargin(const int physObjectID) const {
  assertIDValidity(physObjectID);
  return static_cast<BulletRigidObject*>(
             existingObjects_.at(physObjectID).get())
      ->getMargin();
}

double BulletPhysicsManager::getSceneFrictionCoefficient() const {
  return staticSceneObject_->getFrictionCoefficient();
}

double BulletPhysicsManager::getSceneRestitutionCoefficient() const {
  return staticSceneObject_->getRestitutionCoefficient();
}

const Magnum::Range3D BulletPhysicsManager::getCollisionShapeAabb(
    const int physObjectID) const {
  assertIDValidity(physObjectID);
  return static_cast<BulletRigidObject*>(
             existingObjects_.at(physObjectID).get())
      ->getCollisionShapeAabb();
}

const Magnum::Range3D BulletPhysicsManager::getSceneCollisionShapeAabb() const {
  return static_cast<BulletRigidScene*>(staticSceneObject_.get())
      ->getCollisionShapeAabb();
}

void BulletPhysicsManager::debugDraw(const Magnum::Matrix4& projTrans) const {
  debugDrawer_.setTransformationProjectionMatrix(projTrans);
  bWorld_->debugDrawWorld();
}

bool BulletPhysicsManager::contactTest(const int physObjectID) {
  assertIDValidity(physObjectID);
  bWorld_->getCollisionWorld()->performDiscreteCollisionDetection();
  return static_cast<BulletRigidObject*>(
             existingObjects_.at(physObjectID).get())
      ->contactTest();
}

}  // namespace physics
}  // namespace esp
