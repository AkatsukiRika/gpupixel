#include "gpupixel/filter/sharpen_filter.h"
#include "core/gpupixel_context.h"
#include "../../include/gpupixel/filter/sharpen_filter.h"


namespace gpupixel {
  const std::string kSharpenFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    uniform lowp float sharpness;
    uniform highp vec2 texelSize;
    varying highp vec2 textureCoordinate;

    void main() {
      lowp vec4 color = texture2D(inputImageTexture, textureCoordinate);
      lowp vec4 colorLeft = texture2D(inputImageTexture, textureCoordinate - vec2(texelSize.x, 0.0));
      lowp vec4 colorRight = texture2D(inputImageTexture, textureCoordinate + vec2(texelSize.x, 0.0));
      lowp vec4 colorUp = texture2D(inputImageTexture, textureCoordinate - vec2(0.0, texelSize.y));
      lowp vec4 colorDown = texture2D(inputImageTexture, textureCoordinate + vec2(0.0, texelSize.y));

      lowp vec4 sharpColor = color * (1.0 + 4.0 * sharpness)
                           - (colorLeft + colorRight + colorUp + colorDown) * sharpness;
      sharpColor = clamp(sharpColor, 0.0, 1.0);
      gl_FragColor = sharpColor;
    }
  )";

  std::shared_ptr<SharpenFilter> gpupixel::SharpenFilter::Create() {
    auto ret = std::shared_ptr<SharpenFilter>(new SharpenFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool SharpenFilter::Init() {
    if (!InitWithFragmentShaderString(kSharpenFragmentShaderString)) {
      return false;
    }
    sharpness_ = 0;
    RegisterProperty("sharpness", sharpness_,
                     "The sharpness of filter with range between 0 and 1.",
                     [this](float &sharpness) { setSharpness(sharpness); });
    RegisterProperty(
      "texel_width", 0,
      "The texture width of image.",
      [this](int &texelWidth) { setTexelWidth(texelWidth); });
    RegisterProperty(
      "texel_height", 0,
      "The texture height of image.",
      [this](int &texelHeight) { setTexelHeight(texelHeight); });
    return true;
  }

  bool SharpenFilter::DoRender(bool updateSinks) {
    filter_program_->SetUniformValue("sharpness", sharpness_);
    filter_program_->SetUniformValue("texelSize", Vector2(texel_size_x_, texel_size_y_));
    return Filter::DoRender(updateSinks);
  }

  void SharpenFilter::setSharpness(float sharpness) {
    sharpness_ = sharpness;
    if (sharpness_ < 0) {
      sharpness_ = 0;
    } else if (sharpness_ > 1) {
      sharpness_ = 1;
    }
  }

  void SharpenFilter::setTexelWidth(int textureWidth) {
    if (textureWidth > 0) {
      texel_size_x_ = 1.0f / textureWidth;
    }
  }

  void SharpenFilter::setTexelHeight(int textureHeight) {
    if (textureHeight > 0) {
      texel_size_y_ = 1.0f / textureHeight;
    }
  }
}