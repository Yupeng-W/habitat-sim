// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
#include "CubeMapSensorBase.h"
#include "esp/core/Check.h"
#include "esp/sim/Simulator.h"

#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/FormatStl.h>

#include <algorithm>

namespace Mn = Magnum;
namespace Cr = Corrade;

namespace esp {
namespace sensor {

CubeMapSensorBaseSpec::CubeMapSensorBaseSpec() : VisualSensorSpec() {
  uuid = "cubemap_sensor_base";
  sensorSubType = SensorSubType::None;
}

void CubeMapSensorBaseSpec::sanityCheck() const {
  VisualSensorSpec::sanityCheck();
  if (cubemapSize != Cr::Containers::NullOpt) {
    ESP_CHECK(*cubemapSize > 0,
              "CubeMapSensorBaseSpec::sanityCheck(): the size of the cubemap,"
                  << *cubemapSize << "is illegal.");
  }
}

int computeCubemapSize(const esp::vec2i& resolution,
                       const Cr::Containers::Optional<int>& cubemapSize) {
  int size = std::min<int>(resolution[0], resolution[1]);
  // if user sets the size of the cubemap, use it
  if (cubemapSize != Corrade::Containers::NullOpt) {
    size = *cubemapSize;
  }
  return size;
}

CubeMapSensorBase::CubeMapSensorBase(scene::SceneNode& cameraNode,
                                     const CubeMapSensorBaseSpec::ptr& spec)
    : VisualSensor(cameraNode, spec) {
  // initialize a cubemap
  int size = computeCubemapSize(cubeMapSensorBaseSpec_->resolution,
                                cubeMapSensorBaseSpec_->cubemapSize);
  gfx::CubeMap::Flags cubeMapFlags = {};
  switch (cubeMapSensorBaseSpec_->sensorType) {
    case SensorType::Color:
      cubeMapFlags |= gfx::CubeMap::Flag::ColorTexture;
      break;
    case SensorType::Depth:
      cubeMapFlags |= gfx::CubeMap::Flag::DepthTexture;
      break;
    // TODO: Semantic
    default:
      CORRADE_INTERNAL_ASSERT_UNREACHABLE();
      break;
  }
  cubeMap_ = esp::gfx::CubeMap{size, cubeMapFlags};

  // initialize the cubemap camera, it attaches to the same node as the sensor
  // You do not have to release it in the dtor since magnum scene graph will
  // handle it
  cubeMapCamera_ = new gfx::CubeMapCamera(cameraNode);
  cubeMapCamera_->setProjectionMatrix(size, cubeMapSensorBaseSpec_->near,
                                      cubeMapSensorBaseSpec_->far);

  // setup shader flags
  switch (cubeMapSensorBaseSpec_->sensorType) {
    case SensorType::Color:
      cubeMapShaderBaseFlags_ |= gfx::CubeMapShaderBase::Flag::ColorTexture;
      break;
    case SensorType::Depth:
      cubeMapShaderBaseFlags_ |= gfx::CubeMapShaderBase::Flag::DepthTexture;
      break;
    // TODO: Semantic
    // sensor type list is too long, have to use default
    default:
      CORRADE_INTERNAL_ASSERT_UNREACHABLE();
      break;
  }

  // prepare a big triangle mesh to cover the screen
  mesh_ = Mn::GL::Mesh{};
  mesh_.setCount(3);
}

bool CubeMapSensorBaseSpec::operator==(const CubeMapSensorBaseSpec& a) const {
  return (VisualSensorSpec::operator==(a) && cubemapSize == a.cubemapSize);
}

bool CubeMapSensorBase::renderToCubemapTexture(sim::Simulator& sim) {
  if (!hasRenderTarget()) {
    return false;
  }

  // in case the fisheye sensor resolution changed at runtime
  {
    int size = computeCubemapSize(cubeMapSensorBaseSpec_->resolution,
                                  cubeMapSensorBaseSpec_->cubemapSize);
    bool reset = cubeMap_->reset(size);
    if (reset) {
      cubeMapCamera_->setProjectionMatrix(size, cubeMapSensorBaseSpec_->near,
                                          cubeMapSensorBaseSpec_->far);
    }
  }

  esp::gfx::RenderCamera::Flags flags = {gfx::RenderCamera::Flag::ClearColor |
                                         gfx::RenderCamera::Flag::ClearDepth};
  if (sim.isFrustumCullingEnabled()) {
    flags |= gfx::RenderCamera::Flag::FrustumCulling;
  }

  // generate the cubemap texture
  if (cubeMapSensorBaseSpec_->sensorType == SensorType::Semantic) {
    bool twoSceneGraphs =
        (&sim.getActiveSemanticSceneGraph() != &sim.getActiveSceneGraph());

    if (twoSceneGraphs) {
      VisualSensor::MoveSemanticSensorNodeHelper helper(*this, sim);
      cubeMap_->renderToTexture(*cubeMapCamera_,
                                sim.getActiveSemanticSceneGraph(), flags);
    } else {
      cubeMap_->renderToTexture(*cubeMapCamera_,
                                sim.getActiveSemanticSceneGraph(), flags);
    }

    if (twoSceneGraphs) {
      flags |= gfx::RenderCamera::Flag::ObjectsOnly;
      // Incremental rendering:
      // BE AWARE that here "ClearColor" and "ClearDepth" is NOT set!!
      // Rendering happens on top of whatever existing there.
      flags &= ~gfx::RenderCamera::Flag::ClearColor;
      flags &= ~gfx::RenderCamera::Flag::ClearDepth;
      cubeMap_->renderToTexture(*cubeMapCamera_, sim.getActiveSceneGraph(),
                                flags);
    }
  } else {
    cubeMap_->renderToTexture(*cubeMapCamera_, sim.getActiveSceneGraph(),
                              flags);
  }

  return true;
}

void CubeMapSensorBase::drawWith(gfx::CubeMapShaderBase& shader) {
  if (cubeMapSensorBaseSpec_->sensorType == SensorType::Color) {
    shader.bindColorTexture(
        cubeMap_->getTexture(gfx::CubeMap::TextureType::Color));
  }
  if (cubeMapSensorBaseSpec_->sensorType == SensorType::Depth) {
    shader.bindDepthTexture(
        cubeMap_->getTexture(gfx::CubeMap::TextureType::Depth));
  }
  renderTarget().renderEnter();
  shader.draw(mesh_);
  renderTarget().renderExit();
}

}  // namespace sensor
}  // namespace esp
