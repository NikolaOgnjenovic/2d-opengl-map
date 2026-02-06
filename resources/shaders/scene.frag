#version 460 core
out vec4 FragColor;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform sampler2D uDiffMap1;
uniform bool useColor;
uniform vec3 color;
uniform bool isEmissive;

#define MAX_POINT_LIGHTS 20
uniform int nrPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform vec3 globalLightPos;
uniform vec3 globalLightColor;
uniform float globalLightIntensity;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse 
    float diff = max(dot(normal, lightDir), 0.0);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));    
    
    vec3 result = light.color * diff * light.intensity * attenuation;
    return result;
}

void main()
{    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    // Globalno svetlo
    vec3 lightDir = normalize(globalLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    result += globalLightColor * diff * globalLightIntensity;
    
    // Tačkasta svetla (od pinova)
    for(int i = 0; i < nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
    
    vec4 texColor;
    if(useColor) {
        texColor = vec4(color, 1.0);
    } else {
        texColor = texture(uDiffMap1, TexCoords);
    }

    if(isEmissive) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(result, 1.0) * texColor;
    }
}
