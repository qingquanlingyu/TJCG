#ifndef _SSAO_HPP_
#define _SSAO_HPP_

#include <iostream>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>

#include "gbuffer.hpp"


float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

// SSAO 封装
/***************************************************
 * 考虑到 OpenGL 的上下文(context)相关性，封装SSAO比较勉强
 * 下面介绍一些要求：
 * @note Gbuffer 纹理顺序必须为 position, normal + ...
 * @note uniform 绑定纹理需要从 0 依次编号
 * @note 样本的数量是固定的，按照 learnopengl 网站示例
 * 
 **************************************************/
class SSAO {
public:
	SSAO(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum);
	~SSAO();
	void BindGbuffer();
    void BindSSAOFBO();
    void ActivateSSAOTexture(Shader&);
    void BindSSAOBlurFBO();
    void ActivateSSAOBlurTexture(Shader&);
    void ActivateTextureForLight();
    void BindGBufferForReading();
    void BindGBufferForWriting();
    void SetGBufferReadBuffer(unsigned int TextureType);
	
protected:
	void Init(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum);
	void GenerateKernel();
	void GenerateNoiseTexture();

private:
	GLuint ssaoFBO, ssaoBlurFBO;
	GLuint ssaoColorBuffer, ssaoColorBufferBlur;
	GBuffer *m_gbuffer;
    GLuint noiseTexture;
	
	// generates random floats between 0.0 and 1.0
	std::uniform_real_distribution<GLfloat> randomFloats{0.0, 1.0}; 
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise;

};

SSAO::SSAO(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum)
{
    Init(SCR_WIDTH, SCR_HEIGHT, textureNum);
    GenerateKernel();
    GenerateNoiseTexture();
}

SSAO::~SSAO()
{
    delete [] m_gbuffer;
    ssaoKernel.clear();
    ssaoNoise.clear();
}

void SSAO::Init(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum)
{
	m_gbuffer = new GBuffer(SCR_WIDTH, SCR_HEIGHT, textureNum, GBuffer::GBUFFER_SSAO);

	glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

// generate sample kernel
void SSAO::GenerateKernel()
{
	for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

}


// generate noise texture
void SSAO::GenerateNoiseTexture()
{
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

// 绑定 SSAO 中的 Gbuffer
void SSAO::BindGbuffer()
{
    m_gbuffer->Bind();
}

// 绑定 ssaoFBO
void SSAO::BindSSAOFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
}

// 此处仅仅是为了类的封装，projection 矩阵需要在外界设置
void SSAO::ActivateSSAOTexture(Shader& shaderSSAO)
{            
    shaderSSAO.use();
    // Send kernel + rotation
    for (unsigned int i = 0; i < 64; ++i)
        shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    
    // 要求顺序为 position, normal
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_gbuffer->m_textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_gbuffer->m_textures[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
}

// 绑定 SSAO blurFBO
void SSAO::BindSSAOBlurFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
}

// 激活 blur texture
void SSAO::ActivateSSAOBlurTexture(Shader& shaderSSAOBlur)
{
    shaderSSAOBlur.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
}

// 激活光照阶段需要的纹理
void SSAO::ActivateTextureForLight()
{
    m_gbuffer->ActiveBindTextures();
    // add extra SSAO texture to lighting pass
    glActiveTexture(GL_TEXTURE3); 
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
}

void SSAO::BindGBufferForWriting()
{
    m_gbuffer->BindForWriting();
}

void SSAO::BindGBufferForReading()
{
    m_gbuffer->BindForReading();
}

void SSAO::SetGBufferReadBuffer(unsigned int TextureType)
{
    m_gbuffer->SetReadBuffer(TextureType);
}

#endif
