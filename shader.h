#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    // Program ID
    unsigned int ID;

    Shader(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vertexShaderFile;
        std::ifstream fragmentShaderFile;

        vertexShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fragmentShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // Open the shader files
            vertexShaderFile.open(vertexShaderFilePath);
            fragmentShaderFile.open(fragmentShaderFilePath);
            // Read the files into streams
            std::stringstream vertexShaderStream, fragmentShaderStream;
            vertexShaderStream << vertexShaderFile.rdbuf();
            fragmentShaderStream << fragmentShaderFile.rdbuf();
            // Close the files
            vertexShaderFile.close();
            fragmentShaderFile.close();
            // Convert the streams into string
            vertexCode = vertexShaderStream.str();
            fragmentCode = fragmentShaderStream.str();
        }
        catch(std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char* vertexShaderCode = vertexCode.c_str();
        const char* fragmentShaderCode = fragmentCode.c_str();

        // COMPILE SHADERS
        int success;
        char infoLog[512];

        // Vertex Shader
        unsigned int vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
        glCompileShader(vertexShader);
        // -- CHECK SHADER COMPILED PROPERLY
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Fragment Shader
        unsigned int fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
        glCompileShader(fragmentShader);
        // -- CHECK SHADER COMPILED PROPERLY
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        // -- CHECK PROGRAM COMPILED PROPERLY
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // -- Delete shaders after compiling
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    };

    void use()
    {
        glUseProgram(ID);
    };

    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    };
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    };
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    };
};

#endif