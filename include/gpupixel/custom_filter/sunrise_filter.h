/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "../filter/filter.h"
#include "../gpupixel_define.h"
#include "gpupixel/source/source_image.h"

namespace gpupixel {
  class GPUPIXEL_API SunriseFilter : public Filter {
  public:
    static std::shared_ptr<SunriseFilter> Create();
    bool Init();
    virtual bool DoRender(bool updateSinks = true) override;

    void setIntensity(float newIntensity);

  protected:
    SunriseFilter() {};

    std::shared_ptr<SourceImage> curve_image_;
    std::shared_ptr<SourceImage> grey_mask_image_1_;
    std::shared_ptr<SourceImage> grey_mask_image_2_;
    std::shared_ptr<SourceImage> grey_mask_image_3_;
    float intensity = 1;
  };
}
