// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include <Magnum/BulletIntegration/DebugDraw.h>
#include <Magnum/BulletIntegration/Integration.h>

#include <Corrade/Utility/Assert.h>
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletRigidObject.h"

//!  A Few considerations in construction
//!  Bullet Mesh conversion adapted from:
//!      https://github.com/mosra/magnum-integration/issues/20
//!      https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=11001
//!  Bullet object margin (p15):
//!      https://facultyfp.salisbury.edu/despickler/personal/Resources/
//!        GraphicsExampleCodeGLSL_SFML/InterfaceDoc/Bullet/Bullet_User_Manual.pdf
//!      It's okay to set margin down to 1mm
//!        (1) Bullet/MJCF example
//!      Another solution:
//!        (1) Keep 4cm margin
//!        (2) Use examples/Importers/ImportBsp

namespace esp {
namespace physics {

BulletRigidObject::BulletRigidObject(
    scene::SceneNode* rigidBodyNode,
    int objectId,
    std::shared_ptr<btMultiBodyDynamicsWorld> bWorld,
    std::shared_ptr<std::map<const btCollisionObject*, int> >
        collisionObjToObjIds)
    : BulletBase(bWorld, collisionObjToObjIds),
      RigidObject(rigidBodyNode, objectId),
      MotionState(*rigidBodyNode) {}

BulletRigidObject::~BulletRigidObject() {
  if (!isActive()) {
    // This object may be supporting other sleeping objects, so wake them before
    // removing.
    activateCollisionIsland();
  }

  if (objectMotionType_ != MotionType::STATIC) {
    // remove rigid body from the world
    bWorld_->removeRigidBody(bObjectRigidBody_.get());
  } else if (objectMotionType_ == MotionType::STATIC) {
    // remove collision objects from the world
    for (auto& co : bStaticCollisionObjects_) {
      bWorld_->removeRigidBody(co.get());
      collisionObjToObjIds_->erase(co.get());
    }
  }
  collisionObjToObjIds_->erase(bObjectRigidBody_.get());

}  //~BulletRigidObject

bool BulletRigidObject::initialization_LibSpecific(
    const assets::ResourceManager& resMgr) {
  objectMotionType_ = MotionType::DYNAMIC;
  // get this object's creation template, appropriately cast
  auto tmpAttr = getInitializationAttributes();

  //! Physical parameters
  double margin = tmpAttr->getMargin();
  bool joinCollisionMeshes = tmpAttr->getJoinCollisionMeshes();
  usingBBCollisionShape_ = tmpAttr->getBoundingBoxCollisions();

  // TODO(alexanderwclegg): should provide the option for joinCollisionMeshes
  // and collisionFromBB_ to specify complete vs. component level bounding box
  // heirarchies.

  //! Iterate through all mesh components for one object
  //! The components are combined into a convex compound shape
  bObjectShape_ = std::make_unique<btCompoundShape>();
  // collision mesh/asset handle
  const std::string collisionAssetHandle =
      initializationAttributes_->getCollisionAssetHandle();

  if (!initializationAttributes_->getUseMeshCollision()) {
    // if using prim collider get appropriate bullet collision primitive
    // attributes and build bullet collision shape
    auto primAttributes =
        resMgr.getAssetAttributesManager()->getObjectCopyByHandle(
            collisionAssetHandle);
    // primitive object pointer construction
    auto primObjPtr = buildPrimitiveCollisionObject(
        primAttributes->getPrimObjType(), primAttributes->getHalfLength());
    if (nullptr == primObjPtr) {
      return false;
    }
    primObjPtr->setLocalScaling(btVector3(tmpAttr->getCollisionAssetSize()));
    bGenericShapes_.clear();
    bGenericShapes_.emplace_back(std::move(primObjPtr));
    bObjectShape_->addChildShape(btTransform::getIdentity(),
                                 bGenericShapes_.back().get());
    bObjectShape_->recalculateLocalAabb();
  } else {
    // mesh collider
    const std::vector<assets::CollisionMeshData>& meshGroup =
        resMgr.getCollisionMesh(collisionAssetHandle);
    const assets::MeshMetaData& metaData =
        resMgr.getMeshMetaData(collisionAssetHandle);

    if (!usingBBCollisionShape_) {
      constructBulletCompoundFromMeshes(Magnum::Matrix4{}, meshGroup,
                                        metaData.root, joinCollisionMeshes);

      // add the final object after joining meshes
      if (joinCollisionMeshes) {
        bObjectConvexShapes_.back()->setLocalScaling(
            btVector3(tmpAttr->getCollisionAssetSize()));
        bObjectConvexShapes_.back()->setMargin(0.0);
        bObjectConvexShapes_.back()->recalcLocalAabb();
        bObjectShape_->addChildShape(btTransform::getIdentity(),
                                     bObjectConvexShapes_.back().get());
      }
    }
  }  // if using prim collider else use mesh collider

  //! Set properties
  bObjectShape_->setMargin(margin);

  bObjectShape_->setLocalScaling(btVector3{tmpAttr->getScale()});

  // create the bObjectRigidBody_
  constructRigidBody(false);

  //! Add to world
  bWorld_->addRigidBody(bObjectRigidBody_.get());
  return true;
}  // initialization_LibSpecific

bool BulletRigidObject::finalizeObject_LibSpecific() {
  if (usingBBCollisionShape_) {
    setCollisionFromBB();
  }
  return true;
}  // finalizeObject_LibSpecifc

std::unique_ptr<btCollisionShape>
BulletRigidObject::buildPrimitiveCollisionObject(int primTypeVal,
                                                 double halfLength) {
  // int primTypeVal = primAttributes.getPrimObjType();
  CORRADE_ASSERT(
      (primTypeVal >= 0) &&
          (primTypeVal <
           static_cast<int>(metadata::PrimObjTypes::END_PRIM_OBJ_TYPES)),
      "BulletRigidObject::buildPrimitiveCollisionObject : Illegal primitive "
      "value requested : "
          << primTypeVal,
      nullptr);
  metadata::PrimObjTypes primType =
      static_cast<metadata::PrimObjTypes>(primTypeVal);

  std::unique_ptr<btCollisionShape> obj(nullptr);
  switch (primType) {
    case metadata::PrimObjTypes::CAPSULE_SOLID:
    case metadata::PrimObjTypes::CAPSULE_WF: {
      // use bullet capsule :  btCapsuleShape(btScalar radius,btScalar height);
      btScalar radius = 1.0f;
      btScalar height = 2.0 * halfLength;
      obj = std::make_unique<btCapsuleShape>(radius, height);
      break;
    }
    case metadata::PrimObjTypes::CONE_SOLID:
    case metadata::PrimObjTypes::CONE_WF: {
      // use bullet cone : btConeShape(btScalar radius,btScalar height);
      btScalar radius = 1.0f;
      btScalar height = 2.0 * halfLength;
      obj = std::make_unique<btConeShape>(radius, height);
      break;
    }
    case metadata::PrimObjTypes::CUBE_SOLID:
    case metadata::PrimObjTypes::CUBE_WF: {
      // use bullet box shape : btBoxShape( const btVector3& boxHalfExtents);
      btVector3 dim(1.0, 1.0, 1.0);
      obj = std::make_unique<btBoxShape>(dim);
      break;
    }
    case metadata::PrimObjTypes::CYLINDER_SOLID:
    case metadata::PrimObjTypes::CYLINDER_WF: {
      // use bullet cylinder shape :btCylinderShape (const btVector3&
      // halfExtents);
      btVector3 dim(1.0, 1.0, 1.0);
      obj = std::make_unique<btCylinderShape>(dim);
      break;
    }
    case metadata::PrimObjTypes::ICOSPHERE_SOLID:
    case metadata::PrimObjTypes::ICOSPHERE_WF:
    case metadata::PrimObjTypes::UVSPHERE_SOLID:
    case metadata::PrimObjTypes::UVSPHERE_WF: {
      // use bullet sphere shape :btSphereShape (btScalar radius)
      btScalar radius = 1.0f;
      obj = std::make_unique<btSphereShape>(radius);
      break;
    }
    default: {
      return nullptr;
    }
  }  // switch
  // set primitive object shape margin to be 0, set margin in actual
  // (potentially compound) object
  obj->setMargin(0.0);
  return obj;
}  // buildPrimitiveCollisionObject

// recursively create the convex mesh shapes and add them to the compound in a
// flat manner by accumulating transformations down the tree
void BulletRigidObject::constructBulletCompoundFromMeshes(
    const Magnum::Matrix4& transformFromParentToWorld,
    const std::vector<assets::CollisionMeshData>& meshGroup,
    const assets::MeshTransformNode& node,
    bool join) {
  Magnum::Matrix4 transformFromLocalToWorld =
      transformFromParentToWorld * node.transformFromLocalToParent;
  if (node.meshIDLocal != ID_UNDEFINED) {
    // This node has a mesh, so add it to the compound

    const assets::CollisionMeshData& mesh = meshGroup[node.meshIDLocal];

    if (join) {
      // add all points to a single convex instead of compounding (more
      // stable)
      if (bObjectConvexShapes_.empty()) {
        // create the convex if it does not exist
        bObjectConvexShapes_.emplace_back(
            std::make_unique<btConvexHullShape>());
      }

      // add points
      for (auto& v : mesh.positions) {
        bObjectConvexShapes_.back()->addPoint(
            btVector3(transformFromLocalToWorld.transformPoint(v)), false);
      }
    } else {
      bObjectConvexShapes_.emplace_back(std::make_unique<btConvexHullShape>(
          static_cast<const btScalar*>(mesh.positions.data()->data()),
          mesh.positions.size(), sizeof(Magnum::Vector3)));
      bObjectConvexShapes_.back()->setMargin(0.0);
      bObjectConvexShapes_.back()->recalcLocalAabb();
      //! Add to compound shape stucture
      bObjectShape_->addChildShape(btTransform{transformFromLocalToWorld},
                                   bObjectConvexShapes_.back().get());
    }
  }

  for (auto& child : node.children) {
    constructBulletCompoundFromMeshes(transformFromLocalToWorld, meshGroup,
                                      child, join);
  }
}  // constructBulletCompoundFromMeshes

void BulletRigidObject::setCollisionFromBB() {
  btVector3 dim(node().getCumulativeBB().size() / 2.0);

  for (auto& shape : bGenericShapes_) {
    bObjectShape_->removeChildShape(shape.get());
  }
  bGenericShapes_.clear();
  bGenericShapes_.emplace_back(std::make_unique<btBoxShape>(dim));
  bObjectShape_->addChildShape(btTransform::getIdentity(),
                               bGenericShapes_.back().get());
  bObjectShape_->recalculateLocalAabb();
  bObjectRigidBody_->setCollisionShape(bObjectShape_.get());

  auto tmpAttr = getInitializationAttributes();
  btVector3 bInertia(tmpAttr->getInertia());
  if (bInertia == btVector3{0, 0, 0}) {
    // allow bullet to compute the inertia tensor if we don't have one
    bObjectShape_->calculateLocalInertia(getMass(),
                                         bInertia);  // overrides bInertia

    setInertiaVector(Magnum::Vector3(bInertia));
  }
}  // setCollisionFromBB

bool BulletRigidObject::setMotionType(MotionType mt) {
  if (mt == objectMotionType_) {
    return true;  // no work
  }

  // remove the existing object from the world to change its type
  if (objectMotionType_ == MotionType::STATIC) {
    bWorld_->removeRigidBody(bStaticCollisionObjects_.back().get());
    collisionObjToObjIds_->erase(bStaticCollisionObjects_.back().get());
    bStaticCollisionObjects_.clear();
  } else {
    bWorld_->removeRigidBody(bObjectRigidBody_.get());
  }

  if (mt == MotionType::KINEMATIC) {
    if (!(bObjectRigidBody_->getCollisionFlags() &
          btCollisionObject::CF_KINEMATIC_OBJECT)) {
      // we need to construct a new rigidBody configured for kinematics
      constructRigidBody(true);
    }
    objectMotionType_ = MotionType::KINEMATIC;
    bWorld_->addRigidBody(bObjectRigidBody_.get());
    setActive();
    return true;
  } else if (mt == MotionType::STATIC) {
    objectMotionType_ = MotionType::STATIC;

    // mass == 0 to indicate static. See isStaticObject assert below. See also
    // examples/MultiThreadedDemo/CommonRigidBodyMTBase.h
    btVector3 localInertia(0, 0, 0);
    btRigidBody::btRigidBodyConstructionInfo cInfo(
        /*mass*/ 0.0, nullptr, bObjectShape_.get(), localInertia);
    cInfo.m_startWorldTransform = bObjectRigidBody_->getWorldTransform();
    std::unique_ptr<btRigidBody> staticCollisionObject =
        std::make_unique<btRigidBody>(cInfo);
    ASSERT(staticCollisionObject->isStaticObject());
    bWorld_->addRigidBody(
        staticCollisionObject.get(),
        2,       // collisionFilterGroup (2 == StaticFilter)
        1 + 2);  // collisionFilterMask (1 == DefaultFilter, 2==StaticFilter)
    collisionObjToObjIds_->emplace(staticCollisionObject.get(), objectId_);
    bStaticCollisionObjects_.emplace_back(std::move(staticCollisionObject));
    return true;
  } else if (mt == MotionType::DYNAMIC) {
    if (bObjectRigidBody_->getCollisionFlags() &
        btCollisionObject::CF_KINEMATIC_OBJECT) {
      // we need to construct a new rigidBody configured for dynamics
      constructRigidBody(false);
    }
    objectMotionType_ = MotionType::DYNAMIC;
    bWorld_->addRigidBody(bObjectRigidBody_.get());
    setActive();
    return true;
  }

  return false;
}  // setMotionType

void BulletRigidObject::shiftOrigin(const Magnum::Vector3& shift) {
  if (visualNode_)
    visualNode_->translate(shift);

  // shift all children of the parent collision shape
  for (int i = 0; i < bObjectShape_->getNumChildShapes(); i++) {
    btTransform cT = bObjectShape_->getChildTransform(i);
    cT.setOrigin(cT.getOrigin() + btVector3(shift));
    bObjectShape_->updateChildTransform(i, cT, false);
  }
  // recompute the Aabb once when done
  bObjectShape_->recalculateLocalAabb();
  node().computeCumulativeBB();
}  // shiftOrigin

//! Synchronize Physics transformations
//! Needed after changing the pose from Magnum side
void BulletRigidObject::syncPose() {
  //! For syncing objects
  bObjectRigidBody_->setWorldTransform(
      btTransform(node().transformationMatrix()));
}  // syncPose

void BulletRigidObject::constructRigidBody(bool kinematic) {
  // get this object's creation template, appropriately cast
  auto tmpAttr = getInitializationAttributes();

  double mass = 0;
  btVector3 bInertia = {0, 0, 0};
  if (!kinematic) {
    mass = tmpAttr->getMass();
    bInertia = btVector3(tmpAttr->getInertia());
    if (bInertia == btVector3{0, 0, 0}) {
      // allow bullet to compute the inertia tensor if we don't have one
      bObjectShape_->calculateLocalInertia(mass,
                                           bInertia);  // overrides bInertia
    }
  }

  //! Bullet rigid body setup
  btRigidBody::btRigidBodyConstructionInfo info =
      btRigidBody::btRigidBodyConstructionInfo(mass, &(btMotionState()),
                                               bObjectShape_.get(), bInertia);

  info.m_friction = tmpAttr->getFrictionCoefficient();
  info.m_restitution = tmpAttr->getRestitutionCoefficient();
  info.m_linearDamping = tmpAttr->getLinearDamping();
  info.m_angularDamping = tmpAttr->getAngularDamping();

  //! Create rigid body
  if (collisionObjToObjIds_->count(bObjectRigidBody_.get())) {
    collisionObjToObjIds_->erase(bObjectRigidBody_.get());
  }
  bObjectRigidBody_ = std::make_unique<btRigidBody>(info);
  collisionObjToObjIds_->emplace(bObjectRigidBody_.get(), objectId_);

  // set the
  if (kinematic) {
    bObjectRigidBody_->setCollisionFlags(
        bObjectRigidBody_->getCollisionFlags() |
        btCollisionObject::CF_KINEMATIC_OBJECT);
  }
  syncPose();
}

void BulletRigidObject::activateCollisionIsland() {
  // activate nearby objects in the simulation island as computed on the
  // previous collision detection pass
  btCollisionObject* thisColObj = bObjectRigidBody_.get();
  if (getMotionType() == MotionType::STATIC) {
    thisColObj = bStaticCollisionObjects_.back().get();
  }
  auto& colObjs = bWorld_->getCollisionWorld()->getCollisionObjectArray();
  for (auto objIx = 0; objIx < colObjs.size(); ++objIx) {
    if (colObjs[objIx]->getIslandTag() == thisColObj->getIslandTag()) {
      colObjs[objIx]->activate();
    }
  }
}

void BulletRigidObject::setCOM(const Magnum::Vector3&) {
  // Current not supported
  /*
    bObjectRigidBody_->setCenterOfMassTransform(
        btTransform(Magnum::Matrix4<float>::translation(COM)));*/
}  // setCOM

Magnum::Vector3 BulletRigidObject::getCOM() const {
  // TODO: double check the position if there is any implicit transformation
  // done

  const Magnum::Vector3 com =
      Magnum::Vector3(bObjectRigidBody_->getCenterOfMassPosition());
  return com;
}  // getCOM

bool BulletRigidObject::contactTest() {
  SimulationContactResultCallback src;
  bWorld_->getCollisionWorld()->contactTest(bObjectRigidBody_.get(), src);
  return src.bCollision;
}  // contactTest

const Magnum::Range3D BulletRigidObject::getCollisionShapeAabb() const {
  if (!bObjectShape_) {
    // e.g. empty scene
    return Magnum::Range3D();
  }
  btVector3 localAabbMin, localAabbMax;
  bObjectShape_->getAabb(btTransform::getIdentity(), localAabbMin,
                         localAabbMax);
  return Magnum::Range3D{Magnum::Vector3{localAabbMin},
                         Magnum::Vector3{localAabbMax}};
}  // getCollisionShapeAabb

}  // namespace physics
}  // namespace esp
