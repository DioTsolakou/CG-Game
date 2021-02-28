#ifndef GEOMETRY_NODE_H
#define GEOMETRY_NODE_H

#include <vector>
#include "GLEW\glew.h"
#include <unordered_map>
#include "glm\gtx\hash.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class GeometryNode
{
private:
	int type;
	glm::vec3 scale;
	glm::vec3 position;
	glm::vec3 rotation;
public:
	GeometryNode();
	virtual ~GeometryNode();

	virtual void Init(class GeometricMesh* mesh);

	int GetType()
	{
		return type;
	}
	void SetType(int t)
	{
		type = t;
	}

	glm::vec3 GetScale()
	{
		return scale;
	}

	void SetScale(glm::vec3 s)
	{
		scale.x = s.x;
		scale.y = s.y;
		scale.z = s.z;
	}

	glm::mat4 Scale(glm::vec3 s, bool flag = false);

	glm::vec3 GetPosition()
	{
		return position;
	}

	void SetPosition(glm::vec3 p)
	{
		position.x = p.x;
		position.y = p.y;
		position.z = p.z;
	}

	glm::mat4 Move(glm::vec3 p, bool flag = false);

	glm::vec3 GetRotation()
	{
		return rotation;
	}

	void SetRotation(glm::vec3 r)
	{
		rotation.x = r.x;
		rotation.y = r.y;
		rotation.z = r.z;
	}

	glm::mat4 Rotate(glm::vec3 r, bool flag = false);

	void Place(glm::vec3 m, glm::vec3 r, glm::vec3 s);

	struct Objects
	{
		unsigned int start_offset;
		unsigned int count;

		glm::vec3 diffuse;
		glm::vec3 ambient;
		glm::vec3 specular;

		float shininess;
		GLuint diffuse_textureID;
		GLuint normal_textureID;
		GLuint bump_textureID;
		GLuint emissive_textureID;
		GLuint mask_textureID;
	};

	struct aabb
	{
		glm::vec3 min;
		glm::vec3 max;
		glm::vec3 center;
	};

	std::vector<Objects> parts;

	glm::mat4 model_matrix;
	glm::mat4 app_model_matrix;
	aabb m_aabb;

	GLuint m_vao;
	GLuint m_vbo_positions;
	GLuint m_vbo_normals;
	GLuint m_vbo_tangents;
	GLuint m_vbo_bitangents;
	GLuint m_vbo_texcoords;
};

#endif