#ifndef SKY_DOME_H
#define SKY_DOME_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>

#include"shader.h"

#define GET_X(r,u,v) r*std::sin(PI*v)*std::cos(2*PI*u)
#define GET_Y(r,u,v) r*std::sin(PI*v)*std::sin(2*PI*u)
#define GET_Z(r,u,v) r*std::sin(PI*v)*std::cos(2*PI*u)

const unsigned int X_SEGMENTS = 256;
const unsigned int Y_SEGMENTS = 256;
const float PI = 3.14159265359;

const unsigned int MAX_TIME = 86400;				//24*60*60穹顶内以秒为单位

const float SkyRadius = 100;						//天空球的半径

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

class SkyDome
{
private:
	unsigned int time;						//穹顶内的时间，其值在
	float timeSpeed = 1000;
	float startTime;
	unsigned int VAO, VBO, EBO;
	unsigned int indexCount;
	std::vector<unsigned int> indices;
	std::vector<float> data;
	glm::vec3 SunPos;
	glm::vec3 CameraPos;
	int frameCounter = 0;
public:
	unsigned int clouds1Map, clouds2Map, tint1Map, tint2Map, moonMap, sunMap, noisetexMap;
	SkyDome();
	~SkyDome();
	void BindTexture(Shader& SkyDomeShader);
	void setSunPos(glm::vec3);
	glm::vec3 getSunPos();
	void setTime(unsigned int);
	int getTime();
	void setTimeSpeed(float);
	void drawSkyDome(Shader& SkyDomeShader, glm::mat4 p, glm::mat4 v);
	void setCameraPos(glm::vec3);
};

#endif
