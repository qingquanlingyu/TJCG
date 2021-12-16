#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
	float alpha = texture(texture_diffuse1, TexCoords).a;
    if(alpha < 0.5){
        discard;
    }
    gPosition.rgb = FragPos;
    // also store the per-fragment normals into the gbuffer
    vec3 normal;
    normal = texture(texture_normal1, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN*normal);
    gNormal = normal;
    // // and the diffuse per-fragment color
    //gAlbedo.rgb = vec3(0.95);
    //gAlbedo.a = 1.0;
    // and the diffuse per-fragment color
    gAlbedo.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
}