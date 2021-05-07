// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#ifndef ESP_PHYSICS_MANAGEDRIGIDBASE_H_
#define ESP_PHYSICS_MANAGEDRIGIDBASE_H_

#include "ManagedPhysicsObjectBase.h"
#include "esp/physics/RigidObject.h"

namespace esp {
namespace physics {

/**
 * @brief Class template describing wrapper for RigidBase constructions.
 * Provides bindings for all RigidBase functionality.
 */

template <class T>
class AbstractManagedRigidBase
    : public esp::physics::AbstractManagedPhysicsObject<T> {
 public:
  static_assert(
      std::is_base_of<esp::physics::RigidBase, T>::value,
      "AbstractManagedRigidBase :: Managed physics object type must be "
      "derived from esp::physics::RigidBase");

  AbstractManagedRigidBase(std::shared_ptr<T>& objPtr,
                           const std::string& classKey)
      : AbstractManagedPhysicsObject<T>(objPtr, classKey) {}

  bool isActive() {
    if (auto sp = this->getObjectReference()) {
      return sp->isActive();
    } else {
      return false;
    }
  }  // isActive()

  void setActive() {
    if (auto sp = this->getObjectReference()) {
      sp->setActive();
    }
  }  // activate()

  void applyForce(const Magnum::Vector3& force, const Magnum::Vector3& relPos) {
    if (auto sp = this->getObjectReference()) {
      sp->applyForce(force, relPos);
    }
  }  // applyForce
  void applyImpulse(const Magnum::Vector3& impulse,
                    const Magnum::Vector3& relPos) {
    if (auto sp = this->getObjectReference()) {
      sp->applyImpulse(impulse, relPos);
    }
  }  // applyImpulse

  void applyTorque(const Magnum::Vector3& torque) {
    if (auto sp = this->getObjectReference()) {
      sp->applyTorque(torque);
    }
  }  // applyTorque

  void applyImpulseTorque(const Magnum::Vector3& impulse) {
    if (auto sp = this->getObjectReference()) {
      sp->applyImpulseTorque(impulse);
    }
  }  // applyImpulseTorque

  // ==== Transformations ===

  Magnum::Matrix4 getTransformation() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getTransformation();
    } else {
      return Magnum::Matrix4{};
    }
  }  // getTransformation

  void setTransformation(const Magnum::Matrix4& transformation) {
    if (auto sp = this->getObjectReference()) {
      sp->setTransformation(transformation);
    }
  }  // setTransformation

