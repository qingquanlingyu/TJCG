#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <iostream>
#include <glad/glad.h>
#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

class DirShadow
{
public:
	unsigned int depthMapFBO;
	unsigned int depthMap;
	DirShadow();
};

class PointShadow
{
public:
	unsigned int depthMapFBO;
	unsigned int depthCubemap;
	PointShadow();
};
