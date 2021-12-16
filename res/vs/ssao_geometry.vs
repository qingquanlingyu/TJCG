#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec4 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform bool invertedNormals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // vec4 viewPos = view * model * vec4(aPos, 1.0);
    // // 注意此处传入相机坐标系位置
    // FragPos = viewPos.xyz; 
    vec4 worldPos = view * model * vec4(aPos, 1.0);
    FragPos = worldPos; 

    TexCoords = aTexCoords;
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * (invertedNormals ? -aNormal : aNormal);
    
    // gl_Position = projection * viewPos;
    gl_Position = projection * view * worldPos;
}