  Magnum::Vector3 getTranslation() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getTranslation();
    } else {
      return Magnum::Vector3{};
    }
  }  // getTranslation

  void setTranslation(const Magnum::Vector3& vector) {
    if (auto sp = this->getObjectReference()) {
      sp->setTranslation(vector);
    }
  }  // setTranslation

  Magnum::Quaternion getRotation() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getRotation();
    } else {
      return Magnum::Quaternion{};
    }
  }  // getTranslation
  void setRotation(const Magnum::Quaternion& quaternion) {
    if (auto sp = this->getObjectReference()) {
      sp->setRotation(quaternion);
    }
  }  // setRotation

  core::RigidState getRigidState() {
    if (auto sp = this->getObjectReference()) {
      return sp->getRigidState();
    } else {
      return core::RigidState{};
    }
  }  // getRigidState()

  void setRigidState(const core::RigidState& rigidState) {
    if (auto sp = this->getObjectReference()) {
      sp->setRigidState(rigidState);
    }
  }  // setRigidState

  void resetTransformation() {
    if (auto sp = this->getObjectReference()) {
      sp->resetTransformation();
    }
  }  // resetTransformation

  void translate(const Magnum::Vector3& vector) {
    if (auto sp = this->getObjectReference()) {
      sp->translate(vector);
    }
  }  // translate

  void translateLocal(const Magnum::Vector3& vector) {
    if (auto sp = this->getObjectReference()) {
      sp->translateLocal(vector);
    }
  }  // translateLocal

  void rotate(const Magnum::Rad angleInRad,
              const Magnum::Vector3& normalizedAxis) {
    if (auto sp = this->getObjectReference()) {
      sp->rotate(angleInRad, normalizedAxis);
    }
  }  // rotate

  void rotateLocal(const Magnum::Rad angleInRad,
                   const Magnum::Vector3& normalizedAxis) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateLocal(angleInRad, normalizedAxis);
    }
  }  // rotateLocal

  void rotateX(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateX(angleInRad);
    }
  }  // rotateX

  void rotateY(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateY(angleInRad);
    }
  }  // rotateY

  void rotateZ(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateZ(angleInRad);
    }
  }  // rotateZ

  void rotateXLocal(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateXLocal(angleInRad);
    }
  }  // rotateXLocal

  void rotateYLocal(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateYLocal(angleInRad);
    }
  }  // rotateYLocal

  void rotateZLocal(const Magnum::Rad angleInRad) {
    if (auto sp = this->getObjectReference()) {
      sp->rotateZLocal(angleInRad);
    }
  }  // rotateZLocal

  // ==== Getter/Setter functions ===

  double getAngularDamping() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getAngularDamping();
    } else {
      return 0.0;
    }
  }  // getAngularDamping

  void setAngularDamping(const double angDamping) {
    if (auto sp = this->getObjectReference()) {
      sp->setAngularDamping(angDamping);
    }
  }  // setAngularDamping

  Magnum::Vector3 getAngularVelocity() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getAngularVelocity();
    } else {
      return Magnum::Vector3();
    }
  }  // getAngularVelocity

  void setAngularVelocity(const Magnum::Vector3& angVel) {
    if (auto sp = this->getObjectReference()) {
      sp->setAngularVelocity(angVel);
    }
  }  // setAngularVelocity

  bool getCollidable() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getCollidable();
    } else {
      return false;
    }
  }  // getCollidable()

  bool setCollidable(bool collidable) {
    if (auto sp = this->getObjectReference()) {
      return sp->setCollidable(collidable);
    } else {
      return false;
    }
  }  // setCollidable

  Magnum::Vector3 getCOM() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getCOM();
    } else {
      return Magnum::Vector3();
    }
  }  // getCOM

  void setCOM(const Magnum::Vector3& COM) {
    if (auto sp = this->getObjectReference()) {
      sp->setCOM(COM);
    }
  }  // setCOM

  double getFrictionCoefficient() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getFrictionCoefficient();
    } else {
      return 0.0;
    }
  }  // getFrictionCoefficient

  void setFrictionCoefficient(const double frictionCoefficient) {
    if (auto sp = this->getObjectReference()) {
      sp->setFrictionCoefficient(frictionCoefficient);
    }
  }  // setFrictionCoefficient

  Magnum::Matrix3 getInertiaMatrix() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getInertiaMatrix();
    } else {
      return Magnum::Matrix3();
    }
  }  // getInertiaMatrix

  Magnum::Vector3 getInertiaVector() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getInertiaVector();
    } else {
      return Magnum::Vector3();
    }
  }  // getInertiaVector

  void setInertiaVector(const Magnum::Vector3& inertia) {
    if (auto sp = this->getObjectReference()) {
      sp->setInertiaVector(inertia);
    }
  }  // setInertiaVector

  void setLightSetup(const std::string& lightSetupKey) {}

  double getLinearDamping() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getLinearDamping();
    } else {
      return 0.0;
    }
  }  // getLinearDamping

  void setLinearDamping(const double linDamping) {
    if (auto sp = this->getObjectReference()) {
      sp->setLinearDamping(linDamping);
    }
  }  // setLinearDamping

  Magnum::Vector3 getLinearVelocity() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getLinearVelocity();
    } else {
      return Magnum::Vector3();
    }
  }  // getLinearVelocity

  void setLinearVelocity(const Magnum::Vector3& linVel) {
    if (auto sp = this->getObjectReference()) {
      sp->setLinearVelocity(linVel);
    }
  }  // setLinearVelocity

  double getMass() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getMass();
    } else {
      return 0.0;
    }
  }  // getMass

  void setMass(const double mass) {
    if (auto sp = this->getObjectReference()) {
      sp->setMass(mass);
    }
  }  // setMass

  double getRestitutionCoefficient() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getRestitutionCoefficient();
    } else {
      return 0.0;
    }
  }  // getRestitutionCoefficient

  void setRestitutionCoefficient(const double restitutionCoefficient) {
    if (auto sp = this->getObjectReference()) {
      sp->setRestitutionCoefficient(restitutionCoefficient);
    }
  }  // setRestitutionCoefficient

  Magnum::Vector3 getScale() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getScale();
    } else {
      return Magnum::Vector3();
    }
  }  // getScale

  int getSemanticId() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getSemanticId();
    } else {
      return 0;
    }
  }  // getSemanticId

  void setSemanticId(uint32_t semanticId) {
    if (auto sp = this->getObjectReference()) {
      sp->setSemanticId(semanticId);
    }
  }  // setSemanticId

  std::vector<scene::SceneNode*> getVisualSceneNodes() const {
    if (auto sp = this->getObjectReference()) {
      sp->getVisualSceneNodes();
    } else {
      return std::vector<scene::SceneNode*>();
    }
  }  // getVisualSceneNodes

  scene::SceneNode* getSceneNode() {
    if (auto sp = this->getObjectReference()) {
      sp->getSceneNode();
    } else {
      return nullptr;
    }
  }  // getSceneNode

 public:
  ESP_SMART_POINTERS(AbstractManagedRigidBase<T>)
};

}  // namespace physics
}  // namespace esp

#endif  // ESP_PHYSICS_MANAGEDRIGIDBASE_H_
