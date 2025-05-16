#include "gpupixel/custom_filter/sunset_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kSunsetFragmentShaderString = R"(
    varying highp vec2 textureCoordinate;

    precision highp float;
    uniform sampler2D inputImageTexture;
    uniform sampler2D curve;

    uniform sampler2D grey1Frame;
    uniform sampler2D grey2Frame;
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

      textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate));
      grey1Color = texture2D(grey2Frame, vec2(xCoordinate, yCoordinate));
      grey2Color = texture2D(grey1Frame, vec2(xCoordinate, yCoordinate));

      // step1 normal blending with original
      redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;

      textureColorOri = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      textureColor = (textureColorOri - textureColor) * grey1Color.r + textureColor;

      redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).a;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).a;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).a;

      //textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);

      // step3 60% opacity  ExclusionBlending
      textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      mediump vec4 textureColor2 = vec4(0.08627, 0.03529, 0.15294, 1.0);
      textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor);

      textureColor = (textureColor2 - textureColor) * 0.6784 + textureColor;


      mediump vec4 overlay = vec4(0.6431, 0.5882, 0.5803, 1.0);
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
      } else {
        ga = 1.0 - ((1.0 - base.g) * (1.0 - overlay.g) * 2.0);
      }

      mediump float ba;
      if (base.b < 0.5) {
        ba = overlay.b * base.b * 2.0;
      } else {
        ba = 1.0 - ((1.0 - base.b) * (1.0 - overlay.b) * 2.0);
      }

      textureColor = vec4(ra, ga, ba, 1.0);
      base = (textureColor - base) + base;

      // again overlay blending
      overlay = vec4(0.0, 0.0, 0.0, 1.0);

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
      textureColor = (textureColor - base) * (grey2Color * 0.549) + base;

      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<SunsetFilter> SunsetFilter::Create() {
    auto ret = std::shared_ptr<SunsetFilter>(new SunsetFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool SunsetFilter::Init() {
    if (!InitWithFragmentShaderString(kSunsetFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    curve_image_ = SourceImage::Create((path / "lookup_sunset.png").string());
    grey_mask_image_1_ = SourceImage::Create((path / "rise_mask1.jpg").string());
    grey_mask_image_2_ = SourceImage::Create((path / "rise_mask2.jpg").string());
    return true;
  }

  bool SunsetFilter::DoRender(bool updateSinks) {
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

    // vertex position
    GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                  imageVertices));

    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void SunsetFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }
}