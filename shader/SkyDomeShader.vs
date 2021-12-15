#version 430 core
//---------IN------------
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
//---------UNIFORM------------
uniform vec3 sun_pos;               //sun position in world space
uniform mat4 trans;
uniform mat4 proj;
uniform mat4 view;
uniform mat4 rot_stars;//rotation matrix for the stars
//---------OUT------------
out vec3 pos;
out vec2 texCoord;
out vec3 sun_norm;
out vec3 star_pos;

//---------MAIN------------
void main()
{
    gl_Position = proj*view*trans*vec4(aPos, 1.0);
    pos = vec3(trans * vec4(aPos, 1.0));

    //Sun pos being a constant vector, we can normalize it in the vshader
    //and pass it to the fshader without having to re-normalize it
    sun_norm = normalize(sun_pos);

    //And we compute an approximate star position using the special rotation matrix
    star_pos = mat3(rot_stars) * normalize(pos);

    texCoord = aTexCoords;
}
