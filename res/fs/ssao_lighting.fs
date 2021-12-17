#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
uniform sampler2D shadowMap;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
uniform Light light;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 invViewMatrix;    // view 矩阵的逆矩阵
uniform float zFar;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal, vec3 worldPos)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(light.Position - worldPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

const float NEAR = 0.1;     // 投影矩阵的近平面
const float FAR = 100.0f;   // 投影矩阵的远平面

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));    
}

bool IsInShadow(vec3 worldPos) {    
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos.xyz, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // float currentDepth = LinearizeDepth(projCoords.z);

    return currentDepth > closestDepth;
}

// 判断是否在视线内，用于体积光
// 目前需要考虑因为Gbuffer的存在是否必要
bool IsInView(vec3 worldPos) {
    vec4 fragPosViewSpace = projection * view * vec4(worldPos.xyz, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosViewSpace.xyz / fragPosViewSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(gPositionDepth, projCoords.xy).a;
    // get depth of current fragment from light's perspective
    float currentDepth = LinearizeDepth(projCoords.z);

    return currentDepth < closestDepth;
}

void main()
{             
    // retrieve data from gbuffer
    // 注意FragPos是相机坐标系的位置，需要先还原为世界坐标
    vec3 viewFragPos = texture(gPositionDepth, TexCoords).xyz;
    vec4 worldFragPos = invViewMatrix * vec4(viewFragPos, 1.0);

    vec3 FragPos = worldFragPos.xyz;
    // vec3 FragPos = texture(gPosition, TexCoords).xyz;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    
    // then calculate lighting as usual
    vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
    vec3 lighting  = ambient; 
    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0)
    // diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = light.Color * spec * Specular;
    // attenuation
    // float distance = length(light.Position - FragPos);
    // float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    // diffuse *= attenuation;
    // specular *= attenuation;

    // calculate shadow
    vec4 FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(FragPosLightSpace, Normal, FragPos);                      
    
    // calculate volume light
    float I = 0.0f;
    {
        // volume light
        const int n_steps = 150;
        vec3 startPos = viewPos;
        vec3 endPos = FragPos;
        vec3 ray = endPos - startPos;
        float rayLen = length(ray);
        vec3 rayDir = normalize(ray);
        float stepLen = rayLen / n_steps;
        vec3 step = rayDir * stepLen;
        vec3 pos = startPos;

        for (int i = 0; i < n_steps; i++) {
            if (!IsInShadow(pos)) {
            // if (IsInView(pos)) {
            // if (IsInView(pos) && !IsInShadow(pos)) {
            // if (tmpshadow >= 0.0f) {
                vec3 lightDir = normalize(pos - light.Position);
                vec3 viewDir = normalize(pos - viewPos);

                // Mie散射
                float cosTheta = dot(lightDir, normalize(-light.Position));
                float g = 0.0f;
                float hg = 1.0f/(4.0f*3.14f)* (1.0f - g*g)/ pow(1.0f + g * g - 2.0f * g * dot(lightDir,-viewDir), 1.5f);
                
                if (cosTheta > 0.9) {
                    I += clamp(10 * hg / n_steps, 0.0f, 1.0f);
                }
            }
            pos += step;
        }
        I = clamp(I, 0.0f, 1.0f);
    }
    vec3 yellow_light = vec3(1,198.0/255.0,107.0/255.0);
    vec3 volume = light.Color * I;
    lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * Diffuse + volume;    
    // lighting = volume;
    // lighting += diffuse + specular;

    FragColor = vec4(lighting, 1.0);
}
