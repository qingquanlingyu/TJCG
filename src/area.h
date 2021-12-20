#ifndef AERA_H
#define AERA_H
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define INDEX(x, z, x_width) (x+z*x_width)

class Area
{
protected:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

private:
	int x_width;
	int z_width;
	float x_spacing;
	float z_spacing;
	int x_instance;
	int z_instance;
	
	std::vector<unsigned int> indices;
	std::vector<glm::vec2> UVs;
	std::vector<glm::vec3> instance_offset;

public:
	Area(int x_width, int z_width, float length_x, float length_z, int x_instance, int z_instance);
	

	int GetWidth_X();
	int GetWidth_Z();
	float GetSpacing();
	int GetInstance_X();
	int GetInstance_Z();

	std::vector<glm::vec3> GetVertices();
	std::vector<glm::vec3> GetNormals();
	std::vector<unsigned int> GetIndices();
	std::vector<glm::vec2> GetUVs();
	std::vector<glm::vec3> GetInstance_offset();
};


Area::Area(int x_width, int z_width, float length_x, float length_z, int x_instance, int z_instance)
{

	this->x_width = x_width;
	this->z_width = z_width;
	this->x_spacing = length_x / (x_width - 1);
	this->z_spacing = length_z / (z_width - 1);
	this->x_instance = x_instance;
	this->z_instance = z_instance;

	for (int z = 0; z < z_width; z++)
	{
		for (int x = 0; x < x_width; x++)
		{
			vertices.push_back(glm::vec3(x * x_spacing, 0, z * z_spacing));
			normals.push_back(glm::vec3(0, 1, 0));
			
			UVs.push_back(glm::vec2(z/ (float)z_width, x/ (float)x_width));
			// Filling indices
			if (z > 0 && x > 0)
			{
				// Bottom triangle
				indices.push_back(INDEX((x - 1), (z - 1), x_width));
				indices.push_back(INDEX((x - 1), (z - 0), x_width));
				indices.push_back(INDEX((x - 0), (z - 0), x_width));

				// Top triangle
				indices.push_back(INDEX((x - 1), (z - 1), x_width));
				indices.push_back(INDEX((x - 0), (z - 0), x_width));
				indices.push_back(INDEX((x - 0), (z - 1), x_width));
			}
		}
	}

	for (int z = 0; z < z_instance; z++)
	{
		int a;
		for (int x = 0; x < x_instance; x++)
		{
			instance_offset.push_back(glm::vec3(x * length_x,0, z * length_z));
			int b;
		}
	}
}

int Area::GetWidth_X()
{
	return x_width;
}

int Area::GetWidth_Z()
{
	return z_width;
}

int Area::GetInstance_X()
{
	return x_instance;
}

int Area::GetInstance_Z()
{
	return z_instance;
}

std::vector<glm::vec3> Area::GetVertices()
{
	return vertices;
}

std::vector<glm::vec3> Area::GetNormals()
{
	return normals;
}

std::vector<unsigned int> Area::GetIndices()
{
	return indices;
}

std::vector<glm::vec2> Area::GetUVs()
{
	return UVs;
}

std::vector<glm::vec3> Area::GetInstance_offset()
{
	return instance_offset;
}
#endif

