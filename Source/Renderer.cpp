#include "Renderer.h"
#include "GeometryNode.h"
#include "Tools.h"
#include "ShaderProgram.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "OBJLoader.h"

#include <algorithm>
#include <array>
#include <iostream>

// RENDERER
Renderer::Renderer()
{
	this->m_nodes = {};
	this->m_continous_time = 0.0;
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_fbo_texture);
	glDeleteFramebuffers(1, &m_fbo);

	glDeleteVertexArrays(1, &m_vao_fbo);
	glDeleteBuffers(1, &m_vbo_fbo_vertices);
}

bool Renderer::Init(int SCREEN_WIDTH, int SCREEN_HEIGHT)
{
	this->m_screen_width = SCREEN_WIDTH;
	this->m_screen_height = SCREEN_HEIGHT;

	bool techniques_initialization = InitShaders();

	bool meshes_initialization = InitGeometricMeshes();
	bool light_initialization = InitLights();

	bool common_initialization = InitCommonItems();
	bool inter_buffers_initialization = InitIntermediateBuffers();

	//If there was any errors
	if (Tools::CheckGLError() != GL_NO_ERROR)
	{
		printf("Exiting with error at Renderer::Init\n");
		return false;
	}

	this->BuildWorld();
	this->InitCamera();

	//If everything initialized
	return techniques_initialization && meshes_initialization &&
		common_initialization && inter_buffers_initialization;
}

void Renderer::BuildWorld()
{
	/*GeometryNode& beam = *this->m_nodes[MAP_ASSETS::BEAM];
	GeometryNode& cannon = *this->m_nodes[MAP_ASSETS::CANNON];
	GeometryNode& cannon_mount = *this->m_nodes[MAP_ASSETS::CANNON_MOUNT];*/


	//CollidableNode& corridor_straight = *this->m_collidables_nodes[0];
	
	/*beam.model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 5.f, 0.f));
	beam.m_aabb.center = glm::vec3(beam.model_matrix * glm::vec4(beam.m_aabb.center, 1.f));

	glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(-60.f), glm::vec3(0.f, 1.f, 0.f));

	cannon.model_matrix = 
		glm::translate(glm::mat4(1.f), glm::vec3(cannon.m_aabb.center.x, cannon.m_aabb.center.y, cannon.m_aabb.center.z)) *
		R *
		glm::translate(glm::mat4(1.f), glm::vec3(-cannon.m_aabb.center.x, -cannon.m_aabb.center.y, -cannon.m_aabb.center.z));


	cannon.m_aabb.center = glm::vec3(cannon.model_matrix * glm::vec4(cannon.m_aabb.center, 1.f));

	cannon_mount.model_matrix = glm::mat4(1.f);*/

	/*CollidableNode& pipe0 = *this->m_collidables_nodes[0];
	CollidableNode& pipe1 = *this->m_collidables_nodes[1];
	CollidableNode& pipe2 = *this->m_collidables_nodes[2];

	pipe0.model_matrix = rotate(pipe0, 181.25f, 173.8f, 90.74f);
	pipe0.m_aabb.center = glm::vec3(pipe0.model_matrix * glm::vec4(pipe0.m_aabb.center, 1.f));

	pipe1.model_matrix = move(pipe1, 0.f, 5.f, 0.f) * rotate(pipe1, 90.f, 0.f, 0.f);
	pipe1.m_aabb.center = glm::vec3(pipe1.model_matrix * glm::vec4(pipe1.m_aabb.center, 1.f));

	pipe2.model_matrix = move(pipe2, 5.f, 0.f, 0.f);
	pipe2.m_aabb.center = glm::vec3(pipe2.model_matrix * glm::vec4(pipe2.m_aabb.center, 1.f));*/

	//createMap();

	this->m_world_matrix = glm::mat4(1.f);
}

void Renderer::InitCamera()
{
	this->FOV = 90.f;
	this->aspectRatio = this->m_screen_width / (float)this->m_screen_height;
	this->nearPlane = 0.1f;
	this->farPlane = 10000.f;

	this->m_camera_position = glm::vec3(0, 0, -1.5);
	this->m_camera_target_position = glm::vec3(0, 0, 0.1);
	this->m_camera_up_vector = glm::vec3(0, 1, 0);

	this->m_view_matrix = glm::lookAt(this->m_camera_position, this->m_camera_target_position, m_camera_up_vector);

	this->m_projection_matrix = glm::perspective(glm::radians(FOV), aspectRatio, nearPlane, farPlane);
}

bool Renderer::InitLights()
{
	this->m_light.SetColor(glm::vec3(70.f));
	this->m_light.SetPosition(this->m_camera_position);
	this->m_light.SetTarget(this->m_camera_target_position);
	this->m_light.SetConeSize(500, 1000);
	this->m_light.CastShadow(false);

	return true;
}

