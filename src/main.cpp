#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shadow.h"
#include "camera.h"
#include "texture.h"
#include "model.h"
#include "ssao.hpp"
#include "SkyDome.h"

#include "ocean.h"
#include "object.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderQuad();
void RenderSkybox();
void RenderSphere();
void SendUniformMVP();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLfloat exposure = 1.0f; 

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Nature v2.0", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//隐藏光标 

    glEnable(GL_DEPTH_TEST);//开启深度测试
    glDepthFunc(GL_LESS);

    //stbi_set_flip_vertically_on_load(true);
    Shader shaderGeometryPass("../shader/geometry.vs", "../shader/geometry.fs");
    Shader SSAOshader("../shader/ssao.vs", "../shader/ssao.fs");
    Shader SSAOBlurShader("../shader/ssao.vs", "../shader/ssao_blur.fs");
    Shader MainShader("../shader/ssao.vs", "../shader/PBR.fs");
    Shader screenShader("../shader/screen.vs", "../shader/screen.fs");
    Shader shadowShader("../shader/shadow.vs", "../shader/shadow.fs");
    Shader blurShader("../shader/blur.vs", "../shader/blur.fs");
    Shader lightShader("../shader/light.vs", "../shader/light.fs");
    Shader SkyDomeShader("../shader/SkyDomeShader.vs", "../shader/SkyDomeShader.fs");

    
    Model ourModel("../res/model/untitled.obj");

    DirShadow dirshadow;
    enum GBUFFER_TEXTURE_TYPE {
        GBUFFER_TEXTURE_POSITION,
        GBUFFER_TEXTURE_NORMAL,
        GBUFFER_TEXTURE_ALBEDO,	
        GBUFFER_TEXTURE_NUM
    };
    SSAO* m_ssao = new SSAO(SCR_WIDTH, SCR_HEIGHT, GBUFFER_TEXTURE_NUM);
    SkyDome *skydome = new SkyDome();
    skydome->clouds1Map = loadTexture("../res/textures/physky/clouds1.png");
    skydome->clouds2Map = loadTexture("../res/textures/physky/clouds2.png");
    skydome->tint1Map = loadTexture("../res/textures/physky/tint1.png");
    skydome->tint2Map = loadTexture("../res/textures/physky/tint2.png");
    skydome->moonMap = loadTexture("../res/textures/physky/moon.png");
    skydome->sunMap = loadTexture("../res/textures/physky/sun.png");
    skydome->noisetexMap = loadTexture("../res/textures/noisetex.png");


    screenShader.use();
    screenShader.setInt("hdrBuffer", 0);
    screenShader.setInt("bloomBlur", 1);
    blurShader.use();
    blurShader.setInt("image", 0);

    MainShader.use();  
    MainShader.setInt("gPosition", 0);
    MainShader.setInt("gNormal", 1);
    MainShader.setInt("gAlbedo", 2);
    MainShader.setInt("ssao", 3);
    MainShader.setInt("shadowMap", 4);
    
    SSAOshader.use();
    SSAOshader.setInt("gPosition", 0);
    SSAOshader.setInt("gNormal", 1);
    SSAOshader.setInt("texNoise", 2);
    SSAOBlurShader.use();
    SSAOBlurShader.setInt("ssaoInput", 0);

    SkyDomeShader.use();
    SkyDomeShader.setInt("clouds1", 0);
    SkyDomeShader.setInt("clouds2", 1);
    SkyDomeShader.setInt("tint", 2);
    SkyDomeShader.setInt("tint2", 3);
    SkyDomeShader.setInt("moon", 4);
    SkyDomeShader.setInt("sun", 5);
    SkyDomeShader.setInt("noisetex", 6);

    GLuint hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    GLuint colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (GLuint i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
        );
    }
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint pingpongFBO[2];
    GLuint pingpongBuffer[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (GLuint i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);
    }

    //--------------------------------------
    // light info
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 SunColor(1.0f, 0.9f, 0.6f);
    glm::vec3 SunsetColor(1.0f, 0.7f, 0.5f);
    glm::vec3 DirColor = SunColor * glm::vec3(8.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);


    //----------------------------------------------------------------------
    Shader OceanShader("../shader/vs_ocean.glsl", "../shader/fs_ocean.glsl");

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    std::cout << glGetError() << std::endl; // 返回 0 (无错误)

    glBindVertexArray(VertexArrayID);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)

    Ocean oceanObj(128, 128, 40, 40, 3, 3);
    Object oceanObjBuffer;
    oceanObjBuffer.SetVertex(oceanObj.GetVertices());
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    oceanObjBuffer.SetNormal(oceanObj.GetNormals());
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    oceanObjBuffer.SetIndices(oceanObj.GetIndices());
std::cout << glGetError() << std::endl; // 返回 0 (无错误)

    GLuint mvp_uniform_block;
    glGenBuffers(1, &mvp_uniform_block);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    glBindBuffer(GL_UNIFORM_BUFFER, mvp_uniform_block);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, NULL, GL_STATIC_DRAW);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mvp_uniform_block);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)

    std::vector<glm::vec3> instance_offset_vec3 = oceanObj.GetInstance_offset();

    GLuint instance_offsetID = glGetUniformLocation(OceanShader.ID, "instance_offset");
