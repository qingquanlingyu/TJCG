#version 430 core

uniform vec3 LightPosition_worldspace;
uniform vec3 DirectionalLight_direction_worldspace;
layout(std140, binding = 0) uniform MVP
{
  mat4 model;
  mat4 view;
  mat4 projection;
};

in VS_OUT
{
  vec3 position_worldspace;
  vec3 normal_cameraspace;
  vec3 eyeDirection_cameraspace;
  vec3 lightDirection_cameraspace;
} fs_in;

out vec4 FragColor;
uniform vec3 LightColor;

float saturate(float inpu)
{
    if(inpu < 0)
        return 0;
    else if(inpu < 1)
        return inpu;
    else 
        return 1;
}
float SSScolor(vec3 lightDir,vec3 viewDir,vec3 normal,float Distortion,float waveHeight,float SSSMask)
{
    int power = 1;//直接半透明的功率值
    vec3 vLTLight = normalize (-lightDir + normal *Distortion);
    float I = pow(saturate(dot(viewDir,-vLTLight)),power)*SSSMask*waveHeight;

    return I*LightColor;
}


void main()
{
	vec3 color;

  vec3 MaterialDiffuseColor = vec3(0.3686, 0.7098, 0.7922);
  vec3 MaterialAmbientColor = 0.01 * LightColor * MaterialDiffuseColor;
  //vec3 MaterialSpecularColor = vec3(0.1, 0.0, 0.0);
vec3 MaterialSpecularColor = vec3(0.1,0.1,0.1);
  // Point Light
  float lightstrength = 0.05f;
  float LightPower = 500;

  float distance_to_light = length(LightPosition_worldspace - fs_in.position_worldspace);

  vec3 n = normalize(fs_in.normal_cameraspace); //法线

  vec3 l = normalize(fs_in.lightDirection_cameraspace);

  float cosTheta = clamp(dot(n,l), 0, 1);

  vec3 E = normalize(fs_in.eyeDirection_cameraspace);
  vec3 R = reflect(-l, n);

  float cosAlpha = clamp(dot(E, R), 0, 1);

  color = lightstrength * MaterialAmbientColor
    + lightstrength * MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, 32) / (distance_to_light * distance_to_light);

  // Directional Light
  vec3 DirectionalLight_direction_camspace = ( view * model *vec4(DirectionalLight_direction_worldspace, 0)).xyz;
  vec3 Di_Light_direction = normalize(-DirectionalLight_direction_camspace);
  float Di_Light_power = 1.8;

  float cosTheta_Di = clamp(dot(n, Di_Light_direction), 0, 1);

  color += lightstrength * MaterialDiffuseColor * LightColor * Di_Light_power * cosTheta_Di;//漫反射
  //SSS
  color += MaterialDiffuseColor * SSScolor(Di_Light_direction,E,n,1,saturate(fs_in.position_worldspace.y/32),30);
  
  FragColor = vec4(color, 1.0f);
        
}
