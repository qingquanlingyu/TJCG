#include "SkyDome.h"

SkyDome::SkyDome()
{
	time = MAX_TIME / 2;
	startTime = (float)glfwGetTime();
	SunPos = glm::vec3(0.0f, 0.0f, 0.0f);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glm::vec3 SunPos = glm::vec3(0.0f, 0.0f, 0.0f);

	//生成球模型
	int i, j;
	float u, v;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uv;
	std::vector<glm::vec3> normals;
	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			positions.push_back(glm::vec3(xPos, yPos, zPos));
			uv.push_back(glm::vec2(xSegment, ySegment));
			normals.push_back(glm::vec3(xPos, yPos, zPos));
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}

		// 这里奇偶分开添加是有道理的，奇偶分开添加，就能首位相连，自己可以拿笔画一画
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
	indexCount = indices.size();

	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);
		if (uv.size() > 0)
		{
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
		if (normals.size() > 0)
		{
			data.push_back(normals[i].x);
			data.push_back(normals[i].y);
			data.push_back(normals[i].z);
		}
	}

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), &data[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
}

void SkyDome::BindTexture(Shader SkyDomeShader)
{
	SkyDomeShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, clouds1Map);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, clouds2Map);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tint1Map);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, tint2Map);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, moonMap);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sunMap);
}

SkyDome::~SkyDome() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void SkyDome::setSunPos(glm::vec3 pos) {
	SunPos = pos;
}
glm::vec3 SkyDome::getSunPos() {
	return SunPos;
}

void SkyDome::setTime(unsigned int time) {
	this->time = time;
	startTime = (float)glfwGetTime();
}


void SkyDome::setTimeSpeed(float timeSpeed) {
	this->timeSpeed = timeSpeed;
}


void SkyDome::drawSkyDome(Shader SkyDomeShader, glm::mat4 p, glm::mat4 v)
{
	float time_0 = ((int)(((float)glfwGetTime() - startTime) * timeSpeed) + time) % MAX_TIME * 1.0 / MAX_TIME;	//在0~1之间

	//设置太阳位置
	SunPos = glm::vec3(SkyRadius * std::cos(time_0 * 2 * PI - glm::radians(90.0)), SkyRadius * std::sin(time_0 * 2 * PI - glm::radians(90.0)), 0.0);
	
	SkyDomeShader.use();
	BindTexture(SkyDomeShader);
	glm::mat4 trans, proj, view, rotStars;

	trans = glm::mat4();
	trans = glm::translate(trans, glm::vec3(0.0f, 10.0f, 0.0f));
	trans = glm::scale(trans, glm::vec3(SkyRadius, SkyRadius, SkyRadius));

	proj = p;
	view = v;

	rotStars = glm::mat4();
	rotStars = glm::rotate(rotStars, glm::radians((float)0.0), glm::vec3(0.0, 1.0, 0.0));

	//设置uniform
	SkyDomeShader.setFloat("time", time_0);
	SkyDomeShader.setFloat("weather", 1.0);
	SkyDomeShader.setVec3("sun_pos", SunPos);
	SkyDomeShader.setMat4("rot_stars", rotStars);
	SkyDomeShader.setMat4("trans", trans);
	SkyDomeShader.setMat4("proj", proj);
	SkyDomeShader.setMat4("view", view);

	//绑定VAO，VBO，EBO
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), &data[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

	int stride = (3 + 2 + 3) * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
	
}
