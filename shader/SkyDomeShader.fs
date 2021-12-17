#version 430 core
//---------IN------------
in vec3 pos;
in vec2 texCoord;
in vec3 star_pos;
//---------UNIFORM------------
uniform sampler2D tint;//the color of the sky on the half-sphere where the sun is. (time x height)
uniform sampler2D tint2;//the color of the sky on the opposite half-sphere. (time x height)
uniform sampler2D sun;//sun texture (radius x time)
uniform sampler2D moon;//moon texture (circular)
uniform sampler2D clouds1;//light clouds texture (spherical UV projection)
uniform sampler2D clouds2;//heavy clouds texture (spherical UV projection)
uniform float weather;//mixing factor (0.5 to 1.0)
uniform float time;

uniform vec3 sun_pos;               //sun position in world space
vec3 sun_norm;

uniform vec3 CameraPos;
uniform int frameCounter;
uniform sampler2D noisetex;

#define bottom 20   // 云层底部
#define top 25      // 云层顶部
#define width 100     // 云层 xz 坐标范围 [-width, width]

#define baseBright  vec3(1.26,1.25,1.29)    // 基础颜色 -- 亮部
#define baseDark    vec3(0.31,0.31,0.32)    // 基础颜色 -- 暗部
#define lightBright vec3(1.29, 1.17, 1.05)  // 光照颜色 -- 亮部
#define lightDark   vec3(0.7,0.75,0.8)      // 光照颜色 -- 暗部

//---------OUT------------
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

//---------NOISE GENERATION------------
//Noise generation based on a simple hash, to ensure that if a given point on the dome
//(after taking into account the rotation of the sky) is a star, it remains a star all night long
float Hash( float n ){
        return fract( (1.0 + sin(n)) * 415.92653);
}
float Noise3d( vec3 x ){
    float xhash = Hash(round(400*x.x) * 37.0);
    float yhash = Hash(round(400*x.y) * 57.0);
    float zhash = Hash(round(400*x.z) * 67.0);
    return fract(xhash + yhash + zhash);
}

float noise(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = smoothstep(0.0, 1.0, f);
     
    vec2 uv = (p.xy+vec2(37.0, 17.0)*p.z) + f.xy;
    float v1 = texture2D( noisetex, (uv)/256.0, -100.0 ).x;
    float v2 = texture2D( noisetex, (uv + vec2(37.0, 17.0))/256.0, -100.0 ).x;
    return mix(v1, v2, f.z);
}
 
float getCloudNoise(vec3 worldPos) {
    vec3 coord = worldPos;
    coord *= 0.2;
    coord.x += float(frameCounter)*0.02;
    float n  = noise(coord) * 0.5;   coord *= 3.0;
          n += noise(coord) * 0.25;  coord *= 3.01;
          n += noise(coord) * 0.125; coord *= 3.02;
          n += noise(coord) * 0.0625;
    return max(n - 0.5, 0.0) * (1.0 / (1.0 - 0.5));
}

// 计算 pos 点的云密度
float getDensity(sampler2D noisetex, vec3 pos) {
    // 高度衰减
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(weight, 0.5);

    // 采样噪声图
    vec2 coord = pos.xz * 0.0025;
    coord.x += float(frameCounter)*0.0002;
    float noise = texture2D(noisetex, coord).x;
    noise += texture2D(noisetex,coord*3.5).x/3.5;
    noise += texture2D(noisetex,coord*12.25).x/12.25;
    noise += texture2D(noisetex,coord*42.87).x/42.87;
    noise /= 1.4472;
    noise *= weight;

    // 截断
    if(noise<0.5) {
        noise = 0;
    }
    return noise;
}

// 获取体积云颜色
vec4 getCloud(vec3 worldPos, vec3 cameraPos) {
    vec3 direction = normalize(worldPos - cameraPos);   // 视线射线方向
    vec3 step = direction * 0.25;   // 步长
    vec4 colorSum = vec4(0);        // 积累的颜色
    vec3 point = cameraPos;         // 从相机出发开始测试

    // 如果相机在云层下，将测试起始点移动到云层底部 bottom
    if(point.y<bottom) {
        point += direction * (abs(bottom - cameraPos.y) / abs(direction.y));
    }
    // 如果相机在云层上，将测试起始点移动到云层顶部 top
    if(top<point.y) {
        point += direction * (abs(cameraPos.y - top) / abs(direction.y));
    }


    // 如果目标像素遮挡了云层则放弃测试
    float len1 = length(point - cameraPos);     // 云层到眼距离
    float len2 = length(worldPos - cameraPos);  // 目标像素到眼距离
    if(len2<len1) {
        return vec4(0);
    }

    // ray marching
    for(int i=0; i<100; i++) {
        point += step;
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            break;
        }
        
        // 采样
        //float density = getDensity(noisetex, point);                // 当前点云密度
        float density = getCloudNoise(point); 
        vec3 L = normalize(sun_pos - point);                       // 光源方向
        float lightDensity = getDensity(noisetex, point + L);       // 向光源方向采样一次 获取密度
        float delta = clamp(density - lightDensity, 0.0, 1.0);      // 两次采样密度差

        // 控制透明度
        density *= 0.3;

        // 颜色计算
        vec3 base = mix(baseBright, baseDark, density) * density;   // 基础颜色
        vec3 light = mix(lightDark, lightBright, delta);            // 光照对颜色影响

        // 混合
        vec4 color = vec4(base*light, density);                     // 当前点的最终颜色
        colorSum = color * (1.0 - colorSum.a) + colorSum;           // 与累积的颜色混合

        vec4 colorP = vec4(0.9, 0.8, 0.7, 1.0) * density;    // 当前点的颜色
        colorSum = colorSum + colorP * (1.0 - colorSum.a);   // 与累积的颜色混合
    }

    return colorSum;
}