std::cout << glGetError() << std::endl; // 返回 0 (无错误)

    // h0
    GLuint h0_uniform = glGetUniformLocation(OceanShader.ID, "h0");
    GLuint h0_data_buffer;
    glGenBuffers(1, &h0_data_buffer);
std::cout << glGetError() << std::endl; // 返回 0 (无错误)
    glBindBuffer(GL_TEXTURE_BUFFER, h0_data_buffer);
    glBufferData(GL_TEXTURE_BUFFER, oceanObj.vVar.h0.size() * sizeof(float), &oceanObj.vVar.h0[0], GL_STATIC_DRAW);

    GLuint h0_texID;
    glGenTextures(1, &h0_texID);
    glBindTexture(GL_TEXTURE_BUFFER, h0_texID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, h0_data_buffer);

    // h0Conj
    GLuint h0Conj_uniform = glGetUniformLocation(OceanShader.ID, "h0Conj");
    GLuint h0Conj_data_buffer;
    glGenBuffers(1, &h0Conj_data_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, h0Conj_data_buffer);
    glBufferData(GL_TEXTURE_BUFFER, oceanObj.vVar.h0Conj.size() * sizeof(float), &oceanObj.vVar.h0Conj[0], GL_STATIC_DRAW);

    GLuint h0Conj_texID;
    glGenTextures(1, &h0Conj_texID);
    glBindTexture(GL_TEXTURE_BUFFER, h0Conj_texID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, h0Conj_data_buffer);

    // dispersion
    GLuint dispersion_uniform = glGetUniformLocation(OceanShader.ID, "dispersion");
    GLuint dispersion_data_buffer;
    glGenBuffers(1, &dispersion_data_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, dispersion_data_buffer);
    glBufferData(GL_TEXTURE_BUFFER, oceanObj.vVar.dispersion.size() * sizeof(float), &oceanObj.vVar.dispersion[0], GL_STATIC_DRAW);

    GLuint dispersion_texID;
    glGenTextures(1, &dispersion_texID);
    glBindTexture(GL_TEXTURE_BUFFER, dispersion_texID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, dispersion_data_buffer);

    //------------------------------------------------------------------------



    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glEnable(GL_DEPTH_TEST);
        
        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::vec3 DirLightPos = skydome->getSunPos() * glm::vec3(0.5f);
        //
        // if (DirLightPos.y >= 0) {
        //     DirColor = SunColor * glm::vec3(7.5f * min(15.0f, DirLightPos.y) / 15.0) +
        //                SunsetColor * glm::vec3(2.5f * (15.0f - min(15.0f, DirLightPos.y)) / 15.0);
        // } else {
        //     DirLightPos = -DirLightPos;
        //     DirColor = lightColor * glm::vec3(2.0f * min(15.0f, DirLightPos.y) / 15.0) +
        //                lightColor * glm::vec3(1.0f * (15.0f - min(15.0f, DirLightPos.y)) / 15.0);
        // }
        //
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);
        // // GLfloat aspect = (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT;
        // GLfloat near_plane = 0.1f, far_plane = 200.0f;
        // glm::mat4 lightProjection, lightView;
        // glm::mat4 lightSpaceMatrix;
        // lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, 100.0f);
        // lightView = glm::lookAt(DirLightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        // lightSpaceMatrix = lightProjection * lightView;
        // shadowShader.use();
        // shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        //
        // glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        // dirshadow.Bind();
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //
        // glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        // //model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0, 0.0, 0.0));
        // model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        // shadowShader.setMat4("model", model);
        // ourModel.Draw(shadowShader);
        //
        // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        // glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, near_plane, far_plane);
        // glm::mat4 view = camera.GetViewMatrix();
        //
        // // 2. geometry pass: render scene's geometry/color data into gbuffer
        // // -----------------------------------------------------------------
        // glDisable(GL_CULL_FACE);
        // m_ssao->BindGbuffer();
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // shaderGeometryPass.use();
        // shaderGeometryPass.setMat4("model", model);
        // shaderGeometryPass.setMat4("projection", projection);
        // shaderGeometryPass.setMat4("view", view);
        // shaderGeometryPass.setBool("invertedNormals", false);
        // ourModel.Draw(shaderGeometryPass);
        //
        // // 3. generate SSAO texture
        // // ------------------------
        // m_ssao->BindSSAOFBO();
        // glClear(GL_COLOR_BUFFER_BIT);
        // SSAOshader.use();
        // m_ssao->ActivateSSAOTexture(SSAOshader);
        // SSAOshader.setMat4("projection", projection);
        // view = camera.GetViewMatrix();
        // SSAOshader.setMat4("view", view);
        // RenderQuad();
        //
        // // 4. blur SSAO texture to remove noise
        // // ------------------------------------
        // m_ssao->BindSSAOBlurFBO();
        // glClear(GL_COLOR_BUFFER_BIT);
        // SSAOBlurShader.use();
        // m_ssao->ActivateSSAOBlurTexture(SSAOBlurShader);
        // RenderQuad();
        //
        // // 5. PBR main render using GBuffer and SSAO
        // // ------------------------------------
        // glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // MainShader.use();
        // GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        // glDrawBuffers(2, attachments);
        //
        // MainShader.setVec3("dirLight.position", DirLightPos);
        // MainShader.setVec3("dirLight.color", DirColor);
        // MainShader.setVec3("viewPos", camera.Position);
        // MainShader.setFloat("far_plane", far_plane);
        // MainShader.setFloat("near_plane", near_plane);
        // MainShader.setFloat("metal", 0.0);
        // MainShader.setFloat("rough", 0.5);
        // MainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        // MainShader.setMat4("projection", projection);
        // MainShader.setMat4("view", view);
        //
        // m_ssao->ActivateTextureForLight();
        // glActiveTexture(GL_TEXTURE4);
        // glBindTexture(GL_TEXTURE_2D, dirshadow.depthMap);
        //
        // RenderQuad();
        //
        //
        // // 5.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // // ----------------------------------------------------------------------------------
        // m_ssao->BindGBufferForReading();
        // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFBO);
        // glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        // glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        //
        //
        // // 6. render skydome
        // // ----------------------------------------------------------------------------------
        // //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // skydome->setCameraPos(camera.Position);
        // skydome->drawSkyDome(SkyDomeShader, projection, view);
        //
    //
        // // 7. final screen render: bloom and hdr (and fxaa)
        // // ----------------------------------------------------------------------------------
        // GLboolean horizontal = true, first_iteration = true;
        // GLuint amount = 10;
        // blurShader.use();
        // glActiveTexture(GL_TEXTURE0);
        // for (GLuint i = 0; i < amount; i++)
        // {
        //     glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        //     blurShader.setInt("horizontal", horizontal);
        //     glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffer[!horizontal]);
        //     RenderQuad();
        //     horizontal = !horizontal;
        //     if (first_iteration)
        //         first_iteration = false;
        // }
        //
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
        // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //
        // screenShader.use();
        // screenShader.setFloat("exposure", exposure);
        // screenShader.setVec2("texelStep", glm::vec2(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT));
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        // glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, pingpongBuffer[1]);
        // RenderQuad();

        // 8. ocean
        OceanShader.use();

        OceanShader.setFloat("time", (float)glfwGetTime());
        OceanShader.setInt("N", oceanObj.GetWidth_X());
        OceanShader.setInt("M", oceanObj.GetWidth_Z());
        OceanShader.setFloat("LengthX", oceanObj.LengthX);
        OceanShader.setFloat("LengthZ", oceanObj.LengthZ);
        OceanShader.setVec3("DirectionalLight_direction_worldspace",-1,-1,-1);
        // h_0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, h0_texID);
        glUniform1i(h0_uniform, 0);

        // h_0*
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, h0Conj_texID);
        glUniform1i(h0Conj_uniform, 1);

        // dispersion
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, dispersion_texID);
        glUniform1i(dispersion_uniform, 2);

        OceanShader.setVec3("LightPosition_worldspace", DirLightPos); //光源位置
        OceanShader.setVec3("EyePosition", camera.Position);       //视角（摄像机的位置）
        glUniform3fv(instance_offsetID, instance_offset_vec3.size(), &instance_offset_vec3[0].x);

        glBindBuffer(GL_UNIFORM_BUFFER, mvp_uniform_block);
        SendUniformMVP();

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, oceanObjBuffer.vertices_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oceanObjBuffer.indices_buffer);
        glDrawElementsInstanced(GL_TRIANGLES, oceanObj.GetIndices().size(), GL_UNSIGNED_INT, (void *)0, instance_offset_vec3.size());



        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // float cameraSpeed = 2.5 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        exposure -= 0.1;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        exposure += 0.1;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}


