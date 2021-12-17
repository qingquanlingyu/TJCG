#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;
uniform bool rmap;
uniform bool nmap;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
	float alpha = texture(texture_diffuse1, TexCoords).a;
    if(alpha < 0.5){
        discard;
    }
    gPosition.rgb = FragPos;
    // also store the per-fragment normals into the gbuffer
    vec3 normal1, normal2;
	normal1 = texture(texture_normal1, TexCoords).rgb;
	normal1 = normalize(normal1 * 2.0 - 1.0);
	normal1 = normalize(TBN*normal1);
	normal2 = normalize(Normal);
	if (nmap)
		gNormal = normalize(0.5*normal1+0.5*normal2);
	else
		gNormal = normal2;
    // // and the diffuse per-fragment color
    //gAlbedo.rgb = vec3(0.95);
    //gAlbedo.a = 1.0;
    // and the diffuse per-fragment color
    gAlbedo.rgb = texture(texture_diffuse1, TexCoords).rgb;
	if (rmap)
		gAlbedo.a = texture(texture_specular1, TexCoords).r;
	else
		gAlbedo.a = 0.6;
    // store specular intensity in gAlbedoSpec's alpha component
}