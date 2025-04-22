#version 410 core

// Inputs from the vertex shader
in vec3 m_normal;
in vec4 m_vertex;
in vec2 m_texcoord;

// Textures
uniform sampler2D u_ambientTex;
uniform sampler2D u_diffuseTex;
uniform sampler2D u_specularTex;
uniform sampler2D u_specularHighTex;
uniform sampler2D u_bumpTex;
uniform sampler2D u_reflectionTex;
uniform sampler2D u_alphaTex;

// Constants
const int num_lights = 5;

// Outputs
out vec4 fragColor;

// Uniforms
uniform mat4 uModelView;
uniform vec4 light_posn[num_lights];  // Light positions (in eye space)
uniform vec4 light_col[num_lights];   // Light colors

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 transmittance;
uniform vec3 emission;
uniform float ior;
uniform float dissolve;
uniform float shininess;
uniform int illum;

// Compute Phong Lighting
vec4 compute_lighting(vec3 direction, vec4 lightcolor, vec3 normal, vec3 halfvec, vec4 mydiffuse, vec4 myspecular, float myshininess, float distance) {
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.02 * distance * distance); // Quadratic attenuation
    vec4 corrected_light = lightcolor.rgba * attenuation * 1000.0f; // Apply attenuation
    float n_dot_l = max(dot(normal, direction), 0.0);
    vec4 lambert = mydiffuse * corrected_light * n_dot_l;

    float n_dot_h = max(dot(normal, halfvec), 0.0);
    vec4 phong = myspecular * corrected_light * pow(n_dot_h, myshininess);

    return lambert + phong;
}

void main() {

    // Sample textures
    vec4 ambientColor = texture(u_ambientTex, m_texcoord);
    vec4 diffuseColor = texture(u_diffuseTex, m_texcoord);
    vec4 specularColor = texture(u_specularTex, m_texcoord);
    vec4 specularHighlight = texture(u_specularHighTex, m_texcoord);
    vec4 bumpMap = texture(u_bumpTex, m_texcoord);
    vec4 reflectionColor = texture(u_reflectionTex, m_texcoord);
    vec4 alphaColor = texture(u_alphaTex, m_texcoord);

    float ambient_light = 0.0;
    // Start with ambient color
    vec4 finalColor = vec4((ambient * ambientColor.xyz) * ambient_light, 1.0);

    // Normalize normal
    vec3 normal = normalize(m_normal + bumpMap.rgb * 2.0 - 1.0);

    // Eye position is at (0,0,0) in eye space
    vec3 mypos = m_vertex.xyz;  // No need for division by w
    vec3 eyedirn = normalize(-mypos);

    // Loop through light sources
    for (int i = 0; i < num_lights; i++) {
        vec3 position = light_posn[i].xyz;
        vec3 direction = normalize(position - mypos);
        float distance = length(position - mypos); // Calculate light distance
        vec3 half_i = normalize(direction + eyedirn);
        vec4 scaledSpecular = specularColor * specularHighlight.rgba;
        if (light_col[i].a > 0.001) {
            vec4 col = compute_lighting(direction, light_col[i], normal, half_i, diffuseColor, scaledSpecular, shininess, distance);
            finalColor += col;
        }
    }
//    finalColor += reflectionColor * 0.3; // Blending factor for reflections
    fragColor = finalColor;
}
