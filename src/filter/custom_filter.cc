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

    fairy_tale_filter_->setIntensity(0);
    sunrise_filter_->setIntensity(0);
    sunset_filter_->setIntensity(0);

    fairy_tale_filter_
      ->AddSink(sunrise_filter_)
      ->AddSink(sunset_filter_);
    SetTerminalFilter(sunset_filter_);

    RegisterProperty("type", TYPE_ORIGINAL,
                     "The type of custom filter",
                     [this](int& val) { setType(val); });

    RegisterProperty("intensity", 0,
                     "The intensity of custom filter with range between 0 and 1.",
                     [this](float& val) { setIntensity(val); });

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
}