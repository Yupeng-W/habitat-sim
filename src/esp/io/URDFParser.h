// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Code adapted from Bullet3/examples/Importers/ImportURDFDemo ...

#pragma once
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tinyxml2 {
class XMLElement;
};

////////////////////////////////////
// Utility/storage structs
////////////////////////////////////

namespace esp {
namespace io {

namespace URDF {

struct MaterialColor {
  Magnum::Color4 m_rgbaColor;
  Magnum::Color3 m_specularColor;
  MaterialColor()
      : m_rgbaColor(0.8, 0.8, 0.8, 1), m_specularColor(0.4, 0.4, 0.4) {}
};

struct Material {
  std::string m_name;
  std::string m_textureFilename;
  MaterialColor m_matColor;

  Material() {}
};

enum JointTypes {
  RevoluteJoint = 1,
  PrismaticJoint,
  ContinuousJoint,
  FloatingJoint,
  PlanarJoint,
  FixedJoint,
  SphericalJoint,

};

enum GeomTypes {
  GEOM_SPHERE = 2,
  GEOM_BOX,
  GEOM_CYLINDER,
  GEOM_MESH,
  GEOM_PLANE,
  GEOM_CAPSULE,  // non-standard URDF
  GEOM_UNKNOWN,
};

struct Geometry {
  GeomTypes m_type;

  double m_sphereRadius;

  Magnum::Vector3 m_boxSize;

  double m_capsuleRadius;
  double m_capsuleHeight;
  int m_hasFromTo;
  Magnum::Vector3 m_capsuleFrom;
  Magnum::Vector3 m_capsuleTo;

  Magnum::Vector3 m_planeNormal;
  std::string m_meshFileName;
  Magnum::Vector3 m_meshScale;

  std::shared_ptr<Material> m_localMaterial;
  bool m_hasLocalMaterial;

  Geometry()
      : m_type(GEOM_UNKNOWN),
        m_sphereRadius(1),
        m_boxSize(1, 1, 1),
        m_capsuleRadius(1),
        m_capsuleHeight(1),
        m_hasFromTo(0),
        m_capsuleFrom(0, 1, 0),
        m_capsuleTo(1, 0, 0),
        m_planeNormal(0, 0, 1),
        m_meshScale(1, 1, 1),
        m_hasLocalMaterial(false) {}
};

struct Shape {
  std::string m_sourceFileLocation;
  Magnum::Matrix4 m_linkLocalFrame;
  Geometry m_geometry;
  std::string m_name;
};

struct VisualShape : Shape {
  std::string m_materialName;
};

// TODO: need this?
enum CollisionFlags {
  FORCE_CONCAVE_TRIMESH = 1,
  HAS_COLLISION_GROUP = 2,
  HAS_COLLISION_MASK = 4,
};

struct CollisionShape : Shape {
  int m_flags;
  int m_collisionGroup;
  int m_collisionMask;
  CollisionShape() : m_flags(0) {}
};

struct Inertia {
  Magnum::Matrix4 m_linkLocalFrame;
  bool m_hasLinkLocalFrame;

  double m_mass;
  double m_ixx, m_ixy, m_ixz, m_iyy, m_iyz, m_izz;

  Inertia() {
    m_hasLinkLocalFrame = false;
    m_mass = 0.f;
    m_ixx = m_ixy = m_ixz = m_iyy = m_iyz = m_izz = 0.f;
  }
};

struct Joint {
  std::string m_name;
  JointTypes m_type;
  Magnum::Matrix4 m_parentLinkToJointTransform;
  std::string m_parentLinkName;
  std::string m_childLinkName;
  Magnum::Vector3 m_localJointAxis;

  double m_lowerLimit;
  double m_upperLimit;

  double m_effortLimit;
  double m_velocityLimit;

  double m_jointDamping;
  double m_jointFriction;
  Joint()
      : m_lowerLimit(0),
        m_upperLimit(-1),
        m_effortLimit(0),
        m_velocityLimit(0),
        m_jointDamping(0),
        m_jointFriction(0) {}
};

// TODO: need this?
enum LinkContactFlags {
  CONTACT_HAS_LATERAL_FRICTION = 1,
  CONTACT_HAS_INERTIA_SCALING = 2,
  CONTACT_HAS_CONTACT_CFM = 4,
  CONTACT_HAS_CONTACT_ERP = 8,
  CONTACT_HAS_STIFFNESS_DAMPING = 16,
  CONTACT_HAS_ROLLING_FRICTION = 32,
  CONTACT_HAS_SPINNING_FRICTION = 64,
  CONTACT_HAS_RESTITUTION = 128,
  CONTACT_HAS_FRICTION_ANCHOR = 256,
};

struct LinkContactInfo {
  float m_lateralFriction;
  float m_rollingFriction;
  float m_spinningFriction;
  float m_restitution;
  float m_inertiaScaling;
  float m_contactCfm;
  float m_contactErp;
  float m_contactStiffness;
  float m_contactDamping;

