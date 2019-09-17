// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>

#include "PinholeCamera.h"
#include "esp/gfx/DepthUnprojection.h"
#include "esp/gfx/Renderer.h"
#include "esp/gfx/Simulator.h"

namespace esp {
namespace sensor {

PinholeCamera::PinholeCamera(scene::SceneNode& pinholeCameraNode,
                             sensor::SensorSpec::ptr spec)
    : sensor::Sensor(pinholeCameraNode, spec) {
  setProjectionParameters(spec);
}

void PinholeCamera::setProjectionParameters(SensorSpec::ptr spec) {
  ASSERT(spec != nullptr);
  width_ = spec_->resolution[1];
  height_ = spec_->resolution[0];
  near_ = std::atof(spec_->parameters.at("near").c_str());
  far_ = std::atof(spec_->parameters.at("far").c_str());
  hfov_ = std::atof(spec_->parameters.at("hfov").c_str());
}

void PinholeCamera::setProjectionMatrix(gfx::RenderCamera& targetCamera) {
  targetCamera.setProjectionMatrix(width_, height_, near_, far_, hfov_);
}

bool PinholeCamera::getObservationSpace(ObservationSpace& space) {
  space.spaceType = ObservationSpaceType::TENSOR;
  space.shape = {static_cast<size_t>(spec_->resolution[0]),
                 static_cast<size_t>(spec_->resolution[1]),
                 static_cast<size_t>(spec_->channels)};
  space.dataType = core::DataType::DT_UINT8;
  if (spec_->sensorType == SensorType::SEMANTIC) {
    space.dataType = core::DataType::DT_UINT32;
  } else if (spec_->sensorType == SensorType::DEPTH) {
    space.dataType = core::DataType::DT_FLOAT;
  }
  return true;
}

bool PinholeCamera::getObservation(gfx::Simulator& sim, Observation& obs) {
  // TODO: check if sensor is valid?
  // TODO: have different classes for the different types of sensors
  //
  if (!hasRenderTarget())
    return false;

  drawObservation(sim);
  readObservation(obs);

  return true;
}

void PinholeCamera::drawObservation(gfx::Simulator& sim) {
  renderTarget().renderEnter();

  gfx::Renderer::ptr renderer = sim.getRenderer();
  if (spec_->sensorType == SensorType::SEMANTIC) {
    // TODO: check sim has semantic scene graph
    renderer->draw(*this, sim.getActiveSemanticSceneGraph());
  } else {
    // SensorType is DEPTH or any other type
    renderer->draw(*this, sim.getActiveSceneGraph());
  }

  renderTarget().renderExit();
}

void PinholeCamera::readObservation(Observation& obs) {
  // Make sure we have memory
  if (buffer_ == nullptr) {
    // TODO: check if our sensor was resized and resize our buffer if needed
    ObservationSpace space;
    getObservationSpace(space);
    buffer_ = core::Buffer::create(space.shape, space.dataType);
  }
  obs.buffer = buffer_;

  // TODO: have different classes for the different types of sensors
  // TODO: do we need to flip axis?
  if (spec_->sensorType == SensorType::SEMANTIC) {
    renderTarget().readFrameObjectId(Magnum::MutableImageView2D{
        Magnum::PixelFormat::R32UI, renderTarget().framebufferSize(),
        obs.buffer->data});
  } else if (spec_->sensorType == SensorType::DEPTH) {
    renderTarget().readFrameDepth(Magnum::MutableImageView2D{
        Magnum::PixelFormat::R32F, renderTarget().framebufferSize(),
        obs.buffer->data});
  } else {
    renderTarget().readFrameRgba(Magnum::MutableImageView2D{
        Magnum::PixelFormat::RGBA8Unorm, renderTarget().framebufferSize(),
        obs.buffer->data});
  }
}

bool PinholeCamera::displayObservation(gfx::Simulator& sim) {
  if (!hasRenderTarget()) {
    return false;
  }

  renderTarget().renderEnter();

  drawObservation(sim);
  renderTarget().blitRgbaToDefault();

  renderTarget().renderExit();

  return true;
}

Corrade::Containers::Optional<Magnum::Vector2>
PinholeCamera::depthUnprojection() const {
  const Magnum::Matrix4 projection = Magnum::Matrix4::perspectiveProjection(
      Magnum::Deg{hfov_}, static_cast<float>(width_) / height_, near_, far_);

  return {gfx::calculateDepthUnprojection(projection)};
}

}  // namespace sensor
}  // namespace esp