bool Renderer::InitShaders()
{
	std::string vertex_shader_path = "Assets/Shaders/basic_rendering.vert";
	std::string fragment_shader_path = "Assets/Shaders/basic_rendering.frag";

	m_geometry_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_geometry_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	m_geometry_program.CreateProgram();

	vertex_shader_path = "Assets/Shaders/post_process.vert";
	fragment_shader_path = "Assets/Shaders/post_process.frag";

	m_post_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_post_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	m_post_program.CreateProgram();

	vertex_shader_path = "Assets/Shaders/shadow_map_rendering.vert";
	fragment_shader_path = "Assets/Shaders/shadow_map_rendering.frag";

	m_spot_light_shadow_map_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_spot_light_shadow_map_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	m_spot_light_shadow_map_program.CreateProgram();

	return true;
}

bool Renderer::InitIntermediateBuffers()
{
	glGenTextures(1, &m_fbo_texture);
	glGenTextures(1, &m_fbo_depth_texture);
	glGenFramebuffers(1, &m_fbo);

	return ResizeBuffers(m_screen_width, m_screen_height);
}

bool Renderer::ResizeBuffers(int width, int height)
{
	m_screen_width = width;
	m_screen_height = height;

	// texture
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_screen_width, m_screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// framebuffer to link to everything together
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depth_texture, 0);

	GLenum status = Tools::CheckFramebufferStatus(m_fbo);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool Renderer::InitCommonItems()
{
	glGenVertexArrays(1, &m_vao_fbo);
	glBindVertexArray(m_vao_fbo);

	GLfloat fbo_vertices[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1, };

	glGenBuffers(1, &m_vbo_fbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_fbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fbo_vertices), fbo_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);
	return true;
}

