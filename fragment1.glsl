#version 330 core
out vec4 FragColor;
in vec3 vertexColor;
in vec2 texCoord;
uniform sampler2D imageTexture;
void main()
{
    FragColor = texture(imageTexture, texCoord);
}