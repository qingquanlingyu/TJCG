#ifndef OBJECT_H
#define OBJECT_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Object
{
private:
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> normals;
	std::vector<GLfloat> colors;
	std::vector<GLfloat> UVs;
	std::vector<unsigned int> indices;

	void SetByArray(GLfloat data [], int length, std::vector<GLfloat> &targetData, GLuint &targetBuffer);
	void SetByVector(std::vector<GLfloat> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer);
	void SetByVectorVec3(std::vector<glm::vec3> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer);
	void SetByVectorVec2(std::vector<glm::vec2> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer);

public:
	GLuint vertices_buffer;
	GLuint normals_buffer;
	GLuint colors_buffer;
	GLuint UVs_buffer;
	GLuint indices_buffer;
	
	void SetVertex(GLfloat vertices[], int length);
	void SetVertex(std::vector<GLfloat> vertices);
	void SetVertex(std::vector<glm::vec3> vertices);

	void SetNormal(GLfloat normals[], int length);
	void SetNormal(std::vector<GLfloat> normals);
	void SetNormal(std::vector<glm::vec3> normals);

	void SetColor(GLfloat colors [], int length);
	void SetColor(std::vector<GLfloat> colors);
	void SetColor(std::vector<glm::vec3> colors);

	void SetUVs(GLfloat UVs [], int length);
	void SetUVs(std::vector<GLfloat> UVs);
	void SetUVs(std::vector<glm::vec2> UVs);

	void SetIndices(std::vector<unsigned int> indices);

	void UpdateVertex(std::vector<glm::vec3> vertices);
	void UpdateNormal(std::vector<glm::vec3> normals);

};


void Object::SetByArray(GLfloat data[], int length, std::vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	targetData = std::vector<GLfloat>(data, data + length);
	SetByVector(targetData, targetData, targetBuffer);
}
void Object::SetByVector(std::vector<GLfloat> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	targetData = data;
	glGenBuffers(1, &targetBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, targetBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * targetData.size(), &targetData[0], GL_STATIC_DRAW);
}
void Object::SetByVectorVec3(std::vector<glm::vec3> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	for (size_t i = 0; i < data.size(); i++)
	{
		targetData.push_back(data[i].x);
		targetData.push_back(data[i].y);
		targetData.push_back(data[i].z);
	}
	SetByVector(targetData, targetData, targetBuffer);
}
void Object::SetByVectorVec2(std::vector<glm::vec2> data, std::vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	for (size_t i = 0; i < data.size(); i++)
	{
		targetData.push_back(data[i].x);
		targetData.push_back(data[i].y);
	}
	SetByVector(targetData, targetData, targetBuffer);
}


void Object::SetVertex(GLfloat vertices [], int length)
{
	SetByArray(vertices, length, this->vertices, vertices_buffer);
}
void Object::SetVertex(std::vector<GLfloat> vertices)
{
	SetByVector(vertices, this->vertices, vertices_buffer);
}
void Object::SetVertex(std::vector<glm::vec3> vertices)
{
	SetByVectorVec3(vertices, this->vertices, vertices_buffer);
}


void Object::SetNormal(std::vector<GLfloat> normals)
{
	SetByVector(normals, this->normals, normals_buffer);
}
void Object::SetNormal(GLfloat normals[], int length)
{
	SetByArray(normals, length, this->normals, normals_buffer);
}
void Object::SetNormal(std::vector<glm::vec3> normals)
{
	SetByVectorVec3(normals, this->normals, normals_buffer);
}


void Object::SetColor(std::vector<GLfloat> colors)
{
	SetByVector(colors, this->colors, colors_buffer);
}
void Object::SetColor(GLfloat colors [], int length)
{
	SetByArray(colors, length, this->colors, colors_buffer);
}
void Object::SetColor(std::vector<glm::vec3> colors)
{
	SetByVectorVec3(colors, this->colors, colors_buffer);
}


void Object::SetUVs(std::vector<GLfloat> UVs)
{
	SetByVector(UVs, this->UVs, UVs_buffer);
}

void Object::SetUVs(GLfloat UVs [], int length)
{
	SetByArray(UVs, length, this->UVs, UVs_buffer);
}

void Object::SetUVs(std::vector<glm::vec2> UVs)
{
	SetByVectorVec2(UVs, this->UVs, UVs_buffer);
}


void Object::SetIndices(std::vector<unsigned int> indices)
{
	this->indices = indices;
	glGenBuffers(1, &indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * this->indices.size(), &indices[0], GL_STATIC_DRAW);
}

void Object::UpdateVertex(std::vector<glm::vec3> vertices)
{
	for (size_t i = 0; i < vertices.size(); i++)
	{
		this->vertices[i * 3] = vertices[i].x;
		this->vertices[i * 3 + 1] = vertices[i].y;
		this->vertices[i * 3 + 2] = vertices[i].z;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vertices_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * this->vertices.size(), &this->vertices[0], GL_STATIC_DRAW);
}

void Object::UpdateNormal(std::vector<glm::vec3> normals)
{
	for (size_t i = 0; i < normals.size(); i++)
	{
		this->normals[i * 3] = normals[i].x;
		this->normals[i * 3 + 1] = normals[i].y;
		this->normals[i * 3 + 2] = normals[i].z;
	}
	glBindBuffer(GL_ARRAY_BUFFER, normals_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * this->normals.size(), &this->normals[0], GL_STATIC_DRAW);
}
#endif