bool Renderer::InitGeometricMeshes()
{
	std::array<const char*, MAP_ASSETS::SIZE_ALL> mapAssets = {
		"Assets/Beam/Beam.obj",
		//"Assets/Beam/CH-Beam.obj",

		//"Assets/Cannon/Cannon.obj",
		//"Assets/Cannon/CH-Cannon.obj",

		//"Assets/Cannon/CannonMount.obj",
		//"Assets/Cannon/CH-CannonMount.obj",

		"Assets/Corridor/Corridor_Curve.obj",

		"Assets/Corridor/Corridor_Fork.obj",
		//"Assets/Corridor/CH-Corridor_Fork.obj",

		"Assets/Corridor/Corridor_Straight.obj",
		//"Assets/Corridor/CH-Corridor_Straight.obj",

		"Assets/Corridor/Corridor_Left.obj",
		//"Assets/Corridor/CH-Corridor_Left.obj",

		"Assets/Corridor/Corridor_Right.obj",
		//"Assets/Corridor/CH-Corridor_Right.obj",
		
		//"Assets/Iris/Iris.obj",
		//"Assets/Iris/CH-Iris.obj",

		"Assets/Pipe/Pipe.obj",
		//"Assets/Pipe/CH-Pipe.obj",

		"Assets/Wall/Wall.obj",
		//"Assets/Wall/CH-Wall.obj",
	};

	bool initialized = true;
	//OBJLoader loader;

	/*for (int32_t i = 0; i < MAP_ASSETS::SIZE_ALL - 1; ++i)
	{
		auto& asset = mapAssets[i];
		GeometricMesh* mesh = loader.load(asset);

		if (mesh != nullptr)
		{
			GeometryNode* node = new GeometryNode();
			node->Init(mesh);
			this->m_nodes.push_back(node);
			delete mesh;
		}
		else
		{
			initialized = false;
		}
	}*/

	/*for (int i = 0; i < 92; i++)
	{
		GeometricMesh* mesh;
		if (i < 3)
		{
			mesh = loader.load(mapAssets[PIPE]);
		}
		else if (i < 7)
		{
			mesh = loader.load(mapAssets[CORRIDOR_FORK]);
		}
		else if (i < 12)
		{
			mesh = loader.load(mapAssets[CORRIDOR_RIGHT]);
		}
		else if (i < 18)
		{
			mesh = loader.load(mapAssets[BEAM]);
		}
		else if (i < 34)
		{
			mesh = loader.load(mapAssets[CORRIDOR_LEFT]);
		}
		else if (i < 52)
		{
			mesh = loader.load(mapAssets[CORRIDOR_CURVE]);
		}
		else
		{
			mesh = loader.load(mapAssets[CORRIDOR_STRAIGHT]);
		}


		if (mesh != nullptr)
		{
			CollidableNode* node = new CollidableNode();
			node->Init(mesh);
			this->m_collidables_nodes.push_back(node);
			delete mesh;
		}
		else
		{
			initialized = false;
		}
	}*/


	// these should probably called inside a method called buildMap, hence the rename to placeObject

	// left path
	this->placeObject(initialized, mapAssets, WALL, glm::vec3(3.f, 0.f, 0.5f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, WALL, glm::vec3(-3.f, 0.f, 0.5f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_FORK, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, PIPE, glm::vec3(0.f, 0.f, -11.f), glm::vec3(90.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, BEAM, glm::vec3(0.f, 0.f, -18.f), glm::vec3(0.f, 0.f, 90.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_LEFT, glm::vec3(-5.f, 0.f, -20.f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-10.f, 0.f, -40.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(-1.f, 1.f, 1.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-42.5f, 0.f, -40.f), glm::vec3(0.f, 0.f, 0.f)); // corridor curve needs to be 32.5 meters left/right to create 180 turn
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-42.5f, 0.f, -20.f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-42.5f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-54.25f, 0.f, 22.25f), glm::vec3(0.f, 180.f, 0.f)); // corridor curve is 11.75m in X axis and 22.25m in Z axis
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-69.5f, 0.f, 27.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-89.5f, 0.f, 27.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-109.5f, 0.f, 27.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-129.5f, 0.f, 27.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-149.5f, 0.f, 27.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-176.5f, 0.f, 22.25f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-176.5f, 0.f, 0.25f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-149.5f, 0.f, -6.75f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(-129.5f, 0.f, -6.75f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-114.5f, 0.f, -11.5f), glm::vec3(0.f, 180.f, 0.f)); // curve needs to have a difference of 4.75 in Z axis to properly align with straight
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-102.75f, 0.f, -33.75f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(-80.5f, 0.f, -45.5f), glm::vec3(0.f, 180.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_FORK, glm::vec3(-68.75f, 0.f, -67.75f), glm::vec3(0.f, 0.f, 90.f));

	// right path
	this->placeObject(initialized, mapAssets, CORRIDOR_RIGHT, glm::vec3(5.f, 0.f, -20.f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(10.f, 0.f, -40.f), glm::vec3(0.f, 0.f, 0.f));


	return initialized;
}

void Renderer::Update(float dt)
{
	this->UpdateGeometry(dt);
	this->UpdateCamera(dt);
	m_continous_time += dt;
}

void Renderer::UpdateGeometry(float dt)
{

	for (auto& node : this->m_collidables_nodes)
	{
		node->app_model_matrix = node->model_matrix;
	}
}

void Renderer::UpdateCamera(float dt)
{
	glm::vec3 direction = glm::normalize(m_camera_target_position - m_camera_position);

	m_camera_position = m_camera_position + (m_camera_movement.x * 5.f * dt) * direction;
	m_camera_target_position = m_camera_target_position + (m_camera_movement.x * 5.f * dt) * direction;

	glm::vec3 right = glm::normalize(glm::cross(direction, m_camera_up_vector));

	m_camera_position = m_camera_position + (m_camera_movement.y * 5.f * dt) * right;
	m_camera_target_position = m_camera_target_position + (m_camera_movement.y * 5.f * dt) * right;

	m_camera_position = m_camera_position + (m_camera_movement.z * 5.f * dt) * direction;
	m_camera_target_position = m_camera_target_position + (m_camera_movement.z * 5.f * dt) * direction;

	float speed = glm::pi<float>() * 0.002f;
	glm::mat4 rotation = glm::rotate(glm::mat4(1.f), m_camera_look_angle_destination.y * speed, right);
	rotation *= glm::rotate(glm::mat4(1.f), m_camera_look_angle_destination.x * speed, m_camera_up_vector);
	m_camera_look_angle_destination = glm::vec2(0.f);

	direction = rotation * glm::vec4(direction, 0.f);
	m_camera_target_position = m_camera_position + direction * glm::distance(m_camera_position, m_camera_target_position);

	m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

	//std::cout << m_camera_position.x << " " << m_camera_position.y << " " << m_camera_position.z << " " << std::endl;
	//std::cout << m_camera_target_position.x << " " << m_camera_target_position.y << " " << m_camera_target_position.z << " " << std::endl;
	m_light.SetPosition(m_camera_position);
	m_light.SetTarget(m_camera_target_position);
}

bool Renderer::ReloadShaders()
{
	m_geometry_program.ReloadProgram();
	m_post_program.ReloadProgram();
	return true;
}

void Renderer::Render()
{
	RenderShadowMaps();
	RenderGeometry();
	RenderPostProcess();

	GLenum error = Tools::CheckGLError();

	if (error != GL_NO_ERROR)
	{
		printf("Reanderer:Draw GL Error\n");
		system("pause");
	}
}

void Renderer::RenderPostProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.f, 0.8f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	m_post_program.Bind();

	glBindVertexArray(m_vao_fbo);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	m_post_program.loadInt("uniform_texture", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_light.GetShadowMapDepthTexture());
	m_post_program.loadInt("uniform_shadow_map", 1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_post_program.Unbind();
}

void Renderer::RenderStaticGeometry()
{
	glm::mat4 proj = m_projection_matrix * m_view_matrix * m_world_matrix;

	for (auto& node : this->m_nodes)
	{
		glBindVertexArray(node->m_vao);

		m_geometry_program.loadMat4("uniform_projection_matrix", proj * node->app_model_matrix);
		m_geometry_program.loadMat4("uniform_normal_matrix", glm::transpose(glm::inverse(m_world_matrix * node->app_model_matrix)));
		m_geometry_program.loadMat4("uniform_world_matrix", m_world_matrix * node->app_model_matrix);

		for (int j = 0; j < node->parts.size(); ++j)
		{
			m_geometry_program.loadVec3("uniform_diffuse", node->parts[j].diffuse);
			m_geometry_program.loadVec3("uniform_ambient", node->parts[j].ambient);
			m_geometry_program.loadVec3("uniform_specular", node->parts[j].specular);
			m_geometry_program.loadFloat("uniform_shininess", node->parts[j].shininess);
			m_geometry_program.loadInt("uniform_has_tex_diffuse", (node->parts[j].diffuse_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_normal", (node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_is_tex_bumb", (node->parts[j].bump_textureID > 0) ? 1 : 0);

			glActiveTexture(GL_TEXTURE0);
			m_geometry_program.loadInt("uniform_tex_diffuse", 0);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].diffuse_textureID);

			glActiveTexture(GL_TEXTURE1);
			m_geometry_program.loadInt("uniform_tex_normal", 1);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].bump_textureID > 0 ?
				node->parts[j].bump_textureID : node->parts[j].normal_textureID);

			glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
		}

		glBindVertexArray(0);
	}
}

