#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 customColor;
uniform bool useCustomColor;

void main()
{
    if (useCustomColor) {
        FragColor = vec4(customColor, 1.0);
    } else {
        FragColor = texture(texture1, TexCoord);
    }
}