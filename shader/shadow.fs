#version 430 core

uniform sampler2D texture_diffuse1;

in vec2 TexCoords;

void main()
{
	float alpha = texture(texture_diffuse1, TexCoords).a;
    if(alpha < 0.5){
        discard;
    }
}