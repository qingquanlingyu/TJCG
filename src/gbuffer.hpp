#ifndef _GBUFFER_HPP_
#define _GBUFFER_HPP_

#include <glad/glad.h>

#include <iostream>

class SSAO;
class GBuffer {
	friend SSAO;
public:
	/* 推荐方式，为纹理创建 enum 类型 */
	// enum GBUFFER_TEXTURE_TYPE {
	// 	GBUFFER_TEXTURE_POSITION,
	// 	GBUFFER_TEXTURE_NORMAL,
	// 	GBUFFER_TEXTURE_ALBEDOSPEC,	// color + specular
	// 	GBUFFER_TEXTURE_NUM
	// };

	// GBuffer 用途设置
	enum GBUFFER_USAGE {
		GBUFFER_DEFAULT,	// 默认用途
		GBUFFER_SSAO		// SSAO 用途
	};

	/* 构造函数传入长度和宽度以及纹理的数量 */
	GBuffer(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum, GBUFFER_USAGE = GBUFFER_DEFAULT);

	~GBuffer();
	bool Init(GLuint SCR_WIDTH, GLuint SCR_HEIGHT);
	void BindForWriting();
	void BindForReading();
	// 绑定 FBO
	void Bind();
	void SetReadBuffer(unsigned int TextureType);
	// 激活并且绑定纹理
	void ActiveBindTextures();

private:
	unsigned int GBUFFER_TEXTURE_NUM;
	GLuint m_fbo;
	GLuint* m_textures;
	GLuint m_rboDepth;
	GBUFFER_USAGE m_usage;
};

// 构造函数
GBuffer::GBuffer(GLuint SCR_WIDTH, GLuint SCR_HEIGHT, unsigned int textureNum, GBUFFER_USAGE usage)
{
	GBUFFER_TEXTURE_NUM = textureNum;
	m_usage = usage;
	Init(SCR_WIDTH, SCR_HEIGHT);
}

// 析构函数
GBuffer::~GBuffer()
{
	delete[] m_textures;
}

// 初始化函数
bool GBuffer::Init(GLuint SCR_WIDTH, GLuint SCR_HEIGHT)
{
	m_textures = new GLuint(GBUFFER_TEXTURE_NUM);
	// configure g-buffer framebuffer
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	// create the gbuffer textures
	glGenTextures(GBUFFER_TEXTURE_NUM, m_textures);
	GLuint* attachments = new GLuint[GBUFFER_TEXTURE_NUM];
	for (unsigned int i = 0; i < GBUFFER_TEXTURE_NUM; i++) {
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// 如果作为 SSAO 用途，则需要此设置
		if (i == 0 && m_usage == GBUFFER_SSAO) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	glDrawBuffers(GBUFFER_TEXTURE_NUM, attachments);
	delete[] attachments;

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, m_rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer not complete!" << std::endl;
		return false;
	}

	// restore default FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}


void GBuffer::BindForWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
}

void GBuffer::BindForReading()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
}

void GBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void GBuffer::SetReadBuffer(unsigned int TextureType)
{
	glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
}

void GBuffer::ActiveBindTextures()
{
	for (unsigned int i = 0; i < GBUFFER_TEXTURE_NUM; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
	}
}
#endif
