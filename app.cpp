#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/strutil.h>

#include "shader.h"
#include "model.h"
#include "mesh.h"
#include "write.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <vector>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Camera initialisation
glm::vec3 cameraPos     = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront   = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp      = glm::vec3(0.0f, 1.0f,  0.0f);
float pitch = 0.0f, yaw = -90.0f;

// Mouse initialisation
float lastX = 400, lastY = 300;
bool firstMouse = true;
bool mouseActive = false;

// Frame timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Render dimensions
unsigned int RENDER_SIZE_X = 360, RENDER_SIZE_Y = 270;
unsigned int WINDOW_SIZE_X = 800, WINDOW_SIZE_Y = 600;

// Quad object that fills whole screen, for use with render textures
float fullscreenQuad[] = {
    // positions   // texCoords
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,

     1.0f,  1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};

// Main window
int main()
{   
    // Initialise GLFW and hint settings
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Enable this for Mac OS X compatibility
#endif

    // Create OpenGL window
    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE_X/2, WINDOW_SIZE_Y/2, "Campfire", NULL, NULL);
    if(window == NULL)
    {
        std::cout << "Failed to create a window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Attach callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Initialise GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Load shaders
    Shader shader1("shader.vert", "shader.frag");
    Shader screenShader("screenshader.vert", "screenshader.frag");

    // Load models
    Model testModel(filesystem::path("resources/models/mario-kart/F2_Item_Kart_Mario_S.dae"));

    // Create and bind frame buffer object
    unsigned int FBO;
    glGenFramebuffers(1, &FBO); // We will render to this and then use it as a texture for our fullscreen quad
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Create frame buffer texture and attach to FBO
    unsigned int framebufferTexture;
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RENDER_SIZE_X, RENDER_SIZE_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    // Create render buffer
    unsigned int RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, RENDER_SIZE_X, RENDER_SIZE_Y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER::" << fboStatus << std::endl;

    // Screen quad
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), &fullscreenQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // Set position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // Set UV coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    screenShader.use();
    glUniform1i(glGetUniformLocation(screenShader.ID, "screenTexture"), 0);

    // Set to wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Render loop
    while(!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Check deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Enable mouse
        if (!mouseActive)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Low-res buffer
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, RENDER_SIZE_X, RENDER_SIZE_Y);
        
        // Render commands
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        shader1.use();
        
        // Camera view
        glm::mat4 view;
        view = glm::lookAt(cameraPos,
                           cameraPos+cameraFront,
                           cameraUp);

        // Create transforms
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, -1.0f, 0.0f));
        projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_SIZE_X/(float)WINDOW_SIZE_Y, 0.1f, 100.0f);
        
        // Send transforms to shader
        int modelLoc = glGetUniformLocation(shader1.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        int viewLoc = glGetUniformLocation(shader1.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        int projectionLoc = glGetUniformLocation(shader1.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        testModel.Draw(shader1);
        // testModel.Draw(shader1);

        // Render framebuffer to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y);
        screenShader.use();
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Swap buffers and call events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    
    // Create buffer and read in FBO pixels
    unsigned char pixels[RENDER_SIZE_X*RENDER_SIZE_Y*4];
    glReadPixels(0,0,RENDER_SIZE_X, RENDER_SIZE_Y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    // Output using OIIO (needs to flip due to OpenGL's bottom up datatype)
    std::string filename = "output.png";
    OIIO::ImageSpec spec(RENDER_SIZE_X, RENDER_SIZE_Y, 4, TypeDesc::UINT8);
    int scanlinesize = RENDER_SIZE_X * 4 * sizeof(pixels[0]);
    unique_ptr<ImageOutput> out = ImageOutput::create(filename);
    out->open(filename, spec);
    out->write_image(TypeDesc::UINT8,
                     (char *)pixels+(RENDER_SIZE_Y-1)*scanlinesize,
                     AutoStride,
                     -scanlinesize,
                     AutoStride);
    out->close();

    // Clear GLFW and end program
    glfwTerminate();
    return 0;
}

// Setup window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    WINDOW_SIZE_X = width;
    WINDOW_SIZE_Y = height;
}

// Setup inputs
void processInput(GLFWwindow *window)
{
    // Close program
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Disable movement if mouse not active
    if (!mouseActive)
        return;

    // Movement
    const float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Forwards
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Backwards
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Right
        cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Left
        cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // Up
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Down
        cameraPos -= cameraSpeed * cameraUp;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        mouseActive = !mouseActive;
        firstMouse = true;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!mouseActive)
        return;
    
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    else if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}