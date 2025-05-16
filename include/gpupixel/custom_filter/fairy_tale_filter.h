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
  class GPUPIXEL_API FairyTaleFilter : public Filter {
  public:
    static std::shared_ptr<FairyTaleFilter> Create();
    bool Init();
    virtual bool DoRender(bool updateSinks = true) override;

    void setIntensity(float newIntensity);

  protected:
    FairyTaleFilter() {};

    std::shared_ptr<SourceImage> fairy_tale_image_;
    float intensity = 1;
  };
}
