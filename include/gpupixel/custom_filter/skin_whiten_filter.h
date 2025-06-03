#pragma once

#include "../filter/filter.h"
#include "../gpupixel_define.h"
#include "gpupixel/source/source_image.h"

namespace gpupixel {
  class GPUPIXEL_API SkinWhitenFilter : public Filter {
  public:
    static std::shared_ptr<SkinWhitenFilter> Create();
    bool Init();
    virtual bool DoRender(bool updateSinks = true) override;

    void setIntensity(float newIntensity);
    void setTexelWidth(int textureWidth);
    void setTexelHeight(int textureHeight);

  protected:
    SkinWhitenFilter() {};

    std::shared_ptr<SourceImage> curve_image_;
    float intensity = 1;
    int texel_size_x_;
    int texel_size_y_;
  };
}
