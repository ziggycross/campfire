#version 330 core
out vec4 FragColor;
in vec2 texCoord;
uniform sampler2D screenTexture;
void main()
{
    vec4 color = texture(screenTexture, texCoord);
    FragColor = vec4(color.rgb, 1.0);
}