void Renderer::RenderCollidableGeometry()
{
	glm::mat4 proj = m_projection_matrix * m_view_matrix * m_world_matrix;

	glm::vec3 camera_dir = normalize(m_camera_target_position - m_camera_position);
	float_t isectT = 0.f;

	for (auto& node : this->m_collidables_nodes)
	{
		//if (node->intersectRay(m_camera_position, camera_dir, m_world_matrix, isectT)) continue; // line for collision detection through rays, atm doesn't render an object if the rays hit it

		glBindVertexArray(node->m_vao);

		m_geometry_program.loadMat4("uniform_projection_matrix", proj * node->app_model_matrix);
		m_geometry_program.loadMat4("uniform_normal_matrix", glm::transpose(glm::inverse(m_world_matrix * node->app_model_matrix)));
		m_geometry_program.loadMat4("uniform_world_matrix", m_world_matrix * node->app_model_matrix);

		for (int j = 0; j < node->parts.size(); ++j)
		{
			m_geometry_program.loadVec3("uniform_diffuse", node->parts[j].diffuse);
			m_geometry_program.loadVec3("uniform_ambient", node->parts[j].ambient);
			m_geometry_program.loadVec3("uniform_specular", node->parts[j].specular);
			m_geometry_program.loadFloat("uniform_shininess", node->parts[j].shininess);
			m_geometry_program.loadInt("uniform_has_tex_diffuse", (node->parts[j].diffuse_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_normal", (node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_is_tex_bumb", (node->parts[j].bump_textureID > 0) ? 1 : 0);

			glActiveTexture(GL_TEXTURE0);
			m_geometry_program.loadInt("uniform_tex_diffuse", 0);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].diffuse_textureID);

			glActiveTexture(GL_TEXTURE1);
			m_geometry_program.loadInt("uniform_tex_normal", 1);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].bump_textureID > 0 ?
				node->parts[j].bump_textureID : node->parts[j].normal_textureID);

			glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
		}

		glBindVertexArray(0);
	}
}

void Renderer::RenderGeometry()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawbuffers);

	glViewport(0, 0, m_screen_width, m_screen_height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_geometry_program.Bind();

	glm::mat4 proj = m_projection_matrix * m_view_matrix * m_world_matrix;

	for (auto& node : this->m_nodes)
	{
		glBindVertexArray(node->m_vao);

		glUniformMatrix4fv(m_geometry_program["uniform_projection_matrix"], 1, GL_FALSE,
			glm::value_ptr(proj * node->app_model_matrix));

		glm::mat4 normal_matrix = glm::transpose(glm::inverse(m_world_matrix * node->app_model_matrix));

		glUniformMatrix4fv(m_geometry_program["uniform_normal_matrix"], 1, GL_FALSE,
			glm::value_ptr(normal_matrix));

		for (int j = 0; j < node->parts.size(); ++j)
		{
			glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
		}

		glBindVertexArray(0);
	}

	m_geometry_program.loadVec3("uniform_light_color", m_light.GetColor());
	m_geometry_program.loadVec3("uniform_light_dir", m_light.GetDirection());
	m_geometry_program.loadVec3("uniform_light_pos", m_light.GetPosition());

	m_geometry_program.loadFloat("uniform_light_umbra", m_light.GetUmbra());
	m_geometry_program.loadFloat("uniform_light_penumbra", m_light.GetPenumbra());

	m_geometry_program.loadVec3("uniform_camera_pos", m_camera_position);
	m_geometry_program.loadVec3("uniform_camera_dir", normalize(m_camera_target_position - m_camera_position));

	m_geometry_program.loadMat4("uniform_light_projection_view", m_light.GetProjectionMatrix() * m_light.GetViewMatrix());
	m_geometry_program.loadInt("uniform_cast_shadows", m_light.GetCastShadowsStatus() ? 1 : 0);

	glActiveTexture(GL_TEXTURE2);
	m_geometry_program.loadInt("uniform_shadow_map", 2);
	glBindTexture(GL_TEXTURE_2D, m_light.GetShadowMapDepthTexture());

	RenderStaticGeometry();
	RenderCollidableGeometry();

	m_geometry_program.Unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	
}

