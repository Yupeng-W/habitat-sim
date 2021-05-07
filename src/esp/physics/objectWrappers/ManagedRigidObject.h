// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#ifndef ESP_PHYSICS_MANAGEDRIGIDOBJECT_H_
#define ESP_PHYSICS_MANAGEDRIGIDOBJECT_H_

#include "ManagedRigidBase.h"
#include "esp/physics/RigidObject.h"

namespace esp {
namespace physics {
class ManagedRigidObject
    : public esp::physics::AbstractManagedRigidBase<esp::physics::RigidObject> {
 public:
  ManagedRigidObject()
      : AbstractManagedRigidBase<esp::physics::RigidObject>(
            "ManagedRigidObject") {}

  std::shared_ptr<metadata::attributes::ObjectAttributes>
  getInitializationAttributes() const {
    if (auto sp = this->getObjectReference()) {
      return sp->getInitializationAttributes();
    } else {
      return nullptr;
    }
  }  // getInitializationAttributes()

 public:
  ESP_SMART_POINTERS(ManagedRigidObject)
};

}  // namespace physics
}  // namespace esp

#endif  // ESP_PHYSICS_MANAGEDRIGIDOBJECT_H_
