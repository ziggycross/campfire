#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/strutil.h>

#include "shader.h"
#include "model.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

class Renderer {
private:
    Shader& shader;
    Model& model;
    GLuint FBO;
    unsigned int RENDER_SIZE_X;
    unsigned int RENDER_SIZE_Y;

public:
    Renderer(Shader& shader, Model& model, GLuint FBO, std::string filename, unsigned int RENDER_SIZE_X, unsigned int RENDER_SIZE_Y)
        : shader(shader), model(model), FBO(FBO), RENDER_SIZE_X(RENDER_SIZE_X), RENDER_SIZE_Y(RENDER_SIZE_Y) {}

    void renderSpin(const int numFrames, const std::string filename) {
        
        // Find the position of the last dot in the filename
        size_t dotPos = filename.find_last_of(".");
        // Split the filename into the name and extension
        std::string name = filename.substr(0, dotPos);
        std::string extension = filename.substr(dotPos);

        // Calculate the rotation angle for each frame
        float rotationAngle = 360.0f / numFrames;

        for (int i = 0; i < numFrames; i++) {
            // Create a new stringstream
            std::stringstream ss;
            // Add the name, a four-digit counter, and the extension to the stringstream
            ss << name << std::setfill('0') << std::setw(4) << i << extension;
            // Get the new filename from the stringstream
            std::string newFilename = ss.str();

            // Rotate the model
            glm::mat4 scene = glm::mat4(1.0f);
            scene = glm::rotate(scene, glm::radians(i * rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));

            // Render commands
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glViewport(0, 0, RENDER_SIZE_X, RENDER_SIZE_Y);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            shader.use();

            // Send transforms to shader
            int modelLoc = glGetUniformLocation(shader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(scene));

            model.Draw(shader);

            // Call the writeFrame function with the new filename
            writeFrame(newFilename);
        }
    }

    void writeFrame(const std::string& filename) {
        // Create array to hold pixel data
        unsigned char pixels[RENDER_SIZE_X*RENDER_SIZE_Y*4]; // 4 channels for RGBA
        
        // Bind FBO and read to array
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glReadPixels(0, 0, RENDER_SIZE_X, RENDER_SIZE_Y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
        // Calculate scanline size
        int scanlinesize = RENDER_SIZE_X*4*sizeof(char);

        // Write to file using OIIO
        OIIO::ImageSpec spec(RENDER_SIZE_X, RENDER_SIZE_Y, 4, OIIO::TypeDesc::UINT8);
        OIIO::ImageBuf buf(spec, (char *)pixels+(RENDER_SIZE_Y -1)*scanlinesize, OIIO::AutoStride, -scanlinesize, OIIO::AutoStride);
        buf.write(filename);
    }
};

#endif