void Renderer::RenderShadowMaps()
{
	if (m_light.GetCastShadowsStatus())
	{
		int m_depth_texture_resolution = m_light.GetShadowMapResolution();

		glBindFramebuffer(GL_FRAMEBUFFER, m_light.GetShadowMapFBO());
		glViewport(0, 0, m_depth_texture_resolution, m_depth_texture_resolution);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Bind the shadow mapping program
		m_spot_light_shadow_map_program.Bind(); // !!!!

		glm::mat4 proj = m_light.GetProjectionMatrix() * m_light.GetViewMatrix() * m_world_matrix;

		for (auto& node : this->m_nodes)
		{
			glBindVertexArray(node->m_vao);

			m_spot_light_shadow_map_program.loadMat4("uniform_projection_matrix", proj * node->app_model_matrix);

			for (int j = 0; j < node->parts.size(); ++j)
			{
				glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
			}

			glBindVertexArray(0);
		}

		glm::vec3 camera_dir = normalize(m_camera_target_position - m_camera_position);
		float_t isectT = 0.f;

		for (auto& node : this->m_collidables_nodes)
		{
			if (node->intersectRay(m_camera_position, camera_dir, m_world_matrix, isectT)) continue;

			glBindVertexArray(node->m_vao);

			m_spot_light_shadow_map_program.loadMat4("uniform_projection_matrix", proj * node->app_model_matrix);

			for (int j = 0; j < node->parts.size(); ++j)
			{
				glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
			}

			glBindVertexArray(0);
		}

		m_spot_light_shadow_map_program.Unbind();
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void Renderer::CameraMoveForward(bool enable, float &factor)
{
	m_camera_movement.x = (enable) ? 2*factor : 0;
}

void Renderer::CameraMoveBackWard(bool enable)
{
	m_camera_movement.x = (enable) ? -1.25 : 0;
}

void Renderer::CameraMoveLeft(bool enable)
{
	m_camera_movement.y = (enable) ? -1.5 : 0;
}

void Renderer::CameraMoveRight(bool enable)
{
	m_camera_movement.y = (enable) ? 1.5 : 0;
}

void Renderer::CameraLook(glm::vec2 lookDir)
{
	m_camera_look_angle_destination = lookDir * glm::vec2(1.5f, 1.5f);
}

void Renderer::CameraRollLeft(bool enable)
{
	glm::mat4 roll_mat = glm::rotate(glm::mat4(1.0f), glm::radians(5.f), m_camera_target_position);
	m_camera_up_vector = glm::mat3(roll_mat) * m_camera_up_vector;
}

void Renderer::CameraRollRight(bool enable)
{
	glm::mat4 roll_mat = glm::rotate(glm::mat4(1.0f), glm::radians(-5.f), m_camera_target_position);
	m_camera_up_vector = glm::mat3(roll_mat) * m_camera_up_vector;
}

glm::mat4 Renderer::rotate(CollidableNode& object, glm::vec3 rotation)
{
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;

	rotationX = glm::rotate(glm::mat4(1.f), glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f));
	rotationY = glm::rotate(glm::mat4(1.f), glm::radians(rotation.y), glm::vec3(0.f, 1.f, 0.f));
	rotationZ = glm::rotate(glm::mat4(1.f), glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f));

	object.model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(object.m_aabb.center.x, object.m_aabb.center.y, object.m_aabb.center.z)) *
		(rotationZ * rotationY * rotationX) *
		glm::translate(glm::mat4(1.f), glm::vec3(-object.m_aabb.center.x, -object.m_aabb.center.y, -object.m_aabb.center.z));

	return object.model_matrix;
}

glm::mat4 Renderer::move(CollidableNode& object, glm::vec3 movement)
{
	object.model_matrix = glm::translate(glm::mat4(1.f), movement);
	return object.model_matrix;
}

glm::mat4 Renderer::scale(CollidableNode& object, glm::vec3 scale)
{
	object.model_matrix = glm::scale(glm::mat4(1.f), scale);
	return object.model_matrix;
}

