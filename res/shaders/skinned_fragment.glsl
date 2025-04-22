#version 410 core

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 LocalPos0;
flat in ivec4 BoneIDs0;
in vec4 Weights0;

out vec4 FragColor;

struct BaseLight {
    vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct Material {
    vec3 AmbientColor;
    vec3 DiffuseColor;
    vec3 SpecularColor;
};

uniform Material gMaterial;
uniform sampler2D gSampler;
uniform sampler2D gSamplerSpecularExponent;
uniform vec3 gCameraLocalPos;
uniform vec3 dir;


vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, vec3 Normal) {
    vec4 AmbientColor = vec4(Light.Color, 1.0f) * Light.AmbientIntensity * vec4(gMaterial.AmbientColor, 1.0f);

    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        DiffuseColor = vec4(Light.Color, 1.0f) * Light.DiffuseIntensity * vec4(gMaterial.DiffuseColor, 1.0f) * DiffuseFactor;

        vec3 PixelToCamera = normalize(gCameraLocalPos - LocalPos0);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(PixelToCamera, LightReflect);
        if (SpecularFactor > 0) {
            float SpecularExponent = texture(gSamplerSpecularExponent, TexCoord0).r * 255.0;
            SpecularFactor = pow(SpecularFactor, SpecularExponent);
            SpecularColor = vec4(Light.Color, 1.0f) * Light.DiffuseIntensity * vec4(gMaterial.SpecularColor, 1.0f) * SpecularFactor;
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

void main() {
    vec3 Normal = normalize(Normal0);

    BaseLight light;
    light.Color = gMaterial.AmbientColor;
    light.AmbientIntensity = 1.0f;
    light.DiffuseIntensity = 1.0f;
    vec4 TotalLight = CalcLightInternal(light, -dir, Normal);
    FragColor = vec4(texture(gSampler, TexCoord0.xy).xyz, 1.0)  * 1.0;
}