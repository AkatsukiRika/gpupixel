#include "gpupixel/custom_filter/white_cat_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kWhiteCatFragmentShaderString = R"(
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

      // step1 20% opacity  ExclusionBlending
      mediump vec4 textureColor2 = textureColor;
      textureColor2 = textureColor + textureColor2 - (2.0 * textureColor2 * textureColor);

      textureColor = (textureColor2 - textureColor) * 0.2 + textureColor;

      // step2 curve
      redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
      greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
      blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;

      redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).r;
      greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).r;
      blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).r;

      redCurveValue = texture2D(curve, vec2(redCurveValue, 1.0)).g;
      greenCurveValue = texture2D(curve, vec2(greenCurveValue, 1.0)).g;
      blueCurveValue = texture2D(curve, vec2(blueCurveValue, 1.0)).g;


      vec3 tColor = vec3(redCurveValue, greenCurveValue, blueCurveValue);
      tColor = rgb2hsv(tColor);

      tColor.g = tColor.g * 0.65;

      tColor = hsv2rgb(tColor);
      tColor = clamp(tColor, 0.0, 1.0);

      mediump vec4 base = vec4(tColor, 1.0);
      mediump vec4 overlay = vec4(0.62, 0.6, 0.498, 1.0);
      // step6 overlay blending
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
      textureColor = (textureColor - base) * 0.1 + base;

      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<WhiteCatFilter> WhiteCatFilter::Create() {
    auto ret = std::shared_ptr<WhiteCatFilter>(new WhiteCatFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool WhiteCatFilter::Init() {
    if (!InitWithFragmentShaderString(kWhiteCatFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    curve_image_ = SourceImage::Create((path / "lookup_white_cat.png").string());
    return true;
  }

  bool WhiteCatFilter::DoRender(bool updateSinks) {
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

  void WhiteCatFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }
}