//void Renderer::createMap()
//{
//	//pipes
//	CollidableNode* temp = &(*this->m_collidables_nodes[0]);
//	(*temp).model_matrix = move(*temp, 0.f, 0.f, -10.f) * rotate(*temp, 90.f, 0.f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[1]);
//	(*temp).model_matrix = move((*temp), 59.12f, -147.24f, -6.15) * rotate((*temp), 0.74f, 173.79f, 181.257);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[2]);
//	(*temp).model_matrix = move((*temp), -71.15f, 75.13f, -0.82f) * rotate((*temp), -90.72f, -0.141f, -87.77f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	//corridorFork
//	temp = &(*this->m_collidables_nodes[3]);
//	(*temp).model_matrix = move((*temp), 0.f, 0.f, 0.f) * rotate((*temp), 0.f, 0.f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[4]);
//	(*temp).model_matrix = move((*temp), 9.f, -72.f, 190.f) * rotate((*temp), 0.f, -90.f, 90.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[5]);
//	(*temp).model_matrix = move((*temp), 115.f, -70.f, 300.f) * rotate((*temp), 0.f, 89.747f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[6]);
//	(*temp).model_matrix = move((*temp), -45.f, -90.f, -77.f) * rotate((*temp), 0.f, 0.f, 90.95f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	//corridorRight
//	temp = &(*this->m_collidables_nodes[7]);
//	(*temp).model_matrix = move((*temp), 16.76f, 10.f, 3.07f) * rotate((*temp), 0.f, 0.f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[8]);
//	(*temp).model_matrix = move((*temp), 80.6f, 9.36f, 59.85f) * rotate((*temp), -4.76f, -94.47f, -44.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[9]);
//	(*temp).model_matrix = move((*temp), -10.15f, 41.71f, 84.99f) * rotate((*temp), 0.f, -90.62f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[10]);
//	(*temp).model_matrix = move((*temp), 10.2f, 41.71f, 80.1f) * rotate((*temp), 0.f, -90.62f, 0.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[11]);
//	(*temp).model_matrix = move((*temp), 122.6f, 23.1f, -143.3f) * rotate((*temp), -1.29f, -185.17f, 55.613f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	//beams
//	temp = &(*this->m_collidables_nodes[12]);
//	(*temp).model_matrix = move((*temp), 0.f, 0.f, -17.f) * rotate((*temp), 0.f, 0.f, 90.f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[13]);
//	(*temp).model_matrix = move((*temp), 2.37f, -0.05f, -2.3f) * rotate((*temp), 0.08f, 117.1f, -1.29f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[14]);
//	(*temp).model_matrix = move((*temp), -26.01f, -2.73f, 75.26f) * rotate((*temp), 1.03f, 208.34f, -93.05f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[15]);
//	(*temp).model_matrix = move((*temp), -5.81f, 18.f, -103.05f) * rotate((*temp), -0.571f, 27.475f, 3.146f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[16]);
//	(*temp).model_matrix = move((*temp), -7.5f, 17.92f, -106.26f) * rotate((*temp), -0.571f, 27.475f, 3.146f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	temp = &(*this->m_collidables_nodes[17]);
//	(*temp).model_matrix = move((*temp), -70.46635f, -6.039843f, 153.0274f) * rotate((*temp), 3.142f, 296.166f, -88.676f);
//	(*temp).m_aabb.center = glm::vec3((*temp).model_matrix * glm::vec4((*temp).m_aabb.center, 1.f));
//
//	//corridorLeft
//	temp = &(*this->m_collidables_nodes[18]);
//	(*temp).model_matrix = move((*temp), 5.15f, 10.f, 0.f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[19]);
//	(*temp).model_matrix = move((*temp), 26.66956f, 11.00362f, -55.64577f) * rotate((*temp), -3.248f, -87.23801f, -1.315f);
//
//	temp = &(*this->m_collidables_nodes[20]);
//	(*temp).model_matrix = move((*temp), 63.02254f, 6.612864f, -52.39686f) * rotate((*temp), -2.479f, -79.444f, -3.691f);
//
//	temp = &(*this->m_collidables_nodes[21]);
//	(*temp).model_matrix = move((*temp), 26.2f, 40.4f, 57.7f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[22]);
//	(*temp).model_matrix = move((*temp), 31.2f, 40.4f, 38.1f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[23]);
//	(*temp).model_matrix = move((*temp), 36.f, 40.4f, 18.8f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[24]);
//	(*temp).model_matrix = move((*temp), 41.f, 40.4f, -0.5f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[25]);
//	(*temp).model_matrix = move((*temp), 46.f, 40.4f, -20.3f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[26]);
//	(*temp).model_matrix = move((*temp), 50.7f, 40.4f, -40.5f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[27]);
//	(*temp).model_matrix = move((*temp), 101.4f, 37.7f, -125.42f) * rotate((*temp), 0.f, 0.f, -94.009f);
//
//	temp = &(*this->m_collidables_nodes[28]);
//	(*temp).model_matrix = move((*temp), 55.5f, 40.4f, -59.4f) * rotate((*temp), 0.f, 0.f, 0.f);
//
//	temp = &(*this->m_collidables_nodes[29]);
//	(*temp).model_matrix = move((*temp), 57.3f, 40.4f, -63.4f) * rotate((*temp), 0.028f, -4.045f, -0.401f);
//
//	temp = &(*this->m_collidables_nodes[30]);
//	(*temp).model_matrix = move((*temp), 37.06f, 27.58f, -181.44f) * rotate((*temp), 2.285f, 85.79301f, -100.632f);
//
//	temp = &(*this->m_collidables_nodes[31]);
//	(*temp).model_matrix = move((*temp), 16.4f, 23.2f, -181.9f) * rotate((*temp), 2.285f, 85.79301f, -100.632f);
//
//	temp = &(*this->m_collidables_nodes[32]);
//	(*temp).model_matrix = move((*temp), -4.639996f, 19.08f, -182.5f) * rotate((*temp), 2.285f, 85.79301f, -100.632f);
//
//	temp = &(*this->m_collidables_nodes[33]);
//	(*temp).model_matrix = move((*temp), -22.73f, 15.55f, -183.04f) * rotate((*temp), 2.285f, 85.79301f, -100.632f);
//
//	//corridorCurve
//	temp = &(*this->m_collidables_nodes[34]);
//	(*temp).model_matrix = move((*temp), -16.61559f, -39.46752f, -13.80444f) * rotate((*temp), 2.03f, -45.834f, -5.593f);
//	temp = &(*this->m_collidables_nodes[35]);
//	(*temp).model_matrix = move((*temp), -78.5f, -46.7f, -15.6f) * rotate((*temp), 0.386f, 135.534f, 5.006f);
//	temp = &(*this->m_collidables_nodes[36]);
//	(*temp).model_matrix = move((*temp), -8.1f, -52.16f, 5.94f) * rotate((*temp), 2.119f, 36.139f, 1.223f);
//	temp = &(*this->m_collidables_nodes[37]);
//	(*temp).model_matrix = move((*temp), -22.87997f, -46.81663f, 93.92767f) * rotate((*temp), -10.801f, -46.033f, -2.335f);
//	temp = &(*this->m_collidables_nodes[38]);
//	(*temp).model_matrix = move((*temp), -102.5f, -47.53f, 80.23f) * rotate((*temp), -2.052f, -134.55f, -1.542f);
//	temp = &(*this->m_collidables_nodes[39]);
//	(*temp).model_matrix = move((*temp), -128.1f, -45.4f, -54.2f) * rotate((*temp), 0.8980001f, 224.643f, -179.687f);
//	temp = &(*this->m_collidables_nodes[40]);
//	(*temp).model_matrix = move((*temp), -130.9f, -47.3f, -54.4f) * rotate((*temp), -0.183f, 135.875f, 4.172f);
//	temp = &(*this->m_collidables_nodes[41]);
//	(*temp).model_matrix = move((*temp), -8.1f, -52.16f, 5.94f) * rotate((*temp), 2.119f, 36.139f, 1.223f);
//	temp = &(*this->m_collidables_nodes[42]);
//	(*temp).model_matrix = move((*temp), -161.5f, -46.9f, 90.6f) * rotate((*temp), -1.592f, -45.489f, 0.255f);
//	temp = &(*this->m_collidables_nodes[43]);
//	(*temp).model_matrix = move((*temp), -198.4f, -30.9f, 85.4f) * rotate((*temp), -43.318f, -90.30501f, 264.59f);
//	temp = &(*this->m_collidables_nodes[44]);
//	(*temp).model_matrix = move((*temp), -218.06f, -46.15f, 80.79f) * rotate((*temp), -2.052f, -134.55f, -1.542f);
//	temp = &(*this->m_collidables_nodes[45]);
//	(*temp).model_matrix = move((*temp), -200.89f, -28.16f, 88.f) * rotate((*temp), 44.487f, 180.572f, 91.08801f);
//	temp = &(*this->m_collidables_nodes[46]);
//	(*temp).model_matrix = move((*temp), -198.62f, -14.31f, 105.83f) * rotate((*temp), -2.052f, -134.55f, -1.542f);
//	temp = &(*this->m_collidables_nodes[47]);
//	(*temp).model_matrix = move((*temp), -93.2f, -15.7f, 111.1f) * rotate((*temp), -2.949f, -45.386f, 4.013f);
//	temp = &(*this->m_collidables_nodes[48]);
//	(*temp).model_matrix = move((*temp), -18.04f, -18.91f, -71.06f) * rotate((*temp), -1.017f, -46.135f, -2.901f);
//	temp = &(*this->m_collidables_nodes[49]);
//	(*temp).model_matrix = move((*temp), -24.7f, -17.9f, -68.1f) * rotate((*temp), 1.942f, 135.589f, 4.079f);
//	temp = &(*this->m_collidables_nodes[50]);
//	(*temp).model_matrix = move((*temp), -18.7f, -27.3f, -140.5f) * rotate((*temp), 0.8980001f, 224.643f, -179.687f);
//	temp = &(*this->m_collidables_nodes[51]);
//	(*temp).model_matrix = move((*temp), -204.46f, -45.8f, -143.8f) * rotate((*temp), -0.183f, 135.875f, 4.172f);
//
//	//corridorStraight
//	temp = &(*this->m_collidables_nodes[52]);
//	(*temp).model_matrix = move((*temp), -3.6f, 10.f, -8.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[53]);
//	(*temp).model_matrix = move((*temp), -12.96f, 10.f, -66.19f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[54]);
//	(*temp).model_matrix = move((*temp), 6.3f, 10.f, -66.61f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[55]);
//	(*temp).model_matrix = move((*temp), 62.24687f, 15.37261f, -99.92525f) * rotate((*temp), 6.722f, -91.16701f, -14.02f);
//	temp = &(*this->m_collidables_nodes[56]);
//	(*temp).model_matrix = move((*temp), 92.9f, 5.31f, -59.47f) * rotate((*temp), -1.124f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[57]);
//	(*temp).model_matrix = move((*temp), 93.f, 5.7f, -40.1f) * rotate((*temp), 0.f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[58]);
//	(*temp).model_matrix = move((*temp), 93.1f, 6.1f, -20.7f) * rotate((*temp), 0.f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[59]);
//	(*temp).model_matrix = move((*temp), 92.9f, 6.4f, -7.3f) * rotate((*temp), 0.f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[60]);
//	(*temp).model_matrix = move((*temp), 35.4f, 9.07f, 9.53f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[61]);
//	(*temp).model_matrix = move((*temp), 12.11f, 9.059999f, 8.910005f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[62]);
//	(*temp).model_matrix = move((*temp), -12.96f, 10.f, -86.f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[63]);
//	(*temp).model_matrix = move((*temp), -12.96f, 10.f, -105.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[64]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -105.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[65]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -86.4f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[66]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -67.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[67]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -48.9f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[68]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -30.6f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[69]);
//	(*temp).model_matrix = move((*temp), -45.9f, 10.f, -11.4f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[70]);
//	(*temp).model_matrix = move((*temp), -102.72f, 10.53f, 9.41f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[71]);
//	(*temp).model_matrix = move((*temp), -83.f, 41.7f, 34.5f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[72]);
//	(*temp).model_matrix = move((*temp), -59.1f, 41.7f, 34.9f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[73]);
//	(*temp).model_matrix = move((*temp), 98.86f, 18.82f, -115.1f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[74]);
//	(*temp).model_matrix = move((*temp), 98.7f, 18.8f, -134.8f) * rotate((*temp), 0.f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[75]);
//	(*temp).model_matrix = move((*temp), 98.43f, 18.8f, -154.28f) * rotate((*temp), 0.f, 0.298f, -0.013f);
//	temp = &(*this->m_collidables_nodes[76]);
//	(*temp).model_matrix = move((*temp), 76.26f, 38.91f, -156.21f) * rotate((*temp), -6.277f, -89.51501f, -13.854f);
//	temp = &(*this->m_collidables_nodes[77]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -100.5f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[78]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -81.69999f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[79]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -44.19999f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[80]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -25.9f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[81]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -6.699987f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[82]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -175.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[83]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -156.9f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[84]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -138.6f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[85]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -119.4f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[86]);
//	(*temp).model_matrix = move((*temp), 32.2f, 27.6f, -228.9f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[87]);
//	(*temp).model_matrix = move((*temp), 56.1f, 27.6f, -228.5f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[88]);
//	(*temp).model_matrix = move((*temp), -119.4f, 11.18f, -194.2f) * rotate((*temp), 0.f, 0.f, 0.f);
//	temp = &(*this->m_collidables_nodes[89]);
//	(*temp).model_matrix = move((*temp), -102.59f, 11.51f, -231.7f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[90]);
//	(*temp).model_matrix = move((*temp), -78.47f, 11.51f, -231.7f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//	temp = &(*this->m_collidables_nodes[91]);
//	(*temp).model_matrix = move((*temp), -54.82f, 11.51f, -231.33f) * rotate((*temp), 0.f, -90.95901f, 1.093f);
//}

void Renderer::placeObject(bool &init, std::array<const char*, MAP_ASSETS::SIZE_ALL> &map_assets, MAP_ASSETS asset, glm::vec3 move, glm::vec3 rotate, glm::vec3 scale)
{
	OBJLoader loader;
	GeometricMesh* mesh;

	mesh = loader.load(map_assets[asset]);

	if (mesh != nullptr)
	{
		CollidableNode* node = new CollidableNode();
		node->Init(mesh);
		this->m_collidables_nodes.push_back(node);
		//CollidableNode* temp = &(*this->m_collidables_nodes[this->m_collidables_nodes.size - 1]);

		CollidableNode* temp = node;

		temp->model_matrix = Renderer::move(*temp, move) * Renderer::rotate(*temp, rotate) * Renderer::scale(*temp, scale);
		temp->m_aabb.center = glm::vec3(temp->model_matrix * glm::vec4(temp->m_aabb.center, 1.f));
		delete mesh;
	}
	else
	{
		init = false;
	}
}