// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "esp/core/esp.h"
#include "esp/core/random.h"
#include "esp/scene/SceneConfiguration.h"
#include "esp/scene/SceneManager.h"
#include "esp/scene/SceneNode.h"

#include "esp/assets/ResourceManager.h"

#include "esp/gfx/RenderTarget.h"
#include "esp/gfx/WindowlessContext.h"

namespace esp {
namespace nav {
class PathFinder;
class ActionSpacePathFinder;
}  // namespace nav
namespace scene {
class SemanticScene;
}  // namespace scene
namespace gfx {

// forward declarations
class Renderer;

struct SimulatorConfiguration {
  scene::SceneConfiguration scene;
  int defaultAgentId = 0;
  int gpuDeviceId = 0;
  std::string defaultCameraUuid = "rgba_camera";
  bool compressTextures = false;
  bool createRenderer = true;

  bool enablePhysics = false;
  std::string physicsConfigFile =
      "./data/default.phys_scene_config.json";  // should we instead link a
                                                // PhysicsManagerConfiguration
                                                // object here?

  ESP_SMART_POINTERS(SimulatorConfiguration)
};
bool operator==(const SimulatorConfiguration& a,
                const SimulatorConfiguration& b);
bool operator!=(const SimulatorConfiguration& a,
                const SimulatorConfiguration& b);

class Simulator {
 public:
  explicit Simulator(const SimulatorConfiguration& cfg);
  virtual ~Simulator();

  virtual void reconfigure(const SimulatorConfiguration& cfg);

  virtual void reset();

  virtual void seed(uint32_t newSeed);

  std::shared_ptr<Renderer> getRenderer();
  std::shared_ptr<physics::PhysicsManager> getPhysicsManager();
  std::shared_ptr<scene::SemanticScene> getSemanticScene();

  scene::SceneGraph& getActiveSceneGraph();
  scene::SceneGraph& getActiveSemanticSceneGraph();

  void saveFrame(const std::string& filename);

  /**
   * @brief The ID of the CUDA device of the OpenGL context owned by the
   * simulator.  This will only be nonzero if the simulator is built in
   * --headless mode on linux
   */
  int gpuDevice() const { return context_->gpuDevice(); }

  // === Physics Simulator Functions ===
  // TODO: support multi-scene physics (default sceneID=0 currently).
  // create an object instance from ResourceManager
  // physicsObjectLibrary_[objectLibIndex] in scene sceneID. return the objectID
  // for the new object instance.
  int addObject(const int objectLibIndex, const int sceneID = 0);

  // return the current size of the physics object library (objects [0,size) can
  // be instanced)
  int getPhysicsObjectLibrarySize();

  // remove object objectID instance in sceneID
  void removeObject(const int objectID, const int sceneID = 0);

  // return a list of existing objected IDs in a physical scene
  std::vector<int> getExistingObjectIDs(const int sceneID = 0);

  // apply forces and torques to objects
  void applyTorque(const Magnum::Vector3& tau,
                   const int objectID,
                   const int sceneID = 0);

  void applyForce(const Magnum::Vector3& force,
                  const Magnum::Vector3& relPos,
                  const int objectID,
                  const int sceneID = 0);

  // set object transform (kinemmatic control)
  void setTransform(const Magnum::Matrix4& transform,
                    const int objectID,
                    const int sceneID = 0);

  Magnum::Matrix4 getTransformation(const int objectID, const int sceneID = 0);

  void setTransformation(const Magnum::Matrix4& transform,
                         const int objectID,
                         const int sceneID = 0);
  // set object translation directly
  void setTranslation(const Magnum::Vector3& translation,
                      const int objectID,
                      const int sceneID = 0);

  Magnum::Vector3 getTranslation(const int objectID, const int sceneID = 0);

  // set object rotation directly
  void setRotation(const Magnum::Quaternion& rotation,
                   const int objectID,
                   const int sceneID = 0);
  Magnum::Quaternion getRotation(const int objectID, const int sceneID = 0);

  // the physical world has a notion of time which passes during
  // animation/simulation/action/etc... return the new world time after stepping
  double stepWorld(const double dt = 1.0 / 60.0);

  // get the simulated world time (0 if no physics enabled)
  double getWorldTime();

 protected:
  Simulator(){};

  WindowlessContext::uptr context_ = nullptr;
  std::shared_ptr<Renderer> renderer_ = nullptr;
  // CANNOT make the specification of resourceManager_ above the context_!
  // Because when deconstructing the resourceManager_, it needs
  // the GL::Context
  // If you switch the order, you will have the error:
  // GL::Context::current(): no current context from Magnum
  // during the deconstruction
  assets::ResourceManager resourceManager_;

  scene::SceneManager sceneManager_;
  int activeSceneID_ = ID_UNDEFINED;
  int activeSemanticSceneID_ = ID_UNDEFINED;
  std::vector<int> sceneID_;

  std::shared_ptr<scene::SemanticScene> semanticScene_ = nullptr;

  std::shared_ptr<physics::PhysicsManager> physicsManager_ = nullptr;

  core::Random random_;
  SimulatorConfiguration config_;

  ESP_SMART_POINTERS(Simulator)
};

}  // namespace gfx
}  // namespace esp
