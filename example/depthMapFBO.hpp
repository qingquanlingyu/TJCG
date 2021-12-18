#ifndef DEPTH_MAP_FBO_HPP
#define DEPTH_MAP_FBO_HPP

#include <iostream>
#include <glad/glad.h>

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

class DepthMapFBO {
public:
    DepthMapFBO();
    ~DepthMapFBO();
    void Bind();
    GLuint getDepthMap();
protected:
    GLuint m_fbo;
    GLuint m_depthMap;

private:
    void Init();
};

DepthMapFBO::DepthMapFBO()
{
    Init();

}

DepthMapFBO::~DepthMapFBO()
{

}

void DepthMapFBO::Init()
{
    // configure depth map FBO
    // -----------------------
    glGenFramebuffers(1, &m_fbo);
    // create depth texture
    glGenTextures(1, &m_depthMap);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DepthMapFBO::Bind()
{
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

GLuint DepthMapFBO::getDepthMap()
{
    return m_depthMap;
}

#endif
