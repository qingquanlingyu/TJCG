#version 430 core
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;
uniform sampler2D shadowMap;

uniform vec3 viewPos;
uniform float far_plane;
uniform float near_plane;
uniform mat4 lightSpaceMatrix;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

// material parameters
uniform float metal;
uniform float rough;

// lights
struct DirLight {
    vec3 position;
    vec3 color;
};  
uniform DirLight dirLight;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, float nDotV, float nDotL, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

float calcDirLightShadows(vec3 FragPos, vec3 Normal, vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
	float shadow = 0.0;
	if(projCoords.z > 1.0)
        shadow = 0.0;
	else
    {
		float closestDepth = texture(shadowMap, projCoords.xy).r; 
		float currentDepth = projCoords.z; 
		vec3 normal = normalize(Normal);
		vec3 lightDir = normalize(dirLight.position - FragPos);
		float bias = max(0.1 * (1.0 - dot(normal, lightDir)), 0.01);
		vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
		for(int x = -2; x <= 2; ++x)
		{
			for(int y = -2; y <= 2; ++y)
			{
				float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
				shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
			}    
		}
		shadow /= 25.0;
	}
       
    return shadow;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float metallic, float roughness, float shadow, vec3 F0)
{
    vec3 lightDir = normalize(light.position);
    vec3 halfway  = normalize(lightDir + viewDir);
    vec3 radianceIn = light.color;
	float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    //Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfway, roughness);
    float G   = GeometrySmith(normal, nDotV, nDotL, roughness);
    vec3  F   = fresnelSchlick(max(dot(halfway,viewDir), 0.0), F0);

    //Finding specular and diffuse component
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max (denominator, 0.0001);

    vec3 radiance = (kD * (albedo / PI) + specular ) * radianceIn * nDotL;
    radiance *= (1.0 - shadow);

    return radiance;
}

bool IsInShadow(vec3 worldPos);
float VolumeCalculation(vec3 worldPos);

void main()
{      
    vec3 FragPos = texture(gPosition, TexCoords).rgb;

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
	float AmbientOcclusion = texture(ssao, TexCoords).r;

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    float Viewlength = length(viewPos - FragPos);
	vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
	
	float Dirshadow = calcDirLightShadows(FragPos, Normal, fragPosLightSpace);        
    Dirshadow = min(Dirshadow, 0.75);
	
	vec3 radianceOut = CalcDirLight(dirLight, N, V, albedo, metal, rough, Dirshadow, F0);

    vec3 ambient = vec3(0.03) * albedo * AmbientOcclusion;
    radianceOut += ambient; 

    // volume light
	float volumeCoe = 0.1;
    vec3 volume = dirLight.color * volumeCoe * VolumeCalculation(FragPos);
    radianceOut += volume;
    // radianceOut = volume;

    FragColor = vec4(radianceOut, Viewlength);
	
	//float brightness = dot(FragColor.rgb, vec3(0.3333, 0.3334, 0.3333));
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else 
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
} 

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, float nDotV, float nDotL, float roughness)
{
    float ggx2  = GeometrySchlickGGX(nDotV, roughness);
    float ggx1  = GeometrySchlickGGX(nDotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 


bool IsInShadow(vec3 worldPos) {    
	if (worldPos.y>20.0)
		return true;
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

float VolumeCalculation(vec3 worldPos)
{
    float I = 0.0f;
    // volume light
    const int n_steps = 80;
    vec3 startPos = viewPos;
    vec3 endPos = worldPos;
    vec3 ray = endPos - startPos;
    float rayLen = length(ray);
    vec3 rayDir = normalize(ray);
    float stepLen = rayLen / n_steps;
    vec3 step = rayDir * stepLen;
    vec3 pos = startPos;

    for (int i = 0; i < n_steps; i++) {
        if (!IsInShadow(pos)) {
            vec3 lightDir = normalize(pos - dirLight.position);
            vec3 viewDir = normalize(pos - viewPos);

            // Mie??????
            float cosTheta = dot(lightDir, normalize(-dirLight.position));
            float g = 0.9f;
            float hg = 1.0f/(4.0f*3.14f)* (1.0f - g*g)/ pow(1.0f + g * g - 2.0f * g * dot(lightDir,-viewDir), 1.5f);
            
            if (cosTheta > 0.9) {
                I += clamp(10 * hg / n_steps, 0.0f, 1.0f);
            }
        }
        pos += step;
    }
    I = clamp(I, 0.0f, 1.0f);
    return I;
}