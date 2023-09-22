#ifndef ESP_GFX_BATCH_HBAO_H_
#define ESP_GFX_BATCH_HBAO_H_

#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

namespace esp {
namespace gfx_batch {

enum class HbaoFlag {
  Blur = 1 << 0,

  /**
   * Special blur is dependent on blur being set, and is ignored if blur is not
   * set.
   */
  UseAoSpecialBlur = 1 << 1,
  /* These two affect only the cache-aware variant. Only one can be set at
     most, not both (mutually exclusive) */
  LayeredImageLoadStore = 1 << 2,
  LayeredGeometryShader = 1 << 3
};
typedef Corrade::Containers::EnumSet<HbaoFlag> HbaoFlags;

CORRADE_ENUMSET_OPERATORS(HbaoFlags)

class HbaoConfiguration {
 public:
  Magnum::Vector2i size() const { return size_; }

  HbaoConfiguration& setSize(const Magnum::Vector2i& size) {
    size_ = size;
    return *this;
  }

  HbaoFlags flags() const { return flags_; }

  HbaoConfiguration& setFlags(HbaoFlags flags) {
    flags_ = flags;
    return *this;
  }

  // At least blur should always be used, otherwise there's nasty grid-like
  // artifacts
  // HbaoConfiguration& setUseBlur(bool state) {
  //   // special blur is dependent on blur being enabled, so both should be
  //   // disabled if blur is.
  //   flags_ = (state ? flags_ | HbaoFlag::Blur
  //                   : flags_ & ~(HbaoFlag::UseAoSpecialBlur |
  //                   HbaoFlag::Blur));
  //   return *this;
  // }

  HbaoConfiguration& setUseSpecialBlur(bool state) {
    // special blur is dependent on blur being enabled, so both should be
    // enabled if special blur is
    flags_ = (state ? flags_ | (HbaoFlag::UseAoSpecialBlur | HbaoFlag::Blur)
                    : flags_ & ~HbaoFlag::UseAoSpecialBlur);
    return *this;
  }

  HbaoConfiguration& setUseLayeredImageLoadStore(bool state) {
    // LayeredImageLoadStore and LayeredGeometryShader are mutually exclusive
    flags_ = (state ? (flags_ | HbaoFlag::LayeredImageLoadStore) &
                          ~HbaoFlag::LayeredGeometryShader
                    : flags_ & ~HbaoFlag::LayeredImageLoadStore);
    return *this;
  }
  HbaoConfiguration& setUseLayeredGeometryShader(bool state) {
    // LayeredImageLoadStore and LayeredGeometryShader are mutually exclusive
    flags_ = (state ? (flags_ | HbaoFlag::LayeredGeometryShader) &
                          ~HbaoFlag::LayeredImageLoadStore
                    : flags_ & ~HbaoFlag::LayeredGeometryShader);
    return *this;
  }

  Magnum::Int samples() const { return samples_; }

  HbaoConfiguration& setSamples(Magnum::Int samples) {
    samples_ = samples;
    return *this;
  }

  Magnum::Float intensity() const { return intensity_; }

  HbaoConfiguration& setIntensity(Magnum::Float intensity) {
    intensity_ = intensity;
    return *this;
  }

  Magnum::Float bias() const { return bias_; }

  HbaoConfiguration& setBias(Magnum::Float bias) {
    bias_ = bias;
    return *this;
  }

  Magnum::Float radius() const { return radius_; }

  HbaoConfiguration& setRadius(Magnum::Float radius) {
    radius_ = radius;
    return *this;
  }

  Magnum::Float blurSharpness() const { return blurSharpness_; }

  HbaoConfiguration& setBlurSharpness(Magnum::Float blurSharpness) {
    blurSharpness_ = blurSharpness;
    return *this;
  }

 private:
  Magnum::Vector2i size_;
  HbaoFlags flags_ = HbaoFlag::Blur;
  Magnum::Int samples_ = 1;
  Magnum::Float intensity_ = 0.732f, bias_ = 0.05f, radius_ = 1.84f,
                blurSharpness_ = 10.0f;
};  // class HbaoConfiguration

class Hbao {
 public:
  // Use this to construct before a GL context is ready
  explicit Hbao(Magnum::NoCreateT) noexcept;

  explicit Hbao(const HbaoConfiguration& configuration);

  Hbao(const Hbao&) = delete;
  Hbao(Hbao&&) noexcept;

  ~Hbao();

  Hbao& operator=(const Hbao&) = delete;
  Hbao& operator=(Hbao&&) noexcept;

  /**
   * @brief Set the configurable quantites of the HBAO alogirthm based on user
   * settings and defaults.
   */
  void setConfiguration(const HbaoConfiguration& configuration);

  /**
   * @brief Draw the HBAO effect on top of the current framebuffer.
   * @param projection The current visual sensor's projection matrix.
   * perspective projection matrix.
   * @param useCacheAware Whether to use the cache-aware algorithm or the
   * original/classic HBAO algorithm. The cache-aware algorithm has performance
   * optimizations.
   * @param inputDepthStencil The owning RenderTarget's depthRenderTexture
   * @param output The owning RenderTarget's framebuffer the effect is to be
   * written to.
   */
  void drawEffect(const Magnum::Matrix4& projection,
                  bool useCacheAware,
                  Magnum::GL::Texture2D& inputDepthStencil,
                  Magnum::GL::AbstractFramebuffer& output);

  /**
   * @brief Retrieve the size of the framebuffer used to build the components of
   * the HBAO algorithms.
   */
  Magnum::Vector2i getFrameBufferSize() const;

 private:
  void drawLinearDepth(const Magnum::Matrix4& projection,
                       Magnum::GL::Texture2D& inputDepthStencil);
  void drawHbaoBlur(Magnum::GL::AbstractFramebuffer& output);
  void drawClassicInternal(Magnum::GL::AbstractFramebuffer& output);
  void drawCacheAwareInternal(Magnum::GL::AbstractFramebuffer& output);

  struct State;
  Corrade::Containers::Pointer<State> state_;
};

}  // namespace gfx_batch
}  // namespace esp

#endif
