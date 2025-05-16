#include "gpupixel/custom_filter/fairy_tale_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kLookupVertexShaderString = R"(
    attribute vec4 position;
    attribute vec4 inputTextureCoordinate;

    varying vec2 textureCoordinate;

    void main() {
        gl_Position = position;
        textureCoordinate = inputTextureCoordinate.xy;
    }
  )";

  const std::string kLookupFragmentShaderString = R"(
    varying highp vec2 textureCoordinate;
    uniform sampler2D inputImageTexture;
    uniform sampler2D lookupImageTexture;
    uniform lowp float intensity;

    void main() {
        lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);

        mediump float blueColor = textureColor.b * 63.0;

        mediump vec2 quad1;
        quad1.y = floor(floor(blueColor) / 8.0);
        quad1.x = floor(blueColor) - (quad1.y * 8.0);

        mediump vec2 quad2;
        quad2.y = floor(ceil(blueColor) / 8.0);
        quad2.x = ceil(blueColor) - (quad2.y * 8.0);

        highp vec2 texPos1;
        texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

        highp vec2 texPos2;
        texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
        texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

        lowp vec4 newColor1 = texture2D(lookupImageTexture, texPos1);
        lowp vec4 newColor2 = texture2D(lookupImageTexture, texPos2);

        lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
        gl_FragColor = vec4(mix(textureColor.rgb, newColor.rgb, intensity), textureColor.w);
    }
  )";

  std::shared_ptr<FairyTaleFilter> FairyTaleFilter::Create() {
    auto ret = std::shared_ptr<FairyTaleFilter>(new FairyTaleFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool FairyTaleFilter::Init() {
    if (!InitWithShaderString(kLookupVertexShaderString, kLookupFragmentShaderString)) {
      return false;
    }

    auto path = Util::GetResourcePath() / "res";
    fairy_tale_image_ = SourceImage::Create((path / "lookup_fairy_tale.png").string());
    return true;
  }

  bool FairyTaleFilter::DoRender(bool updateSinks) {
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
    glBindTexture(GL_TEXTURE_2D, fairy_tale_image_->GetFramebuffer()->GetTexture());
    filter_program_->SetUniformValue("lookupImageTexture", 3);

    // vertex position
    GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                  imageVertices));

    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void FairyTaleFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }
}