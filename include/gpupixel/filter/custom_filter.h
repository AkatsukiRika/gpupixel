#pragma once

#include "gpupixel/filter/filter_group.h"
#include "gpupixel/custom_filter/fairy_tale_filter.h"
#include "gpupixel/custom_filter/sunrise_filter.h"
#include "gpupixel/custom_filter/sunset_filter.h"
#include "gpupixel/custom_filter/white_cat_filter.h"
#include "gpupixel/custom_filter/black_cat_filter.h"
#include "gpupixel/gpupixel_define.h"

namespace gpupixel {
  class GPUPIXEL_API CustomFilter : public FilterGroup {
  public:
    virtual ~CustomFilter() {};

    static std::shared_ptr<CustomFilter> Create();
    bool Init();
    virtual bool DoRender(bool updateSinks = true) override;

    void setType(int newType);
    void setIntensity(float newIntensity);

    virtual void SetInputFramebuffer(
      std::shared_ptr<GPUPixelFramebuffer> framebuffer,
      RotationMode rotation_mode = NoRotation,
      int texIdx = 0
    ) override;

  protected:
    CustomFilter() {};

    static constexpr int TYPE_ORIGINAL = 0;
    static constexpr int TYPE_FAIRY_TALE = 1;
    static constexpr int TYPE_SUNRISE = 2;
    static constexpr int TYPE_SUNSET = 3;
    static constexpr int TYPE_WHITE_CAT = 4;
    static constexpr int TYPE_BLACK_CAT = 5;

    std::shared_ptr<FairyTaleFilter> fairy_tale_filter_;
    std::shared_ptr<SunriseFilter> sunrise_filter_;
    std::shared_ptr<SunsetFilter> sunset_filter_;
    std::shared_ptr<WhiteCatFilter> white_cat_filter_;
    std::shared_ptr<BlackCatFilter> black_cat_filter_;
    int type = TYPE_ORIGINAL;
    float intensity = 0;
  };
}
