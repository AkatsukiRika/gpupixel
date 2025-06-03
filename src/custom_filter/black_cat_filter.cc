#include "gpupixel/custom_filter/black_cat_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kBlackCatFragmentShaderString = R"(
    varying highp vec2 textureCoordinate;
    precision highp float;

    uniform sampler2D inputImageTexture;
    uniform sampler2D curve;
    uniform float intensity;

    vec3 rgb2hsv(vec3 c) {
      vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
      vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
      vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

      float d = q.x - min(q.w, q.y);
      float e = 1.0e-10;
      return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
    }

    vec3 hsv2rgb(vec3 c) {
      vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
      vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
      return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }

    void main() {
      float GreyVal;
      lowp vec4 textureColor;
      lowp vec4 textureColorOri;
      float xCoordinate = textureCoordinate.x;
      float yCoordinate = textureCoordinate.y;

      highp float redCurveValue;
      highp float greenCurveValue;
      highp float blueCurveValue;
      textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate));
      // step1 curve
      redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;


      //textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      vec3 tColor = vec3(redCurveValue, greenCurveValue, blueCurveValue);
      tColor = rgb2hsv(tColor);

      tColor.g = tColor.g * 1.2;

      float dStrength = 1.0;
      float dSatStrength = 0.3;

      float dGap = 0.0;

      if (tColor.r >= 0.0 && tColor.r < 0.417) {
        tColor.g = tColor.g + (tColor.g * dSatStrength);
      } else if (tColor.r > 0.958 && tColor.r <= 1.0) {
        tColor.g = tColor.g + (tColor.g * dSatStrength);
      } else if (tColor.r >= 0.875 && tColor.r <= 0.958) {
        dGap = abs(tColor.r - 0.875);
        dStrength = (dGap / 0.0833);

        tColor.g = tColor.g + (tColor.g * dSatStrength * dStrength);
      } else if (tColor.r >= 0.0417 && tColor.r <= 0.125) {
        dGap = abs(tColor.r - 0.125);
        dStrength = (dGap / 0.0833);

        tColor.g = tColor.g + (tColor.g * dSatStrength * dStrength);
      }

      tColor = hsv2rgb(tColor);
      tColor = clamp(tColor, 0.0, 1.0);

      redCurveValue = texture2D(curve, vec2(tColor.r, 1.0)).r;
      greenCurveValue = texture2D(curve, vec2(tColor.g, 1.0)).r;
      blueCurveValue = texture2D(curve, vec2(tColor.b, 1.0)).r;

      redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).g;
      greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).g;
      blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).g;

      textureColor = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);

      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<BlackCatFilter> BlackCatFilter::Create() {
    auto ret = std::shared_ptr<BlackCatFilter>(new BlackCatFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool BlackCatFilter::Init() {
    if (!InitWithFragmentShaderString(kBlackCatFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    curve_image_ = SourceImage::Create((path / "lookup_black_cat.png").string());
    return true;
  }

  bool BlackCatFilter::DoRender(bool updateSinks) {
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

    // vertex position
    GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                  imageVertices));

    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void BlackCatFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }
}