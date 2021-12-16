#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shadow.h"
#include "camera.h"
#include "texture.h"
#include "model.h"
#include "ssao.hpp"
#include "SkyDome.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderQuad();
void RenderSkybox();
void RenderSphere();

// settings
#define NR_POINT_LIGHTS 4
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "v2.0", NULL, NULL);
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
    Shader skyboxShader("../shader/skybox.vs", "../shader/skybox.fs");
    // Shader skyboxShader("../xres/vs/6.2.skybox.vs", "../xres/fs/6.2.skybox.fs");
    Shader shadowShader("../shader/shadow.vs", "../shader/shadow.fs");
    Shader blurShader("../shader/blur.vs", "../shader/blur.fs");
    Shader lightShader("../shader/light.vs", "../shader/light.fs");
    Shader PointshadowShader("../shader/pointShadow.vs", "../shader/pointShadow.fs", "../shader/pointShadow.gs");
    Shader SkyDomeShader("../shader/SkyDomeShader.vs", "../shader/SkyDomeShader.fs");

    
    glm::vec3 pointLightPositions[] = {
        glm::vec3(10.0f,  10.0f,  10.0f),
        glm::vec3(10.0f,  10.0f,  -10.0f),
        glm::vec3(-10.0f,  10.0f,  10.0f),
        glm::vec3(-10.0f,  10.0f,  -10.0f)
    };
    
    // vector<std::string> faces
    // {
    //     "../xres/textures/skybox/right.jpg",
    //     "../xres/textures/skybox/left.jpg",
    //     "../xres/textures/skybox/top.jpg",
    //     "../xres/textures/skybox/bottom.jpg",
    //     "../xres/textures/skybox/front.jpg",
    //     "../xres/textures/skybox/back.jpg"
    // };
    //
    // unsigned int cubemapTexture = loadCubemap(faces);

    Model ourModel("../res/model/untitled.obj");

    DirShadow dirshadow;
    PointShadow pointshadow[NR_POINT_LIGHTS];
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

    glm::vec3 lightColor;
    lightColor.x = 1.0f;
    lightColor.y = 1.0f;
    lightColor.z = 1.0f;
    glm::vec3 SunColor;
    SunColor.x = 1.0f;
    SunColor.y = 0.9f;
    SunColor.z = 0.6f;
    glm::vec3 DirColor = SunColor * glm::vec3(8.0f);
    glm::vec3 PointColor = lightColor * glm::vec3(2.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glEnable(GL_DEPTH_TEST);
        
        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::vec3 DirLightPos;
        // float SunSpeed = 0.1;
        //DirLightPos.x = 5.0f;
        //DirLightPos.y = 50.0 * sin(SunSpeed * glfwGetTime());
        //DirLightPos.z = 50.0 * cos(SunSpeed * glfwGetTime());

        DirLightPos = skydome->getSunPos() * glm::vec3(0.5f);
        if (DirLightPos.y >= 0) {
            DirColor = SunColor * glm::vec3(8.0f * min(5.0f, DirLightPos.y) / 5.0);
        }        
        else
        {
            DirLightPos = -DirLightPos;
            DirColor = SunColor * glm::vec3(3.0f * min(5.0f, DirLightPos.y) / 5.0);
        }
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        // GLfloat aspect = (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT;
        GLfloat near_plane = 0.1f, far_plane = 200.0f;
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, 100.0f);
        lightView = glm::lookAt(DirLightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        dirshadow.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        shadowShader.setMat4("model", model);
        ourModel.Draw(shadowShader);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, near_plane, far_plane);
        glm::mat4 view = camera.GetViewMatrix();

        // 2. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glDisable(GL_CULL_FACE);
        m_ssao->BindGbuffer();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("model", model);
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        shaderGeometryPass.setBool("invertedNormals", false);
        ourModel.Draw(shaderGeometryPass);

        // 3. generate SSAO texture
       // ------------------------
        m_ssao->BindSSAOFBO();
        glClear(GL_COLOR_BUFFER_BIT);
        SSAOshader.use();
        m_ssao->ActivateSSAOTexture(SSAOshader);
        SSAOshader.setMat4("projection", projection);
        RenderQuad();

        // 4. blur SSAO texture to remove noise
        // ------------------------------------
        m_ssao->BindSSAOBlurFBO();
        glClear(GL_COLOR_BUFFER_BIT);
        SSAOBlurShader.use();
        m_ssao->ActivateSSAOBlurTexture(SSAOBlurShader);
        RenderQuad();

        // 5. PBR main render using GBuffer and SSAO
        // ------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        MainShader.use();
        GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        MainShader.setVec3("dirLight.position", DirLightPos);
        MainShader.setVec3("dirLight.color", DirColor);

        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            std::string s = "pointLights[";

            s.append(1, char(i + 48));
            s.append("].");
            MainShader.setVec3(s + std::string("position"), pointLightPositions[i]);
            MainShader.setVec3(s + std::string("color"), PointColor);
            MainShader.setFloat(s + std::string("constant"), 1.0f);
            MainShader.setFloat(s + std::string("linear"), 0.09);
            MainShader.setFloat(s + std::string("quadratic"), 0.032);
        }

        MainShader.setVec3("viewPos", camera.Position);
        MainShader.setFloat("far_plane", far_plane);
        MainShader.setFloat("near_plane", near_plane);
        MainShader.setFloat("metal", 0.0);
        MainShader.setFloat("rough", 0.5);
        MainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        m_ssao->ActivateTextureForLight();
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, dirshadow.depthMap);

        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            MainShader.setInt(std::string(std::string("pointshadowMap[") + std::to_string(i) + "]").c_str(), 5 + i);
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointshadow[i].depthCubemap);
        }

        RenderQuad();

        /*
        lightShader.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, DirLightPos);
        model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
        lightShader.setMat4("model", model);
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        RenderSphere();

        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        RenderSkybox();
        */

        // 5.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // ----------------------------------------------------------------------------------
        m_ssao->BindGBufferForReading();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFBO);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);


        // 6. render skydome
        // ----------------------------------------------------------------------------------
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        skydome->drawSkyDome(SkyDomeShader, projection, view);
        
    
        // 7. final screen render: bloom and hdr (and fxaa)
        // ----------------------------------------------------------------------------------
        GLboolean horizontal = true, first_iteration = true;
        GLuint amount = 10;
        blurShader.use();
        glActiveTexture(GL_TEXTURE0);
        for (GLuint i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffer[!horizontal]);
            RenderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        screenShader.use();
        screenShader.setFloat("exposure", exposure);
        screenShader.setVec2("texelStep", glm::vec2(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[1]);
        RenderQuad();


        // view = camera.GetViewMatrix();
        // projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        // glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        // skyboxShader.use();
        // view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        // skyboxShader.setMat4("view", view);
        // skyboxShader.setMat4("projection", projection);
        // // skybox cube
        // // glBindVertexArray(skyboxVAO);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        // RenderSkybox();
        // // glDrawArrays(GL_TRIANGLES, 0, 36);
        // // glBindVertexArray(0);
        // glDepthFunc(GL_LESS); // set depth function back to default
        
        
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

