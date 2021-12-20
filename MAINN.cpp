#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <shader.h>
#include <camera.h>
#include <ocean.h>
#include <object.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void SendUniformMVP();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(10, 15, 0), glm::vec3(0.0f, 1.0f, 0.0f), -147.5, -3.2);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(0.1f, 0.3f, 0.1f);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Z-buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader OceanShader("resources/vs_ocean.glsl", "resources/fs_ocean.glsl");

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    Ocean oceanObj(128, 128, 60, 60, 3, 3);
    Object oceanObjBuffer;
    oceanObjBuffer.SetVertex(oceanObj.GetVertices());
    oceanObjBuffer.SetNormal(oceanObj.GetNormals());
    oceanObjBuffer.SetIndices(oceanObj.GetIndices());

    GLuint mvp_uniform_block;
    glGenBuffers(1, &mvp_uniform_block);
    glBindBuffer(GL_UNIFORM_BUFFER, mvp_uniform_block);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mvp_uniform_block);

    std::vector<glm::vec3> instance_offset_vec3 = oceanObj.GetInstance_offset();

    GLuint instance_offsetID = glGetUniformLocation(OceanShader.ID, "instance_offset");

    // h0
    GLuint h0_uniform = glGetUniformLocation(OceanShader.ID, "h0");
    GLuint h0_data_buffer;
    glGenBuffers(1, &h0_data_buffer);
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

    //设置时间归0
    glfwSetTime(0);
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        OceanShader.setVec3("LightPosition_worldspace", lightPos); //光源位置
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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}


glm::mat4 getProjectionMatrix()
{
    //获取投影矩阵
    glm::mat4 ProjectionMatrix = glm::perspective(
        glm::radians((float)45), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    return ProjectionMatrix;
}
glm::mat4 getViewMatrix()
{
    return camera.GetViewMatrix();
}
void SendUniformMVP()
{

    glm::mat4 projection = getProjectionMatrix();
    glm::mat4 view = getViewMatrix();
    glm::mat4 model = glm::mat4(1);

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &model[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::mat4), &projection[0][0]);
}
