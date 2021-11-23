#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;

uniform float exposure;


void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += 0.1 * bloomColor;//ȫ�ַ��⣬��������Ҳ��Ч�����ǰ���ϵ��������߸�Ч������ߺ����֡����Ρ���
	
	bloomColor -= hdrColor; 
    bloomColor = max(bloomColor, 0.0);
    hdrColor += 0.8 * bloomColor;//�������⣬ֻ�Ժ����ĵط���Ч�����ǰ���ϵ��������߸�Ч��

    // reinhard
    vec3 result = hdrColor / (hdrColor + vec3(1.0));
    // exposure
    //vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0f);
} 