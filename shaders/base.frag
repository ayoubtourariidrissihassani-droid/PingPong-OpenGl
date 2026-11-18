#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// material
uniform vec3 uColor;
uniform bool uEmissive;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// camera
uniform vec3 viewPos;

// spot light
uniform vec3  lightPos;
uniform vec3  lightDir;
uniform vec3  lightColor;
uniform float lightInnerCos;
uniform float lightOuterCos;
uniform float lightConstant;
uniform float lightLinear;
uniform float lightQuadratic;

// base lighting level (lower = darker scene)
const float BASE_AMBIENT = 0.22;
const float BASE_DIFFUSE = 0.28;

void main()
{
    // bulb: always bright
    if (uEmissive)
    {
        FragColor = vec4(lightColor, 1.0);
        return;
    }

    // surface color: texture or plain color
    vec3 albedo = uColor;
    if (uUseTexture)
        albedo = texture(uTexture, TexCoord).rgb;

    vec3 norm = normalize(Normal);

    // ---- base lighting (slightly dark) ----
    vec3 baseDir = normalize(vec3(-0.5, 1.0, 0.7));
    float baseDiffuse = max(dot(norm, baseDir), 0.0);
    float intensity = BASE_AMBIENT + baseDiffuse * BASE_DIFFUSE;
    vec3 base = albedo * intensity;

    // ---- spot light ----
    vec3 toLight = lightPos - FragPos;
    float dist = length(toLight);
    vec3 L = toLight / dist;

    // cone with smooth edge
    float theta = dot(L, normalize(-lightDir));
    float epsilon = lightInnerCos - lightOuterCos;
    float spot = clamp((theta - lightOuterCos) / epsilon, 0.0, 1.0);

    // attenuation
    float attenuation = 1.0 / (lightConstant + lightLinear * dist + lightQuadratic * dist * dist);

    // diffuse
    float diff = max(dot(norm, L), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    // specular (Blinn-Phong)
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(norm, H), 0.0), 32.0);
    vec3 specular = 0.25 * spec * lightColor;

    vec3 spotResult = (diffuse + specular) * spot * attenuation;

    FragColor = vec4(base + spotResult, 1.0);
}