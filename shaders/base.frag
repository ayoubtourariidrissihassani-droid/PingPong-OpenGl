#version 330 core

in vec3 Normal;
out vec4 FragColor;

void main()
{
    vec3 normal = normalize(Normal);

    vec3 lightDir = normalize(vec3(-0.5, 1.0, 0.7));

    float diffuse = max(dot(normal, lightDir), 0.0);

    float ambient = 0.45;

    float intensity = ambient + diffuse * 0.45;

    vec3 gray = vec3(0.65) * intensity;

    FragColor = vec4(gray, 1.0);
}