  int m_flags;

  LinkContactInfo()
      : m_lateralFriction(0.5),
        m_rollingFriction(0),
        m_spinningFriction(0),
        m_restitution(0),
        m_inertiaScaling(1),
        m_contactCfm(0),
        m_contactErp(0),
        m_contactStiffness(1e4),
        m_contactDamping(1) {
    m_flags = CONTACT_HAS_LATERAL_FRICTION;
  }
};

struct Link {
  std::string m_name;
  Inertia m_inertia;
  Magnum::Matrix4 m_linkTransformInWorld;
  std::vector<VisualShape> m_visualArray;
  std::vector<CollisionShape> m_collisionArray;
  std::shared_ptr<Link> m_parentLink;
  std::shared_ptr<Joint> m_parentJoint;

  std::vector<std::shared_ptr<Joint>> m_childJoints;
  std::vector<std::shared_ptr<Link>> m_childLinks;

  int m_linkIndex;

  LinkContactInfo m_contactInfo;

  Link() : m_parentLink(0), m_parentJoint(0), m_linkIndex(-2) {}
};

struct Model {
  std::string m_name;
  std::string m_sourceFile;
  Magnum::Matrix4 m_rootTransformInWorld(Magnum::Math::IdentityInitT);

  //! map of names to materials
  std::map<std::string, std::shared_ptr<Material>> m_materials;

  //! map of names to links
  std::map<std::string, std::shared_ptr<Link>> m_links;

  //! map of link indices to names
  std::map<int, std::string> m_linkIndicesToNames;

  //! map of names to joints
  std::map<std::string, std::shared_ptr<Joint>> m_joints;

  //! list of root links (usually 1)
  std::vector<std::shared_ptr<Link>> m_rootLinks;
  bool m_overrideFixedBase;

  void printKinematicChain() const;

  std::shared_ptr<Link> getLink(std::string linkName) const {
    if (m_links.count(linkName)) {
      return m_links.at(linkName);
    }
    return nullptr;
  }
  std::shared_ptr<Link> getLink(int linkIndex) const {
    if (m_linkIndicesToNames.count(linkIndex)) {
      return getLink(m_linkIndicesToNames.at(linkIndex));
    }
    return nullptr;
  }

  //! get the parent joint of a link
  std::shared_ptr<Joint> getJoint(int linkIndex) const {
    if (m_linkIndicesToNames.count(linkIndex)) {
      return getLink(m_linkIndicesToNames.at(linkIndex))->m_parentJoint;
    }
    return nullptr;
  }

  Model() : m_overrideFixedBase(false) {}
};

class Parser {
  // datastructures
  Model m_urdfModel;
  float m_urdfScaling = 1.0;

  // URDF file path of last load call
  std::string sourceFilePath_;

  // parser functions
  bool parseTransform(Magnum::Matrix4& tr, tinyxml2::XMLElement* xml);
  bool parseInertia(Inertia& inertia, tinyxml2::XMLElement* config);
  bool parseGeometry(Geometry& geom, tinyxml2::XMLElement* g);
  bool parseVisual(Model& model,
                   VisualShape& visual,
                   tinyxml2::XMLElement* config);
  bool parseCollision(CollisionShape& collision, tinyxml2::XMLElement* config);
  bool initTreeAndRoot(Model& model);
  bool parseMaterial(Material& material, tinyxml2::XMLElement* config);
  bool parseJointLimits(Joint& joint, tinyxml2::XMLElement* config);
  bool parseJointDynamics(Joint& joint, tinyxml2::XMLElement* config);
  bool parseJoint(Joint& joint, tinyxml2::XMLElement* config);
  bool parseLink(Model& model, Link& link, tinyxml2::XMLElement* config);
  bool parseSensor(Model& model,
                   Link& link,
                   Joint& joint,
                   tinyxml2::XMLElement* config);

  bool validateMeshFile(std::string& filename);

 public:
  Parser(){};

  // parse a loaded URDF string into relevant general data structures
  // return false if the string is not a valid urdf or other error causes abort
  bool parseURDF(const std::string& meshFilename);

  void setGlobalScaling(float scaling) { m_urdfScaling = scaling; }
  float getGlobalScaling() { return m_urdfScaling; }

  const Model& getModel() const { return m_urdfModel; }

  Model& getModel() { return m_urdfModel; }
};

}  // namespace URDF
}  // namespace io
}  // namespace esp
