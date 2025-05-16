#include "gpupixel/custom_filter/sunrise_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kSunriseFragmentShaderString = R"(
    varying highp vec2 textureCoordinate;
    precision highp float;

    uniform sampler2D inputImageTexture;
    uniform sampler2D curve;

    uniform sampler2D grey1Frame;
    uniform sampler2D grey2Frame;
    uniform sampler2D grey3Frame;
    uniform float intensity;

    void main() {
      float GreyVal;
      lowp vec4 textureColor;
      lowp vec4 textureColorOri;
      float xCoordinate = textureCoordinate.x;
      float yCoordinate = textureCoordinate.y;

      highp float redCurveValue;
      highp float greenCurveValue;
      highp float blueCurveValue;

      vec4 grey1Color;
      vec4 grey2Color;
      vec4 grey3Color;

      textureColor = texture2D(inputImageTexture, vec2(xCoordinate, yCoordinate));

      grey1Color = texture2D(grey1Frame, vec2(xCoordinate, yCoordinate));
      grey2Color = texture2D(grey2Frame, vec2(xCoordinate, yCoordinate));
      grey3Color = texture2D(grey3Frame, vec2(xCoordinate, yCoordinate));

      mediump vec4 overlay = vec4(0, 0, 0, 1.0);
      mediump vec4 base = textureColor;

      // overlay blending
      mediump float ra;
      if (base.r < 0.5) {
        ra = overlay.r * base.r * 2.0;
      } else {
        ra = 1.0 - ((1.0 - base.r) * (1.0 - overlay.r) * 2.0);
      }

      mediump float ga;
      if (base.g < 0.5) {
        ga = overlay.g * base.g * 2.0;
      }
      else {
        ga = 1.0 - ((1.0 - base.g) * (1.0 - overlay.g) * 2.0);
      }

      mediump float ba;
      if (base.b < 0.5) {
        ba = overlay.b * base.b * 2.0;
      } else {
        ba = 1.0 - ((1.0 - base.b) * (1.0 - overlay.b) * 2.0);
      }

      textureColor = vec4(ra, ga, ba, 1.0);
      base = (textureColor - base) * (grey1Color.r*0.1019) + base;


      // step2 60% opacity  ExclusionBlending
      textureColor = vec4(base.r, base.g, base.b, 1.0);
      mediump vec4 textureColor2 = vec4(0.098, 0.0, 0.1843, 1.0);
      textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor);

      textureColor = (textureColor2 - textureColor) * 0.6 + textureColor;

      // step3 normal blending with original
      redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;

      textureColorOri = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      textureColor = (textureColorOri - textureColor) * grey2Color.r + textureColor;

      // step4 normal blending with original
      redCurveValue = texture2D(curve, vec2(textureColor.r, 1.0)).r;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 1.0)).g;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 1.0)).b;

      textureColorOri = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      textureColor = (textureColorOri - textureColor) * (grey3Color.r) * 1.0 + textureColor;


      overlay = vec4(0.6117, 0.6117, 0.6117, 1.0);
      base = textureColor;
      // overlay blending
      if (base.r < 0.5) {
        ra = overlay.r * base.r * 2.0;
      } else {
        ra = 1.0 - ((1.0 - base.r) * (1.0 - overlay.r) * 2.0);
      }

      if (base.g < 0.5) {
        ga = overlay.g * base.g * 2.0;
      } else {
        ga = 1.0 - ((1.0 - base.g) * (1.0 - overlay.g) * 2.0);
      }

      if (base.b < 0.5) {
        ba = overlay.b * base.b * 2.0;
      } else {
        ba = 1.0 - ((1.0 - base.b) * (1.0 - overlay.b) * 2.0);
      }

      textureColor = vec4(ra, ga, ba, 1.0);
      base = (textureColor - base) + base;

      // step5-2 30% opacity  ExclusionBlending
      textureColor = vec4(base.r, base.g, base.b, 1.0);
      textureColor2 = vec4(0.113725, 0.0039, 0.0, 1.0);
      textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor);

      base = (textureColor2 - textureColor) * 0.3 + textureColor;
      redCurveValue = texture2D(curve, vec2(base.r, 1.0)).a;
      greenCurveValue = texture2D(curve, vec2(base.g, 1.0)).a;
      blueCurveValue = texture2D(curve, vec2(base.b, 1.0)).a;

      // step6 screen with 60%
      base = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      overlay = vec4(1.0, 1.0, 1.0, 1.0);

      // screen blending
      textureColor = 1.0 - ((1.0 - base) * (1.0 - overlay));
      textureColor = (textureColor - base) * 0.05098 + base;

      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<SunriseFilter> SunriseFilter::Create() {
    auto ret = std::shared_ptr<SunriseFilter>(new SunriseFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool SunriseFilter::Init() {
    if (!InitWithFragmentShaderString(kSunriseFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    curve_image_ = SourceImage::Create((path / "lookup_sunrise.png").string());
    grey_mask_image_1_ = SourceImage::Create((path / "amaro_mask1.jpg").string());
    grey_mask_image_2_ = SourceImage::Create((path / "amaro_mask2.jpg").string());
    grey_mask_image_3_ = SourceImage::Create((path / "toy_mask1.jpg").string());
    return true;
  }

  bool SunriseFilter::DoRender(bool updateSinks) {
    static const float imageVertices[] = {
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    };

    GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);
    framebuffer_->Activate();
    GL_CALL(glClearColor(background_color_.r, background_color_.g,
                         background_color_.b, background_color_.a));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

    GL_CALL(glActiveTexture(GL_TEXTURE2));
    GL_CALL(glBindTexture(GL_TEXTURE_2D,
                          input_framebuffers_[0].frame_buffer->GetTexture()));
    filter_program_->SetUniformValue("inputImageTexture", 2);

    // texcoord attribute
    uint32_t filter_tex_coord_attribute =
      filter_program_->GetAttribLocation("inputTextureCoordinate");
    GL_CALL(glEnableVertexAttribArray(filter_tex_coord_attribute));
    GL_CALL(glVertexAttribPointer(
      filter_tex_coord_attribute, 2, GL_FLOAT, 0, 0,
      GetTextureCoordinate(input_framebuffers_[0].rotation_mode)));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, curve_image_->GetFramebuffer()->GetTexture());
    filter_program_->SetUniformValue("curve", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, grey_mask_image_1_->GetFramebuffer()->GetTexture());
    filter_program_->SetUniformValue("grey1Frame", 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, grey_mask_image_2_->GetFramebuffer()->GetTexture());
    filter_program_->SetUniformValue("grey2Frame", 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, grey_mask_image_3_->GetFramebuffer()->GetTexture());
    filter_program_->SetUniformValue("grey3Frame", 6);

    // vertex position
    GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                  imageVertices));

    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void SunriseFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }
}