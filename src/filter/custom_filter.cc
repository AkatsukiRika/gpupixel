#include "gpupixel/filter/custom_filter.h"
#include "core/gpupixel_context.h"
#include <unordered_map>

namespace gpupixel {
  std::shared_ptr<CustomFilter> CustomFilter::Create() {
    auto ret = std::shared_ptr<CustomFilter>(new CustomFilter());
    GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool CustomFilter::Init() {
    if (!FilterGroup::Init()) {
      return false;
    }

    fairy_tale_filter_ = FairyTaleFilter::Create();
    AddFilter(fairy_tale_filter_);

    sunrise_filter_ = SunriseFilter::Create();
    AddFilter(sunrise_filter_);

    sunset_filter_ = SunsetFilter::Create();
    AddFilter(sunset_filter_);

    white_cat_filter_ = WhiteCatFilter::Create();
    AddFilter(white_cat_filter_);

    black_cat_filter_ = BlackCatFilter::Create();
    AddFilter(black_cat_filter_);

    beauty_filter_ = BeautyFilter::Create();
    AddFilter(beauty_filter_);

    fairy_tale_filter_->setIntensity(0);
    sunrise_filter_->setIntensity(0);
    sunset_filter_->setIntensity(0);
    white_cat_filter_->setIntensity(0);
    black_cat_filter_->setIntensity(0);
    beauty_filter_->setIntensity(0);

    fairy_tale_filter_
      ->AddSink(sunrise_filter_)
      ->AddSink(sunset_filter_)
      ->AddSink(white_cat_filter_)
      ->AddSink(black_cat_filter_)
      ->AddSink(beauty_filter_);
    SetTerminalFilter(beauty_filter_);

    RegisterProperty("type", TYPE_ORIGINAL,
                     "The type of custom filter",
                     [this](int& val) { setType(val); });

    RegisterProperty("intensity", 0,
                     "The intensity of custom filter with range between 0 and 1.",
                     [this](float& val) { setIntensity(val); });

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

  bool CustomFilter::DoRender(bool updateSinks) {
    return FilterGroup::DoRender(updateSinks);
  }

  void CustomFilter::SetInputFramebuffer(std::shared_ptr <GPUPixelFramebuffer> framebuffer, gpupixel::RotationMode rotation_mode, int texIdx) {
    for (auto& filter : filters_) {
      filter->SetInputFramebuffer(framebuffer, rotation_mode, texIdx);
    }
  }

  void CustomFilter::setType(int newType) {
    type = newType;
  }

  void CustomFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;

    std::unordered_map<int, std::function<void(float)>> intensitySetters = {
      {
        TYPE_FAIRY_TALE,
        [this](float i) { fairy_tale_filter_->setIntensity(i); }
      },
      {
        TYPE_SUNRISE,
        [this](float i) { sunrise_filter_->setIntensity(i); }
      },
      {
        TYPE_SUNSET,
        [this](float i) { sunset_filter_->setIntensity(i); }
      },
      {
        TYPE_WHITE_CAT,
        [this](float i) { white_cat_filter_->setIntensity(i); }
      },
      {
        TYPE_BLACK_CAT,
        [this](float i) { black_cat_filter_->setIntensity(i); }
      },
      {
        TYPE_BEAUTY,
        [this](float i) { beauty_filter_->setIntensity(i); }
      }
    };

    for (const auto& setter : intensitySetters) {
      setter.second(0);
    }

    auto it = intensitySetters.find(type);
    if (it != intensitySetters.end()) {
      it->second(intensity);
    }
  }

  void CustomFilter::setTexelWidth(int textureWidth) {
    if (beauty_filter_) {
      beauty_filter_->setTexelWidth(textureWidth);
    }
  }

  void CustomFilter::setTexelHeight(int textureHeight) {
    if (beauty_filter_) {
      beauty_filter_->setTexelHeight(textureHeight);
    }
  }
}