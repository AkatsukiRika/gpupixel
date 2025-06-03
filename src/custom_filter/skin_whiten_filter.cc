#include "gpupixel/custom_filter/skin_whiten_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kSkinWhitenFragmentShaderString = R"(
    precision highp float;

    uniform sampler2D inputImageTexture;
    uniform sampler2D curve;

    uniform float texelWidthOffset;
    uniform float texelHeightOffset;
    uniform float intensity;

    varying mediump vec2 textureCoordinate;

    const mediump vec3 luminanceWeighting = vec3(0.2125, 0.7154, 0.0721);

    vec4 gaussianBlur(sampler2D sampler) {
      lowp float strength = 1.;
      vec4 color = vec4(0.);
      vec2 step  = vec2(0.);

      color += texture2D(sampler,textureCoordinate)* 0.25449 ;

      step.x = 1.37754 * texelWidthOffset  * strength;
      step.y = 1.37754 * texelHeightOffset * strength;
      color += texture2D(sampler,textureCoordinate+step) * 0.24797;
      color += texture2D(sampler,textureCoordinate-step) * 0.24797;

      step.x = 3.37754 * texelWidthOffset  * strength;
      step.y = 3.37754 * texelHeightOffset * strength;
      color += texture2D(sampler,textureCoordinate+step) * 0.09122;
      color += texture2D(sampler,textureCoordinate-step) * 0.09122;

      step.x = 5.37754 * texelWidthOffset  * strength;
      step.y = 5.37754 * texelHeightOffset * strength;

      color += texture2D(sampler,textureCoordinate+step) * 0.03356;
      color += texture2D(sampler,textureCoordinate-step) * 0.03356;

      return color;
    }

    void main() {
      vec4 blurColor;
      lowp vec4 textureColor;
      lowp float strength = -1.0 / 510.0;

      float xCoordinate = textureCoordinate.x;
      float yCoordinate = textureCoordinate.y;

      lowp float satura = 0.7;
      // naver skin
      textureColor = texture2D(inputImageTexture, textureCoordinate);
      blurColor = gaussianBlur(inputImageTexture);

      //saturation
      lowp float luminance = dot(blurColor.rgb, luminanceWeighting);
      lowp vec3 greyScaleColor = vec3(luminance);

      blurColor = vec4(mix(greyScaleColor, blurColor.rgb, satura), blurColor.w);

      lowp float redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
      lowp float greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).r;
      lowp float blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).r;

      redCurveValue = min(1.0, redCurveValue + strength);
      greenCurveValue = min(1.0, greenCurveValue + strength);
      blueCurveValue = min(1.0, blueCurveValue + strength);

      mediump vec4 overlay = blurColor;

      mediump vec4 base = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
      //gl_FragColor = overlay;

      // step4 overlay blending
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

      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<SkinWhitenFilter> SkinWhitenFilter::Create() {
    auto ret = std::shared_ptr<SkinWhitenFilter>(new SkinWhitenFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool SkinWhitenFilter::Init() {
    if (!InitWithFragmentShaderString(kSkinWhitenFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    curve_image_ = SourceImage::Create((path / "lookup_skin_whiten.png").string());
    return true;
  }

  bool SkinWhitenFilter::DoRender(bool updateSinks) {
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

    filter_program_->SetUniformValue("params", Vector4(0.33f, 0.63f, 0.4f, 0.35f));
    if (texel_size_x_ != 0) {
      filter_program_->SetUniformValue("texelWidthOffset", 1.0f / texel_size_x_);
    }
    if (texel_size_y_ != 0) {
      filter_program_->SetUniformValue("texelHeightOffset", 1.0f / texel_size_y_);
    }
    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void SkinWhitenFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }

  void SkinWhitenFilter::setTexelWidth(int textureWidth) {
    texel_size_x_ = textureWidth;
  }

  void SkinWhitenFilter::setTexelHeight(int textureHeight) {
    texel_size_y_ = textureHeight;
  }
}