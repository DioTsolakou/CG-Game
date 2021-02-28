#include "GeometryNode.h"
#include "GeometricMesh.h"
#include <glm/gtc/type_ptr.hpp>
#include "TextureManager.h"

GeometryNode::GeometryNode()
{
	m_vao = 0;
	m_vbo_positions = 0;
	m_vbo_normals = 0;
	m_vbo_texcoords = 0;
	m_vbo_tangents = 0;
	m_vbo_bitangents = 0;
}

GeometryNode::~GeometryNode()
{
	// delete buffers
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo_positions);
	glDeleteBuffers(1, &m_vbo_normals);
	glDeleteBuffers(1, &m_vbo_tangents);
	glDeleteBuffers(1, &m_vbo_bitangents);
	glDeleteBuffers(1, &m_vbo_texcoords);
}

void GeometryNode::Init(GeometricMesh* mesh)
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo_positions);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
	glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(glm::vec3), &mesh->vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,				// attribute index
		3,              // number of elements per vertex, here (x,y,z)
		GL_FLOAT,       // the type of each element
		GL_FALSE,       // take our values as-is
		0,		         // no extra data between each position
		0				// pointer to the C array or an offset to our buffer
	);

	glGenBuffers(1, &m_vbo_normals);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);
	glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(glm::vec3), &mesh->normals[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,				// attribute index
		3,              // number of elements per vertex, here (x,y,z)
		GL_FLOAT,		// the type of each element
		GL_FALSE,       // take our values as-is
		0,		         // no extra data between each position
		0				// pointer to the C array or an offset to our buffer
	);

	if (!mesh->textureCoord.empty())
	{
		glGenBuffers(1, &m_vbo_texcoords);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_texcoords);
		glBufferData(GL_ARRAY_BUFFER, mesh->textureCoord.size() * sizeof(glm::vec2), &mesh->textureCoord[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(
			2,				// attribute index
			2,              // number of elements per vertex, here (x,y,z)
			GL_FLOAT,		// the type of each element
			GL_FALSE,       // take our values as-is
			0,		         // no extra data between each position
			0				// pointer to the C array or an offset to our buffer
		);
	}

	if (!mesh->tangents.empty())
	{
		glGenBuffers(1, &m_vbo_tangents);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_tangents);
		glBufferData(GL_ARRAY_BUFFER, mesh->tangents.size() * sizeof(glm::vec3), &mesh->tangents[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(
			3,				// attribute index
			3,              // number of elements per vertex, here (x,y,z)
			GL_FLOAT,		// the type of each element
			GL_FALSE,       // take our values as-is
			0,		         // no extra data between each position
			0				// pointer to the C array or an offset to our buffer
		);

		glGenBuffers(1, &m_vbo_tangents);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_tangents);
		glBufferData(GL_ARRAY_BUFFER, mesh->tangents.size() * sizeof(glm::vec3), &mesh->tangents[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(
			3,				// attribute index
			3,              // number of elements per vertex, here (x,y,z)
			GL_FLOAT,		// the type of each element
			GL_FALSE,       // take our values as-is
			0,		         // no extra data between each position
			0				// pointer to the C array or an offset to our buffer
		);

		glGenBuffers(1, &m_vbo_bitangents);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_bitangents);
		glBufferData(GL_ARRAY_BUFFER, mesh->bitangents.size() * sizeof(glm::vec3), &mesh->bitangents[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(4);
		glVertexAttribPointer(
			4,				// attribute index
			3,              // number of elements per vertex, here (x,y,z)
			GL_FLOAT,		// the type of each element
			GL_FALSE,       // take our values as-is
			0,		         // no extra data between each position
			0				// pointer to the C array or an offset to our buffer
		);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// *********************************************************************

	for (int i = 0; i < mesh->objects.size(); i++)
	{
		Objects part;
		part.start_offset = mesh->objects[i].start;
		part.count = mesh->objects[i].end - mesh->objects[i].start;
		auto material = mesh->materials[mesh->objects[i].material_id];

		part.diffuse = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0);
		part.specular = glm::vec3(material.specular[0], material.specular[1], material.specular[2]);
		part.ambient = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]);

		part.shininess = material.shininess;
		part.diffuse_textureID = (material.textureDiffuse.empty()) ? 0 : TextureManager::GetInstance().RequestTexture(material.textureDiffuse.c_str());
		part.mask_textureID = (material.textureSpecular.empty()) ? 0 : TextureManager::GetInstance().RequestTexture(material.textureSpecular.c_str());
		part.emissive_textureID = (material.textureAmbient.empty()) ? 0 : TextureManager::GetInstance().RequestTexture(material.textureAmbient.c_str());
		part.normal_textureID = (material.textureNormal.empty()) ? 0 : TextureManager::GetInstance().RequestTexture(material.textureNormal.c_str());
		part.bump_textureID = (material.textureBump.empty()) ? 0 : TextureManager::GetInstance().RequestTexture(material.textureBump.c_str());

		parts.push_back(part);
	}

	this->m_aabb.min = glm::vec3(std::numeric_limits<float_t>::max());
	this->m_aabb.max = glm::vec3(-std::numeric_limits<float_t>::max());

	for (auto& v : mesh->vertices)
	{
		this->m_aabb.min = glm::min(this->m_aabb.min, v);
		this->m_aabb.max = glm::max(this->m_aabb.max, v);
	}

	this->m_aabb.center = (this->m_aabb.min + this->m_aabb.max) * 0.5f;
}

glm::mat4 GeometryNode::Scale(glm::vec3 s, bool flag)
{
	model_matrix = glm::scale(glm::mat4(1.f), s);
	if (flag) m_aabb.center = glm::vec3(model_matrix * glm::vec4(m_aabb.center, 1.f));
	SetScale(s);
	return model_matrix;
}

glm::mat4 GeometryNode::Move(glm::vec3 p, bool flag)
{
	model_matrix = glm::translate(glm::mat4(1.f), p);
	if (flag) m_aabb.center = glm::vec3(model_matrix * glm::vec4(m_aabb.center, 1.f));
	SetPosition(p);
	return model_matrix;
}

glm::mat4 GeometryNode::Rotate(glm::vec3 r, bool flag)
{
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;

	rotationX = glm::rotate(glm::mat4(1.f), glm::radians(r.x), glm::vec3(1.f, 0.f, 0.f));
	rotationY = glm::rotate(glm::mat4(1.f), glm::radians(r.y), glm::vec3(0.f, 1.f, 0.f));
	rotationZ = glm::rotate(glm::mat4(1.f), glm::radians(r.z), glm::vec3(0.f, 0.f, 1.f));

	model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(m_aabb.center.x, m_aabb.center.y, m_aabb.center.z)) *
		(rotationZ * rotationY * rotationX) *
		glm::translate(glm::mat4(1.f), glm::vec3(-m_aabb.center.x, -m_aabb.center.y, -m_aabb.center.z));
	if (flag) m_aabb.center = glm::vec3(model_matrix * glm::vec4(m_aabb.center, 1.f));

	SetRotation(r);

	return model_matrix;
}

void GeometryNode::Place(glm::vec3 m, glm::vec3 r, glm::vec3 s)
{
	model_matrix = Move(m) * Rotate(r) * Scale(s);
	m_aabb.center = glm::vec3(model_matrix * glm::vec4(m_aabb.center, 1.f));
}

void GeometryNode::ExtractMinMaxFromAABB(GeometryNode& node)
{
	m_aabb_max_planes = glm::vec4(node.m_aabb.max.x, node.m_aabb.max.y, node.m_aabb.max.z, 1.0f);
	m_aabb_min_planes = glm::vec4(node.m_aabb.min.x, node.m_aabb.min.y, node.m_aabb.min.z, 1.0f);
}
