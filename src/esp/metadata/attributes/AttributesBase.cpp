// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "AttributesBase.h"
#include "esp/physics/PhysicsObjectBase.h"
namespace esp {

namespace metadata {
namespace attributes {

const std::map<std::string, esp::gfx::LightType> LightTypeNamesMap = {
    {"point", esp::gfx::LightType::Point},
    {"directional", esp::gfx::LightType::Directional},
    {"spot", esp::gfx::LightType::Spot}};

const std::map<std::string, esp::gfx::LightPositionModel>
    LightPositionNamesMap = {{"global", esp::gfx::LightPositionModel::Global},
                             {"camera", esp::gfx::LightPositionModel::Camera},
                             {"object", esp::gfx::LightPositionModel::Object}};

const std::map<std::string, ObjectInstanceShaderType> ShaderTypeNamesMap = {
    {"material", ObjectInstanceShaderType::Material},
    {"flat", ObjectInstanceShaderType::Flat},
    {"phong", ObjectInstanceShaderType::Phong},
    {"pbr", ObjectInstanceShaderType::PBR},
};

const std::map<std::string, SceneInstanceTranslationOrigin>
    InstanceTranslationOriginMap = {
        {"asset_local", SceneInstanceTranslationOrigin::AssetLocal},
        {"com", SceneInstanceTranslationOrigin::COM},
};

// All keys must be lowercase
const std::map<std::string, esp::physics::MotionType> MotionTypeNamesMap = {
    {"static", esp::physics::MotionType::STATIC},
    {"kinematic", esp::physics::MotionType::KINEMATIC},
    {"dynamic", esp::physics::MotionType::DYNAMIC},
};

std::string getLightTypeName(esp::gfx::LightType lightTypeEnum) {
  if (lightTypeEnum < esp::gfx::LightType::Point ||
      lightTypeEnum > esp::gfx::LightType::Spot) {
    return "point";
  }
  // Must always be valid value
  for (const auto& it : LightTypeNamesMap) {
    if (it.second == lightTypeEnum) {
      return it.first;
    }
  }
  return "point";
}

std::string getLightPositionModelName(
    esp::gfx::LightPositionModel lightPositionEnum) {
  if (lightPositionEnum < esp::gfx::LightPositionModel::Camera ||
      lightPositionEnum > esp::gfx::LightPositionModel::Object) {
    return "global";
  }
  // Must always be valid value
  for (const auto& it : LightPositionNamesMap) {
    if (it.second == lightPositionEnum) {
      return it.first;
    }
  }
  return "global";
}

std::string getShaderTypeName(ObjectInstanceShaderType shaderTypeVal) {
  if (shaderTypeVal <= ObjectInstanceShaderType::Unknown ||
      shaderTypeVal >= ObjectInstanceShaderType::EndShaderType) {
    return "unspecified";
  }
  // Must always be valid value
  for (const auto& it : ShaderTypeNamesMap) {
    if (it.second == shaderTypeVal) {
      return it.first;
    }
  }
  return "unspecified";
}

std::string getTranslationOriginName(
    SceneInstanceTranslationOrigin translationOrigin) {
  if ((translationOrigin <= SceneInstanceTranslationOrigin::Unknown) ||
      (translationOrigin >= SceneInstanceTranslationOrigin::EndTransOrigin)) {
    return "default";
  }
  // Must always be valid value
  for (const auto& it : InstanceTranslationOriginMap) {
    if (it.second == translationOrigin) {
      return it.first;
    }
  }
  return "default";
}

std::string getMotionTypeName(esp::physics::MotionType motionTypeEnum) {
  if ((motionTypeEnum <= esp::physics::MotionType::UNDEFINED) ||
      (motionTypeEnum > esp::physics::MotionType::DYNAMIC)) {
    return "undefined";
  }
  // Must always be valid value
  for (const auto& it : MotionTypeNamesMap) {
    if (it.second == motionTypeEnum) {
      return it.first;
    }
  }
  return "undefined";
}

AbstractAttributes::AbstractAttributes(const std::string& attributesClassKey,
                                       const std::string& handle)
    : Configuration() {
  // set up an existing subgroup for user_defined attributes
  addSubgroup("user_defined");
  AbstractAttributes::setClassKey(attributesClassKey);
  AbstractAttributes::setHandle(handle);
  // set initial vals, will be overwritten when registered
  set("ID", 0);
  set("fileDirectory", "");
}

}  // namespace attributes
}  // namespace metadata
}  // namespace esp
