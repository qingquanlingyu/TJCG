#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

float FXAA_THRESHOLD = 0.5;
float FXAA_REDUCE_MIN = 1.0/128.0;
float FXAA_REDUCE_MUL = 1.0/8.0;
float FXAA_SPAN_MAX = 8.0;
uniform vec2 texelStep; //1.0/ScreenWidth,1.0/ScreenHeight

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;

uniform float exposure;


void main()
{
    const float gamma = 2.2;
    vec3 hdrColor;
    vec3 hdrM = texture(hdrBuffer, TexCoords).rgb;
    vec3 hdrNW = textureOffset(hdrBuffer, TexCoords, ivec2(-1, 1)).rgb;
    vec3 hdrNE = textureOffset(hdrBuffer, TexCoords, ivec2(1, 1)).rgb;
    vec3 hdrSW = textureOffset(hdrBuffer, TexCoords, ivec2(-1, -1)).rgb;
    vec3 hdrSE = textureOffset(hdrBuffer, TexCoords, ivec2(1, -1)).rgb;

    const vec3 toLuma = vec3(0.299, 0.587, 0.114);
	
	// Convert from RGB to luma.
	float lumaNW = dot(hdrNW, toLuma);
	float lumaNE = dot(hdrNE, toLuma);
	float lumaSW = dot(hdrSW, toLuma);
	float lumaSE = dot(hdrSE, toLuma);
	float lumaM = dot(hdrM, toLuma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	if (lumaMax - lumaMin <= lumaMax * FXAA_THRESHOLD)
	{
		hdrColor = hdrM;
	}  
	else
    {
	    vec2 samplingDirection;	
	    samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
        samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
        float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * FXAA_REDUCE_MUL, FXAA_REDUCE_MIN);

	    float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);
        samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texelStep;
	
	    vec3 hdrSampleNeg = texture(hdrBuffer, TexCoords + samplingDirection * (1.0/3.0 - 0.5)).rgb;
	    vec3 hdrSamplePos = texture(hdrBuffer, TexCoords + samplingDirection * (2.0/3.0 - 0.5)).rgb;

	    vec3 hdrTwoTab = (hdrSamplePos + hdrSampleNeg) * 0.5;  

	    vec3 hdrSampleNegOuter = texture(hdrBuffer, TexCoords + samplingDirection * (0.0/3.0 - 0.5)).rgb;
	    vec3 hdrSamplePosOuter = texture(hdrBuffer, TexCoords + samplingDirection * (3.0/3.0 - 0.5)).rgb;
	
	    vec3 hdrFourTab = (hdrSamplePosOuter + hdrSampleNegOuter) * 0.25 + hdrTwoTab * 0.5;   
	
	    float lumaFourTab = dot(hdrFourTab, toLuma);
	
	    if (lumaFourTab < lumaMin || lumaFourTab > lumaMax)
	    {
		    hdrColor = hdrTwoTab; 
	    }
	    else
	    { 
		    hdrColor = hdrFourTab;
	    }
    }
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += 0.1 * bloomColor;//全局泛光，对天空球等也有效，提高前面的系数可以提高该效果，提高后会出现“光晕”感
	
	bloomColor -= hdrColor; 
    bloomColor = max(bloomColor, 0.0);
    hdrColor += 0.8 * bloomColor;//高亮泛光，只对很亮的地方有效，提高前面的系数可以提高该效果

    // reinhard
    vec3 result = hdrColor / (hdrColor + vec3(1.0));
    // exposure
    //vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0f);
} 
