// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "ObjectAttributesManager.h"
#include "AttributesManagerBase.h"

#include <Corrade/Utility/String.h>

#include "esp/assets/Asset.h"
#include "esp/io/io.h"
#include "esp/io/json.h"

using std::placeholders::_1;
namespace Cr = Corrade;

namespace esp {
namespace assets {

namespace managers {
PhysicsObjectAttributes::ptr ObjectAttributesManager::createAttributesTemplate(
    const std::string& attributesTemplateHandle,
    bool registerTemplate) {
  PhysicsObjectAttributes::ptr attrs;
  std::string msg;
  if (isValidPrimitiveAttributes(attributesTemplateHandle)) {
    // if attributesTemplateHandle == some existing primitive attributes, then
    // this is a primitive-based object we are building
    attrs = createPrimBasedAttributesTemplate(attributesTemplateHandle,
                                              registerTemplate);
    msg = "Primitive Asset (" + attributesTemplateHandle + ") Based";
  } else if (this->isValidFileName(attributesTemplateHandle)) {
    // if attributesTemplateHandle == some existing file then
    // assume this is a file-based object template we are building.
    attrs = createFileBasedAttributesTemplate(attributesTemplateHandle,
                                              registerTemplate);
    msg = "File (" + attributesTemplateHandle + ") Based";
  } else {
    // if neither of these is true, then build an empty template and assign the
    // passed handle to its origin handle and its render asset handle
    attrs = createDefaultAttributesTemplate(attributesTemplateHandle,
                                            registerTemplate);
    msg = "New default";
  }
  if (nullptr != attrs) {
    LOG(INFO) << msg << " object attributes created"
              << (registerTemplate ? " and registered." : ".");
  }
  return attrs;

}  // ObjectAttributesManager::createAttributesTemplate

PhysicsObjectAttributes::ptr
ObjectAttributesManager::createPrimBasedAttributesTemplate(
    const std::string& primAttrTemplateHandle,
    bool registerTemplate) {
  // verify that a primitive asset with the given handle exists
  if (!isValidPrimitiveAttributes(primAttrTemplateHandle)) {
    LOG(ERROR)
        << "ObjectAttributesManager::createPrimBasedAttributesTemplate : No "
           "primitive with handle '"
        << primAttrTemplateHandle
        << "' exists so cannot build physical object.  Aborting.";
    return nullptr;
  }

  // construct a PhysicsObjectAttributes
  auto primObjectAttributes = initNewAttribsInternal(
      PhysicsObjectAttributes::create(primAttrTemplateHandle));
  // set margin to be 0
  primObjectAttributes->setMargin(0.0);
  // make smaller as default size - prims are approx meter in size
  primObjectAttributes->setScale({0.1, 0.1, 0.1});

  // set render mesh handle
  int primType = static_cast<int>(AssetType::PRIMITIVE);
  primObjectAttributes->setRenderAssetType(primType);
  // set collision mesh/primitive handle and default for primitives to not use
  // mesh collisions
  primObjectAttributes->setCollisionAssetType(primType);
  primObjectAttributes->setUseMeshCollision(false);
  // NOTE to eventually use mesh collisions with primitive objects, a
  // collision primitive mesh needs to be configured and set in MeshMetaData
  // and CollisionMesh

  return this->postCreateRegister(primObjectAttributes, registerTemplate);
}  // ObjectAttributesManager::createPrimBasedAttributesTemplate

void ObjectAttributesManager::createDefaultPrimBasedAttributesTemplates() {
  this->defaultTemplateNames_.clear();
  // build default primtive object templates corresponding to given default
  // asset templates
  const std::vector<std::string> lib =
      assetAttributesMgr_->getDefaultTemplateHandles();
  for (std::string primAssetHandle : lib) {
    auto tmplt = createPrimBasedAttributesTemplate(primAssetHandle, true);
    // save handles in list of defaults, so they are not removed
    std::string tmpltHandle = tmplt->getHandle();
    this->defaultTemplateNames_.push_back(tmpltHandle);
  }
}  // ObjectAttributesManager::createDefaultPrimBasedAttributesTemplates

PhysicsObjectAttributes::ptr
ObjectAttributesManager::createFileBasedAttributesTemplate(
    const std::string& objPhysConfigFilename,
    bool registerTemplate) {
  // Load JSON config file
  io::JsonDocument jsonConfig;
  bool success = this->verifyLoadJson(objPhysConfigFilename, jsonConfig);
  if (!success) {
    LOG(ERROR)
        << "ObjectAttributesManager::createFileBasedAttributesTemplate : "
           "Failure reading json : "
        << objPhysConfigFilename << ". Aborting.";
    return nullptr;
  }

  // Construct a physicsObjectAttributes and populate with any
  // AbstractPhysicsAttributes fields found in json.
  auto objAttributes =
      this->createPhysicsAttributesFromJson<PhysicsObjectAttributes>(
          objPhysConfigFilename, jsonConfig);

  // Populate with object-specific fields found in json, if any are there
  // object mass
  io::jsonIntoSetter<double>(
      jsonConfig, "mass",
      std::bind(&PhysicsObjectAttributes::setMass, objAttributes, _1));

  // optional set bounding box as collision object
  io::jsonIntoSetter<bool>(
      jsonConfig, "use bounding box for collision",
      std::bind(&PhysicsObjectAttributes::setBoundingBoxCollisions,
                objAttributes, _1));

  //! Get collision configuration options if specified
  io::jsonIntoSetter<bool>(
      jsonConfig, "join collision meshes",
      std::bind(&PhysicsObjectAttributes::setJoinCollisionMeshes, objAttributes,
                _1));

  // object's interia matrix diag
  io::jsonIntoConstSetter<Magnum::Vector3>(
      jsonConfig, "inertia",
      std::bind(&PhysicsObjectAttributes::setInertia, objAttributes, _1));

  // the center of mass (in the local frame of the object)
  // if COM is provided, use it for mesh shift
  bool comIsSet = io::jsonIntoConstSetter<Magnum::Vector3>(
      jsonConfig, "COM",
      std::bind(&PhysicsObjectAttributes::setCOM, objAttributes, _1));
  // if com is set from json, don't compute from shape, and vice versa
  objAttributes->setComputeCOMFromShape(!comIsSet);

  return this->postCreateRegister(objAttributes, registerTemplate);
}  // ObjectAttributesManager::createFileBasedAttributesTemplate

PhysicsObjectAttributes::ptr
ObjectAttributesManager::createDefaultAttributesTemplate(
    const std::string& templateName,
    bool registerTemplate) {
  // construct a PhysicsObjectAttributes
  PhysicsObjectAttributes::ptr objAttributes =
      initNewAttribsInternal(PhysicsObjectAttributes::create(templateName));

  return this->postCreateRegister(objAttributes, registerTemplate);
}  // ObjectAttributesManager::createEmptyAttributesTemplate

PhysicsObjectAttributes::ptr ObjectAttributesManager::initNewAttribsInternal(
    PhysicsObjectAttributes::ptr newAttributes) {
  this->setFileDirectoryFromHandle(newAttributes);
  const std::string attributesHandle = newAttributes->getHandle();
  // set default render asset handle
  newAttributes->setRenderAssetHandle(attributesHandle);
  // set default collision asset handle
  newAttributes->setCollisionAssetHandle(attributesHandle);
  // set defaults for passed render asset handles
  setDefaultFileNameBasedAttributes(
      newAttributes, true, newAttributes->getRenderAssetHandle(),
      std::bind(&AbstractPhysicsAttributes::setRenderAssetType, newAttributes,
                _1));
  // set defaults for passed collision asset handles
  setDefaultFileNameBasedAttributes(
      newAttributes, false, newAttributes->getCollisionAssetHandle(),
      std::bind(&AbstractPhysicsAttributes::setCollisionAssetType,
                newAttributes, _1));

  return newAttributes;
}  // ObjectAttributesManager::initNewAttribsInternal

// Eventually support explicitly configuring desirable defaults/file-name
// base settings.
void ObjectAttributesManager::setDefaultFileNameBasedAttributes(
    PhysicsObjectAttributes::ptr attributes,
    bool setFrame,
    const std::string&,
    std::function<void(int)> meshTypeSetter) {
  // TODO : support future mesh-name specific type setting?
  meshTypeSetter(static_cast<int>(AssetType::UNKNOWN));

  if (setFrame) {
    attributes->setOrientUp({0, 1, 0});
    attributes->setOrientFront({0, 0, -1});
  }
}  // SceneAttributesManager::setDefaultFileNameBasedAttributes

int ObjectAttributesManager::registerAttributesTemplateFinalize(
    PhysicsObjectAttributes::ptr objectTemplate,
    const std::string& objectTemplateHandle) {
  if (objectTemplate->getRenderAssetHandle() == "") {
    LOG(ERROR)
        << "ObjectAttributesManager::registerAttributesTemplateFinalize : "
           "Attributes template named"
        << objectTemplateHandle
        << "does not have a valid render asset handle specified. Aborting.";
    return ID_UNDEFINED;
  }

  std::map<int, std::string>* mapToUse;
  // Handles for rendering and collision assets
  std::string renderAssetHandle = objectTemplate->getRenderAssetHandle();
  std::string collisionAssetHandle = objectTemplate->getCollisionAssetHandle();

  if (isValidPrimitiveAttributes(renderAssetHandle)) {
    // If renderAssetHandle corresponds to valid/existing primitive attributes
    // then setRenderAssetIsPrimitive to true and set map of IDs->Names to
    // physicsSynthObjTmpltLibByID_
    objectTemplate->setRenderAssetIsPrimitive(true);
    mapToUse = &physicsSynthObjTmpltLibByID_;
  } else if (this->isValidFileName(renderAssetHandle)) {
    // Check if renderAssetHandle is valid file name and is found in file system
    // - if so then setRenderAssetIsPrimitive to false and set map of IDs->Names
    // to physicsFileObjTmpltLibByID_ - verify file  exists
    objectTemplate->setRenderAssetIsPrimitive(false);
    mapToUse = &physicsFileObjTmpltLibByID_;
  } else {
    // If renderAssetHandle is neither valid file name nor existing primitive
    // attributes template hande, fail
    // by here always fail
    LOG(ERROR)
        << "ObjectAttributesManager::registerAttributesTemplateFinalize "
           ": Render asset template handle : "
        << renderAssetHandle << " specified in object template with handle : "
        << objectTemplateHandle
        << " does not correspond to any existing file or primitive render "
           "asset.  Aborting. ";
    return ID_UNDEFINED;
  }

  if (isValidPrimitiveAttributes(collisionAssetHandle)) {
    // If collisionAssetHandle corresponds to valid/existing primitive
    // attributes then setCollisionAssetIsPrimitive to true
    objectTemplate->setCollisionAssetIsPrimitive(true);
  } else if (this->isValidFileName(collisionAssetHandle)) {
    // Check if collisionAssetHandle is valid file name and is found in file
    // system - if so then setCollisionAssetIsPrimitive to false
    objectTemplate->setCollisionAssetIsPrimitive(false);
  } else {
    // Else, means no collision data specified, use specified render data
    LOG(INFO)
        << "ObjectAttributesManager::registerAttributesTemplateFinalize "
           ": Collision asset template handle : "
        << collisionAssetHandle
        << " specified in object template with handle : "
        << objectTemplateHandle
        << " does not correspond to any existing file or primitive render "
           "asset.  Overriding with given render asset handle : "
        << renderAssetHandle << ". ";

    objectTemplate->setCollisionAssetHandle(renderAssetHandle);
    objectTemplate->setCollisionAssetIsPrimitive(
        objectTemplate->getRenderAssetIsPrimitive());
  }

  // Clear dirty flag from when asset handles are changed
  objectTemplate->setIsClean();

  // Add object template to template library
  int objectTemplateID =
      this->addTemplateToLibrary(objectTemplate, objectTemplateHandle);

  mapToUse->emplace(objectTemplateID, objectTemplateHandle);

  return objectTemplateID;
}  // ObjectAttributesManager::registerAttributesTemplateFinalize

std::vector<int> ObjectAttributesManager::loadAllFileBasedTemplates(
    const std::vector<std::string>& tmpltFilenames,
    bool saveAsDefaults) {
  std::vector<int> resIDs(tmpltFilenames.size(), ID_UNDEFINED);
  for (int i = 0; i < tmpltFilenames.size(); ++i) {
    auto objPhysPropertiesFilename = tmpltFilenames[i];
    LOG(INFO) << "Loading file-based object template: "
              << objPhysPropertiesFilename;
    auto tmplt =
        createFileBasedAttributesTemplate(objPhysPropertiesFilename, true);

    // save handles in list of defaults, so they are not removed
    std::string tmpltHandle = tmplt->getHandle();
    this->defaultTemplateNames_.push_back(tmpltHandle);

    resIDs[i] = tmplt->getID();
  }
  LOG(INFO) << "Loaded file-based object templates: "
            << std::to_string(physicsFileObjTmpltLibByID_.size());
  return resIDs;
}  // ObjectAttributesManager::loadAllObjectTemplates

std::vector<int> ObjectAttributesManager::loadObjectConfigs(
    const std::string& path,
    bool saveAsDefaults) {
  std::vector<std::string> paths;
  std::vector<int> templateIndices;
  namespace Directory = Cr::Utility::Directory;
  std::string objPhysPropertiesFilename = path;
  if (!Cr::Utility::String::endsWith(objPhysPropertiesFilename,
                                     ".phys_properties.json")) {
    objPhysPropertiesFilename = path + ".phys_properties.json";
  }
  const bool dirExists = Directory::isDirectory(path);
  const bool fileExists = Directory::exists(objPhysPropertiesFilename);

  if (!dirExists && !fileExists) {
    LOG(WARNING) << "Cannot find " << path << " or "
                 << objPhysPropertiesFilename << ". Aborting parse.";
    return templateIndices;
  }

  if (fileExists) {
    paths.push_back(objPhysPropertiesFilename);
  }

  if (dirExists) {
    LOG(INFO) << "Parsing object library directory: " + path;
    for (auto& file : Directory::list(path, Directory::Flag::SortAscending)) {
      std::string absoluteSubfilePath = Directory::join(path, file);
      if (Cr::Utility::String::endsWith(absoluteSubfilePath,
                                        ".phys_properties.json")) {
        paths.push_back(absoluteSubfilePath);
      }
    }
  }
  // build templates from aggregated paths
  templateIndices = loadAllFileBasedTemplates(paths, saveAsDefaults);

  return templateIndices;
}  // ObjectAttributesManager::buildObjectConfigPaths

}  // namespace managers
}  // namespace assets
}  // namespace esp