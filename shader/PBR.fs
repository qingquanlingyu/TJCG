#version 430 core
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;
uniform vec3 viewPos;
uniform float far_plane;
//in float Depth;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

#define SHININESS 32
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;

// material parameters
uniform float metal;
uniform float rough;

// lights
struct DirLight {
    vec3 position;
    vec3 color;
};  
uniform DirLight dirLight;

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 color;
};  
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform samplerCube pointshadowMap[NR_POINT_LIGHTS];

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, float nDotV, float nDotL, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

float calcDirLightShadows(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
	float closestDepth2 = texture(shadowMap, projCoords.xy).g; 
    float currentDepth = projCoords.z;
    //float shadow = (closestDepth > currentDepth) ? 1.0 : 0.0;
    
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(dirLight.position - fs_in.FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	
	float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -3; x <= 3; ++x)
    {
        for(int y = -3; y <= 3; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 49.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
       
    return shadow;
}

float calcPointLightShadows(samplerCube depthMap, vec3 lightp, float viewDistance)
{
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    vec3 fragToLight = fs_in.FragPos - lightp;
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;    
    float currentDepth = (length(fragToLight) - bias);
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
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

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos, vec3 albedo, float metallic, float roughness, float shadow, vec3 F0)
{
    vec3 lightDir = normalize(light.position);
    vec3 halfway  = normalize(lightDir + viewDir);
    vec3 radianceIn = light.color;
	float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);
	
	float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));  
	
	radianceIn *= attenuation;
	
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
    vec3 specular = numerator / max (denominator, 00.0000001);

    vec3 radiance = (kD * (albedo / PI) + specular ) * radianceIn * nDotL;
    radiance *= (1.0 - shadow);

    return radiance;
}

void main()
{      
	vec4 Tex = texture(texture_diffuse1, fs_in.TexCoords).rgba;
	vec3 albedo = Tex.rgb;
	
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(viewPos - fs_in.FragPos);
    float Viewlength = length(viewPos - fs_in.FragPos);
	vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
	
	float Dirshadow = calcDirLightShadows(fs_in.FragPosLightSpace);        
    Dirshadow = min(Dirshadow, 0.75);
	
	vec3 radianceOut = CalcDirLight(dirLight, N, V, albedo, metal, rough, Dirshadow, F0);
	for (int i = 0; i < NR_POINT_LIGHTS; i++)
	{
        float Pointshadow = calcPointLightShadows(pointshadowMap[i], pointLights[i].position, Viewlength);        
        Pointshadow = min(Pointshadow, 0.85);
        radianceOut += CalcPointLight(pointLights[i], N, V, fs_in.FragPos, albedo, metal, rough, Pointshadow, F0);
    }

    vec3 ambient = vec3(0.025) * albedo;
    radianceOut += ambient; 
	
    FragColor = vec4(radianceOut, Tex.a);
	
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