GLuint quadVAO = 0;
GLuint quadVBO;
void RenderQuad()
{
    if (quadVAO == 0)
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // Setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

GLuint skyboxVAO = 0;
GLuint skyboxVBO;
void RenderSkybox()
{
    if (skyboxVAO == 0)
    {
        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

unsigned int indexCount = 0;
GLuint SphereVAO = 0;
GLuint SphereVBO;
GLuint SphereEBO;
void RenderSphere()
{
    if (SphereVAO == 0)
    {
        glGenVertexArrays(1, &SphereVAO);
        glGenBuffers(1, &SphereVBO);
        glGenBuffers(1, &SphereEBO);
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        //绘制球
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
                float yPos = cos(ySegment * PI);
                float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, -zPos));//这两个必须用-z，否则光照效果出错
                normals.push_back(glm::vec3(xPos, yPos, -zPos));//这两个必须用-z，否则光照效果出错
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; y++)
        {
            if (!oddRow)
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; x++)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }

        glBindVertexArray(SphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, SphereVBO);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);//Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));//法向量
        glEnableVertexAttribArray(1);
    }
    glBindVertexArray(SphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

glm::mat4 getProjectionMatrix()
{
    //获取投影矩阵
    glm::mat4 ProjectionMatrix = glm::perspective(
        glm::radians((float)45), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
    return ProjectionMatrix;
}

void SendUniformMVP()
{

    glm::mat4 projection = getProjectionMatrix();
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1);

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &model[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::mat4), &projection[0][0]);
}