//---------MAIN------------
void main(){
	vec3 color;
    vec3 pos_norm = normalize(pos);
    sun_norm = normalize(sun_pos);
    float dist = dot(sun_norm,pos_norm);

    //vec2 texture_coord=vec2((sun_norm.y + 1.0) / 2.0 ,max(0.01,pos_norm.y));
    vec2 texture_coord=vec2(clamp((sun_norm.y + 1.0) / 2.0, 0.02,0.98),max(0.01,pos_norm.y));
    
    
    //We read the tint texture according to the position of the sun and the weather factor
    vec3 color_wo_sun = texture(tint2, texture_coord).rgb;
    vec3 color_w_sun = texture(tint, texture_coord).rgb;
    color = weather*mix(color_wo_sun,color_w_sun,dist*0.5+0.5);

    
        

    //Computing u and v for the clouds textures (spherical projection)
    float u = texCoord.x;
    float v = texCoord.y;


    //Cloud color
    //color depending on the weather (shade of grey) *  (day or night) ?
    vec3 cloud_color = vec3(min(weather*3.0/2.0,1.0))*(sun_norm.y > 0 ? 0.95 : 0.95+sun_norm.y*1.8);

    //Reading from the clouds maps
    //mixing according to the weather (1.0 -> clouds1 (sunny), 0.5 -> clouds2 (rainy))
    //+ time translation along the u-axis (horizontal) for the clouds movement
    float transparency = mix(texture(clouds2,vec2(u+time,v)).r,texture(clouds1,vec2(u+time,v)).r,(weather-0.5)*2.0);

    // Stars
    if(sun_norm.y<0.1){//Night or dawn
        float threshold = 0.99;
        //We generate a random value between 0 and 1
        float star_intensity = Noise3d(normalize(star_pos));
        //And we apply a threshold to keep only the brightest areas
        if (star_intensity >= threshold){
            //We compute the star intensity
            star_intensity = pow((star_intensity - threshold)/(1.0 - threshold), 6.0)*(-sun_norm.y+0.1);
            color += vec3(star_intensity);
        }
    }

    //Sun
    float radius = length(pos_norm-sun_norm);
    if(radius < 0.05){//We are in the area of the sky which is covered by the sun
        float t = clamp(sun_norm.y,0.01,1.0);
        radius = radius/0.05;
        radius = clamp(radius,0.01,0.8);
        if(radius < 1.0-0.001){//< we need a small bias to avoid flickering on the border of the texture
            //We read the alpha value from a texture where x = radius and y=height in the sky (~time)
            vec4 sun_color = texture(sun,vec2(radius,t));
            color +=sun_color.rgb;
        }
    }

    //Moon
    float radius_moon = length(pos_norm+sun_norm);//the moon is at position -sun_pos
    if(radius_moon < 0.03){//We are in the area of the sky which is covered by the moon
        //We define a local plane tangent to the skydome at -sun_norm
        //We work in model space (everything normalized)
        vec3 n1 = normalize(cross(-sun_norm,vec3(0,1,0)));
        vec3 n2 = normalize(cross(-sun_norm,n1));
        //We project pos_norm on this plane
        float x = dot(pos_norm,n1);
        float y = dot(pos_norm,n2);
        //x,y are two sine, ranging approx from 0 to sqrt(2)*0.03. We scale them to [-1,1], then we will translate to [0,1]
        float scale = 23.57*0.5;
        //we need a compensation term because we made projection on the plane and not on the real sphere + other approximations.
        float compensation = 1.4;
        //And we read in the texture of the moon. The projection we did previously allows us to have an undeformed moon
        //(for the sun we didn't care as there are no details on it)
        color = mix(color,texture(moon,vec2(x,y)*scale*compensation+vec2(0.5)).rgb,clamp(-sun_norm.y*3,0,1));
    }

    //Final mix
    //mixing with the cloud color allows us to hide things behind clouds (sun, stars, moon)
    float kcloud = (clamp(sun_norm.y,-0.15,0.1)+0.15)/0.25*0.9+0.1;
    color = mix(color,cloud_color,clamp((2-weather)*transparency,0,1)*kcloud);	
    vec4 cloud = getCloud(pos, CameraPos); // 云颜色
    
    //float kcloud = clamp(4*(2-weather)*transparency-1,0,1);
    color = color.rgb*(1.0 - cloud.a*kcloud) + cloud.rgb*kcloud;    // 混色


	FragColor = vec4(color, 1.0f);
    BrightColor = vec4(0.0,0.0,0.0,1.0);



}
