#include "gpupixel/custom_filter/beauty_filter.h"
#include "core/gpupixel_context.h"
#include "utils/util.h"

namespace gpupixel {
  const std::string kBeautyFragmentShaderString = R"(
    precision highp float;

    uniform sampler2D inputImageTexture;
    uniform vec2 singleStepOffset;
    uniform highp vec4 params;

    varying highp vec2 textureCoordinate;
    uniform float intensity;

    const highp vec3 W = vec3(0.299,0.587,0.114);
    const mat3 saturateMatrix = mat3(
        1.1102,-0.0598,-0.061,
        -0.0774,1.0826,-0.1186,
        -0.0228,-0.0228,1.1772);

    float hardlight(float color) {
      if (color <= 0.5) {
        color = color * color * 2.0;
      } else {
        color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);
      }
      return color;
    }

    void main() {
      vec2 blurCoordinates[24];

      blurCoordinates[0] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -10.0);
      blurCoordinates[1] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 10.0);
      blurCoordinates[2] = textureCoordinate.xy + singleStepOffset * vec2(-10.0, 0.0);
      blurCoordinates[3] = textureCoordinate.xy + singleStepOffset * vec2(10.0, 0.0);

      blurCoordinates[4] = textureCoordinate.xy + singleStepOffset * vec2(5.0, -8.0);
      blurCoordinates[5] = textureCoordinate.xy + singleStepOffset * vec2(5.0, 8.0);
      blurCoordinates[6] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, 8.0);
      blurCoordinates[7] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, -8.0);

      blurCoordinates[8] = textureCoordinate.xy + singleStepOffset * vec2(8.0, -5.0);
      blurCoordinates[9] = textureCoordinate.xy + singleStepOffset * vec2(8.0, 5.0);
      blurCoordinates[10] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, 5.0);
      blurCoordinates[11] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, -5.0);

      blurCoordinates[12] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -6.0);
      blurCoordinates[13] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 6.0);
      blurCoordinates[14] = textureCoordinate.xy + singleStepOffset * vec2(6.0, 0.0);
      blurCoordinates[15] = textureCoordinate.xy + singleStepOffset * vec2(-6.0, 0.0);

      blurCoordinates[16] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, -4.0);
      blurCoordinates[17] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, 4.0);
      blurCoordinates[18] = textureCoordinate.xy + singleStepOffset * vec2(4.0, -4.0);
      blurCoordinates[19] = textureCoordinate.xy + singleStepOffset * vec2(4.0, 4.0);

      blurCoordinates[20] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, -2.0);
      blurCoordinates[21] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, 2.0);
      blurCoordinates[22] = textureCoordinate.xy + singleStepOffset * vec2(2.0, -2.0);
      blurCoordinates[23] = textureCoordinate.xy + singleStepOffset * vec2(2.0, 2.0);

      float sampleColor = texture2D(inputImageTexture, textureCoordinate).g * 22.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[0]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[1]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[2]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[3]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[4]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[5]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[6]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[7]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[8]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[9]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[10]).g;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[11]).g;

      sampleColor += texture2D(inputImageTexture, blurCoordinates[12]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[13]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[14]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[15]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[16]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[17]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[18]).g * 2.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[19]).g * 2.0;

      sampleColor += texture2D(inputImageTexture, blurCoordinates[20]).g * 3.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[21]).g * 3.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[22]).g * 3.0;
      sampleColor += texture2D(inputImageTexture, blurCoordinates[23]).g * 3.0;

      sampleColor = sampleColor / 62.0;

      vec3 centralColor = texture2D(inputImageTexture, textureCoordinate).rgb;

      float highpass = centralColor.g - sampleColor + 0.5;

      for(int i = 0; i < 5;i++) {
        highpass = hardlight(highpass);
      }
      float lumance = dot(centralColor, W);

      float alpha = pow(lumance, params.r);

      vec3 smoothColor = centralColor + (centralColor-vec3(highpass))*alpha*0.1;

      smoothColor.r = clamp(pow(smoothColor.r, params.g),0.0,1.0);
      smoothColor.g = clamp(pow(smoothColor.g, params.g),0.0,1.0);
      smoothColor.b = clamp(pow(smoothColor.b, params.g),0.0,1.0);

      vec3 lvse = vec3(1.0)-(vec3(1.0)-smoothColor)*(vec3(1.0)-centralColor);
      vec3 bianliang = max(smoothColor, centralColor);
      vec3 rouguang = 2.0*centralColor*smoothColor + centralColor*centralColor - 2.0*centralColor*centralColor*smoothColor;

      gl_FragColor = vec4(mix(centralColor, lvse, alpha), 1.0);
      gl_FragColor.rgb = mix(gl_FragColor.rgb, bianliang, alpha);
      gl_FragColor.rgb = mix(gl_FragColor.rgb, rouguang, params.b);

      vec3 satcolor = gl_FragColor.rgb * saturateMatrix;
      vec4 textureColor = vec4(mix(gl_FragColor.rgb, satcolor, params.a), 1.0);
      vec4 originColor = texture2D(inputImageTexture, textureCoordinate);
      gl_FragColor = vec4(mix(originColor.rgb, textureColor.rgb, intensity), 1.0);
    }
  )";

  std::shared_ptr<BeautyFilter> BeautyFilter::Create() {
    auto ret = std::shared_ptr<BeautyFilter>(new BeautyFilter());
    gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
      if (ret && !ret->Init()) {
        ret.reset();
      }
    });
    return ret;
  }

  bool BeautyFilter::Init() {
    if (!InitWithFragmentShaderString(kBeautyFragmentShaderString)) {
      return false;
    }
    return true;
  }

  bool BeautyFilter::DoRender(bool updateSinks) {
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

    // vertex position
    GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                  imageVertices));

    filter_program_->SetUniformValue("params", Vector4(0.33f, 0.63f, 0.4f, 0.35f));
    if (texel_size_x_ != 0 && texel_size_y_ != 0) {
      filter_program_->SetUniformValue("singleStepOffset", Vector2(2.0f / texel_size_x_, 2.0f / texel_size_y_));
    }
    filter_program_->SetUniformValue("intensity", intensity);

    // draw
    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    framebuffer_->Deactivate();

    return Source::DoRender(updateSinks);
  }

  void BeautyFilter::setIntensity(float newIntensity) {
    intensity = newIntensity;
  }

  void BeautyFilter::setTexelWidth(int textureWidth) {
    texel_size_x_ = textureWidth;
  }

  void BeautyFilter::setTexelHeight(int textureHeight) {
    texel_size_y_ = textureHeight;
  }
}