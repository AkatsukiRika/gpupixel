/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/filter.h"
#include "gpupixel/gpupixel_define.h"

namespace gpupixel {
  class GPUPIXEL_API SharpenFilter : public Filter {
  public:
    static std::shared_ptr<SharpenFilter> Create();
    bool Init();
    virtual bool DoRender(bool updateSinks = true) override;

    void setSharpness(float sharpness);
    void setTexelWidth(int textureWidth);
    void setTexelHeight(int textureHeight);

  protected:
    SharpenFilter() {};

    float sharpness_;
    float texel_size_x_;
    float texel_size_y_;
  };
}
