// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#ifndef ESP_SENSOR_VISUALSENSOR_H_
#define ESP_SENSOR_VISUALSENSOR_H_

#include <Corrade/Containers/Optional.h>
#include <Magnum/Math/ConfigurationValue.h>

#include "esp/core/esp.h"

#include "esp/gfx/RenderCamera.h"
#include "esp/sensor/Sensor.h"

using Mn::Math::Literals::operator""_degf;

namespace esp {
namespace gfx {
class RenderTarget;
}

namespace sensor {
struct VisualSensorSpec : public SensorSpec {
  float ortho_scale;
  vec2i resolution;
  std::string encoding;  // For rendering colors in images
  bool gpu2gpuTransfer;  // True for pytorch tensor support
  VisualSensorSpec();
  void sanityCheck();
  bool operator==(const VisualSensorSpec& a);
  ESP_SMART_POINTERS(VisualSensorSpec)
};
// Represents a sensor that provides visual data from the environment to an
// agent
class VisualSensor : public Sensor {
 public:
  explicit VisualSensor(scene::SceneNode& node, VisualSensorSpec::ptr spec);
  virtual ~VisualSensor();

  /**
   * @brief Return the size of the framebuffer corresponding to the sensor's
   * resolution as a [W, H] Vector2i
   */
  Magnum::Vector2i framebufferSize() const {
    // NB: The sensor's resolution is in H x W format as that more cleanly
    // corresponds to the practice of treating images as arrays that is used in
    // modern CV and DL. However, graphics frameworks expect W x H format for
    // frame buffer sizes
    return {visualSensorSpec_->resolution[1], visualSensorSpec_->resolution[0]};
  }

  virtual bool isVisualSensor() override { return true; }

  /**
   * @brief Returns the parameters needed to unproject depth for the sensor.
   *
   * Will always be @ref Corrade::Containers::NullOpt for the base sensor class
   * as it has no projection parameters
   */
  virtual Corrade::Containers::Optional<Magnum::Vector2> depthUnprojection()
      const {
    return Corrade::Containers::NullOpt;
  };

  /**
   * @brief Checks to see if this sensor has a RenderTarget bound or not
   */
  bool hasRenderTarget() const { return tgt_ != nullptr; }

  /**
   * @brief Binds the given given RenderTarget to the sensor.  The sensor takes
   * ownership of the RenderTarget
   */
  void bindRenderTarget(std::unique_ptr<gfx::RenderTarget>&& tgt);

  /**
   * @brief Returns a reference to the sensors render target
   */
  gfx::RenderTarget& renderTarget() {
    if (!hasRenderTarget())
      throw std::runtime_error("Sensor has no rendering target");
    return *tgt_;
  }

  /**
   * @brief Draw an observation to the frame buffer using simulator's renderer
   * @return true if success, otherwise false (e.g., frame buffer is not set)
   * @param[in] sim Instance of Simulator class for which the observation needs
   *                to be drawn
   */
  virtual bool drawObservation(CORRADE_UNUSED sim::Simulator& sim) {
    return false;
  }

  /**
   * @brief Sets resolution of Sensor's sensorSpec
   */
  void setResolution(int height, int width);
  void setResolution(vec2i resolution);

  /**
   * @brief Returns RenderCamera
   */
  virtual gfx::RenderCamera* getRenderCamera() = 0;

  /**
   * @brief Gets near plane distance.
   */
  float getNear() { return near_; }

  /**
   * @brief Gets far plane distance.
   */
  float getFar() { return far_; }

  /**
   * @brief Returns the FOV of this Sensor
   */
  Mn::Deg getFOV() const { return hfov_; }

 protected:
  /** @brief projection parameters
   */

  /** @brief near clipping plane
   */
  float near_ = 0.01f;

  /** @brief far clipping plane
   */
  float far_ = 1000.0f;

  /** @brief field of view
   */
  Mn::Deg hfov_ = 90.0_degf;

  std::unique_ptr<gfx::RenderTarget> tgt_;
  VisualSensorSpec::ptr visualSensorSpec_ =
      std::dynamic_pointer_cast<VisualSensorSpec>(spec_);
  ESP_SMART_POINTERS(VisualSensor)
};

}  // namespace sensor
}  // namespace esp

#endif  // ESP_SENSOR_VISUALSENSOR_H_
