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
	this->m_collidables_nodes = {};
	this->m_continous_time = 0.0;
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_fbo_depth_texture);
	glDeleteTextures(1, &m_fbo_pos_texture);
	glDeleteTextures(1, &m_fbo_normal_texture);
	glDeleteTextures(1, &m_fbo_albedo_texture);
	glDeleteTextures(1, &m_fbo_mask_texture);

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

	this->InitCamera();

	//If everything initialized
	return techniques_initialization && meshes_initialization &&
		common_initialization && inter_buffers_initialization;
}

void Renderer::InitCamera()
{
	this->FOV = 90.f;
	this->aspectRatio = this->m_screen_width / (float)this->m_screen_height;
	this->nearPlane = 0.1f;
	this->farPlane = 200.f;

	this->m_camera_position = glm::vec3(0, 0, -1.5);
	this->m_camera_target_position = glm::vec3(0, 0, -2.5);
	this->m_camera_up_vector = glm::vec3(0, 1, 0);

	this->m_view_matrix = glm::lookAt(this->m_camera_position, this->m_camera_target_position, m_camera_up_vector);

	this->m_projection_matrix = glm::perspective(glm::radians(FOV), aspectRatio, nearPlane, farPlane);
}

bool Renderer::InitLights()
{
	this->m_light.SetColor(glm::vec3(108.f, 86.f, 64.f));
	this->m_light.SetPosition(this->m_camera_position);
	this->m_light.SetTarget(this->m_camera_target_position);
	this->m_light.SetConeSize(40, 50);
	this->m_light.CastShadow(false);

	return true;
}

bool Renderer::InitShaders()
{
	std::string vertex_shader_path = "Assets/Shaders/geometry pass.vert";
	std::string geometry_shader_path = "Assets/Shaders/geometry pass.geom";
	std::string fragment_shader_path = "Assets/Shaders/geometry pass.frag";

	m_geometry_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_geometry_program.LoadGeometryShaderFromFile(geometry_shader_path.c_str());
	m_geometry_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	m_geometry_program.CreateProgram();

	vertex_shader_path = "Assets/Shaders/deferred pass.vert";
	fragment_shader_path = "Assets/Shaders/deferred pass.frag";

	m_deferred_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_deferred_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	m_deferred_program.CreateProgram();

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
	glGenTextures(1, &m_fbo_depth_texture);
	glGenTextures(1, &m_fbo_pos_texture);
	glGenTextures(1, &m_fbo_normal_texture);
	glGenTextures(1, &m_fbo_albedo_texture);
	glGenTextures(1, &m_fbo_mask_texture);
	glGenTextures(1, &m_fbo_texture);

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

	glBindTexture(GL_TEXTURE_2D, m_fbo_pos_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, m_fbo_normal_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, m_fbo_albedo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, m_fbo_mask_texture);
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

	// framebuffer to link to everything together
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_pos_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_fbo_normal_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_fbo_albedo_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_fbo_mask_texture, 0);
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
		"Assets/Beam/CH-Beam.obj",

		//"Assets/Cannon/Cannon.obj",
		//"Assets/Cannon/CH-Cannon.obj",

		//"Assets/Cannon/CannonMount.obj",
		//"Assets/Cannon/CH-CannonMount.obj",


		"Assets/Corridor/Corridor_Fork.obj",
		"Assets/Corridor/CH-Corridor_Fork.obj",

		"Assets/Corridor/Corridor_Straight.obj",
		"Assets/Corridor/CH-Corridor_Straight.obj",

		"Assets/Corridor/Corridor_Left.obj",
		"Assets/Corridor/CH-Corridor_Left.obj",

		"Assets/Corridor/Corridor_Right.obj",
		"Assets/Corridor/CH-Corridor_Right.obj",
		
		//"Assets/Iris/Iris.obj",
		//"Assets/Iris/CH-Iris.obj",

		"Assets/Pipe/Pipe.obj",
		"Assets/Pipe/CH-Pipe.obj",

		"Assets/Wall/Wall.obj",
		"Assets/Wall/CH-Wall.obj",

		"Assets/Corridor/Corridor_Curve.obj",
	};

	bool initialized = true;

	buildMap(initialized, mapAssets);
	std::cout << "geometry nodes legnth = " << this->m_nodes.size() << std::endl;
	std::cout << "collidable nodes legnth = " << this->m_collidables_nodes.size() << std::endl;

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
	for (auto& node : this->m_nodes)
	{
		node->app_model_matrix = node->model_matrix;
	}
	for (auto& node : this->m_collidables_nodes)
	{
		node->app_model_matrix = node->model_matrix;
	}
}

void Renderer::UpdateCamera(float dt)
{
	glm::vec3 direction = glm::normalize(m_camera_target_position - m_camera_position);
	std::vector<float> isectDst;

	float distance = 12.5;
	for (auto& node : this->m_collidables_nodes)
	{
		float_t isectT = 0.f;
		int32_t primID = -1;
		isectDst.clear();
		isectDst = node->calculateCameraCollision(m_camera_position, direction, m_world_matrix, isectT, primID);
		for (auto it = isectDst.begin(); it != isectDst.end(); ++it) {
			if (*it < distance) {
				/*int pos = it - isectDst.begin();
				if (pos >= 1 && pos <= 3) m_camera_movement.x = (m_camera_movement.x < 0) ? 0 : m_camera_movement.x;
				if (pos >= 5 && pos <= 7) m_camera_movement.x = (m_camera_movement.x > 0) ? 0 : m_camera_movement.x;
				if (pos >= 3 && pos <= 5) m_camera_movement.z = (m_camera_movement.z > 0) ? 0 : m_camera_movement.z;
				if (pos == 0 || pos == 1 || pos == 7) m_camera_movement.z = (m_camera_movement.z < 0) ? 0 : m_camera_movement.z;*/
				//break;
			}
		}
	}

	m_camera_position = m_camera_position + (m_camera_movement.x * 5.f * dt) * direction;
	m_camera_target_position = m_camera_target_position + (m_camera_movement.x * 5.f * dt) * direction;

	glm::vec3 right = glm::normalize(glm::cross(direction, m_camera_up_vector));

	m_camera_position = m_camera_position + (m_camera_movement.y * 5.f * dt) * right;
	m_camera_target_position = m_camera_target_position + (m_camera_movement.y * 5.f * dt) * right;

	float speed = glm::pi<float>() * 0.002f;
	glm::mat4 rotation = glm::rotate(glm::mat4(1.f), m_camera_look_angle_destination.y * speed, right);
	rotation *= glm::rotate(glm::mat4(1.f), m_camera_look_angle_destination.x * speed, m_camera_up_vector);
	m_camera_look_angle_destination = glm::vec2(0.f);

	direction = rotation * glm::vec4(direction, 0.f);
	m_camera_target_position = m_camera_position + direction * glm::distance(m_camera_position, m_camera_target_position);

	GeometryNode* temp = this->m_nodes.at(this->m_nodes.size() - 1);
	temp->model_matrix = Renderer::move(*temp, m_camera_target_position - m_camera_position); // glm::vec3((m_camera_movement.x * 5.f * dt) * direction, (m_camera_movement.y * 5.f * dt) * right, 0.f));
	temp->m_aabb.center = glm::vec3(temp->model_matrix * glm::vec4(temp->m_aabb.center, 1.f));

	m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

	//std::cout << m_camera_position.x << " " << m_camera_position.y << " " << m_camera_position.z << " " << std::endl;
	std::cout << m_camera_target_position.x << " " << m_camera_target_position.y << " " << m_camera_target_position.z << " " << std::endl;
	//std::cout << m_light.GetTarget().x << " " << m_light.GetTarget().y << " " << m_light.GetTarget().z << " " << std::endl;
	this->m_light.SetPosition(m_camera_position);
	this->m_light.SetTarget(m_camera_target_position);
}

bool Renderer::ReloadShaders()
{
	m_geometry_program.ReloadProgram();
	m_post_program.ReloadProgram();
	m_deferred_program.ReloadProgram();
	m_spot_light_shadow_map_program.ReloadProgram();
	return true;
}

void Renderer::Render()
{
	RenderShadowMaps();
	RenderGeometry();
	RenderDeferredShading();
	RenderPostProcess();

	GLenum error = Tools::CheckGLError();

	if (error != GL_NO_ERROR)
	{
		printf("Renderer:Draw GL Error\n");
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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	m_post_program.loadInt("uniform_texture", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_light.GetShadowMapDepthTexture());
	m_post_program.loadInt("uniform_shadow_map", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_fbo_pos_texture);
	m_post_program.loadInt("uniform_tex_pos", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_fbo_normal_texture);
	m_post_program.loadInt("uniform_tex_normal", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_fbo_albedo_texture);
	m_post_program.loadInt("uniform_tex_albedo", 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, m_fbo_mask_texture);
	m_post_program.loadInt("uniform_tex_mask", 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
	m_post_program.loadInt("uniform_tex_depth", 6);

	glBindVertexArray(m_vao_fbo);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_post_program.Unbind();
}

void Renderer::RenderStaticGeometry()
{
	glm::mat4 proj = m_projection_matrix * m_view_matrix * m_world_matrix;

	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

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
			m_geometry_program.loadInt("uniform_has_tex_emissive", (node->parts[j].emissive_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_mask", (node->parts[j].mask_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_normal", (node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_is_tex_bumb", (node->parts[j].bump_textureID > 0) ? 1 : 0);

			glActiveTexture(GL_TEXTURE0);
			m_geometry_program.loadInt("uniform_tex_diffuse", 0);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].diffuse_textureID);

			if (node->parts[j].mask_textureID > 0)
			{
				glActiveTexture(GL_TEXTURE1);
				m_geometry_program.loadInt("uniform_tex_mask", 1);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].mask_textureID);
			}

			if ((node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0))
			{
				glActiveTexture(GL_TEXTURE2);
				m_geometry_program.loadInt("uniform_tex_normal", 2);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].bump_textureID > 0 ?
					node->parts[j].bump_textureID : node->parts[j].normal_textureID);
			}

			if (node->parts[j].emissive_textureID > 0)
			{
				glActiveTexture(GL_TEXTURE3);
				m_geometry_program.loadInt("uniform_tex_emissive", 3);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].emissive_textureID);
			}

			glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
		}

		glBindVertexArray(0);
	}

	//glDisable(GL_BLEND);
}

bool check = true;
int j = 0;
void Renderer::RenderCollidableGeometry()
{
	glm::mat4 proj = m_projection_matrix * m_view_matrix * m_world_matrix;

	glm::vec3 camera_dir = normalize(m_camera_target_position - m_camera_position);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_ONE);

	int i = 0;
	for (auto& node : this->m_collidables_nodes)
	{
		glEnable(GL_BLEND);
		if (std::find(this->m_curve_positions.begin(), this->m_curve_positions.end(), i) != this->m_curve_positions.end())
			glDisable(GL_BLEND);
		float_t isectT = 0.f;
		int32_t primID = -1;
		int32_t totalRenderedPrims = 0;

		if (check) {
			if (node->intersectRays(m_camera_position, camera_dir, m_world_matrix, isectT, primID)) {
				if (isectT > 0.f) {
					check = false;
					collisionDetection(node);
					//continue;
				}
			}		
		}
		j++;
		if (j == this->m_collidables_nodes.size()) {check = true; j = 0;}

		//if (node - this->m_collidables_nodes.begin() == this->m_collidables_nodes.end() - 1) check = true;
		//node->intersectRay(m_camera_position, camera_dir, m_world_matrix, isectT, primID);

		glBindVertexArray(node->m_vao);

		m_geometry_program.loadMat4("uniform_projection_matrix", proj * node->app_model_matrix);
		m_geometry_program.loadMat4("uniform_normal_matrix", glm::transpose(glm::inverse(m_world_matrix * node->app_model_matrix)));
		m_geometry_program.loadMat4("uniform_world_matrix", m_world_matrix * node->app_model_matrix);
		m_geometry_program.loadFloat("uniform_time", m_continous_time);

		
		for (int j = 0; j < node->parts.size(); ++j)
		{
			m_geometry_program.loadVec3("uniform_diffuse", node->parts[j].diffuse);
			m_geometry_program.loadVec3("uniform_ambient", node->parts[j].ambient);
			m_geometry_program.loadVec3("uniform_specular", node->parts[j].specular);
			m_geometry_program.loadFloat("uniform_shininess", node->parts[j].shininess);
			m_geometry_program.loadInt("uniform_has_tex_diffuse", (node->parts[j].diffuse_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_mask", (node->parts[j].mask_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_emissive", (node->parts[j].emissive_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_has_tex_normal", (node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_is_tex_bumb", (node->parts[j].bump_textureID > 0) ? 1 : 0);
			m_geometry_program.loadInt("uniform_prim_id", primID - totalRenderedPrims);

			glActiveTexture(GL_TEXTURE0);
			m_geometry_program.loadInt("uniform_tex_diffuse", 0);
			glBindTexture(GL_TEXTURE_2D, node->parts[j].diffuse_textureID);

			if (node->parts[j].mask_textureID > 0)
			{
				glActiveTexture(GL_TEXTURE1);
				m_geometry_program.loadInt("uniform_tex_mask", 1);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].mask_textureID);
			}

			if ((node->parts[j].bump_textureID > 0 || node->parts[j].normal_textureID > 0))
			{
				glActiveTexture(GL_TEXTURE2);
				m_geometry_program.loadInt("uniform_tex_normal", 2);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].bump_textureID > 0 ?
					node->parts[j].bump_textureID : node->parts[j].normal_textureID);
			}

			if (node->parts[j].emissive_textureID > 0)
			{
				glActiveTexture(GL_TEXTURE3);
				m_geometry_program.loadInt("uniform_tex_emissive", 3);
				glBindTexture(GL_TEXTURE_2D, node->parts[j].emissive_textureID);
			}

			glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
			totalRenderedPrims += node->parts[j].count;
			
		}

		glBindVertexArray(0);

		/*if (std::find(this->m_curve_positions.begin(), this->m_curve_positions.end(), i) != this->m_curve_positions.end())
			glEnable(GL_BLEND);*/
		i++;
	}

	glDisable(GL_BLEND);
}

void Renderer::RenderDeferredShading()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);

	GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };

	glDrawBuffers(1, drawbuffers);

	glViewport(0, 0, m_screen_width, m_screen_height);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glClear(GL_COLOR_BUFFER_BIT);

	m_deferred_program.Bind();

	m_deferred_program.loadVec3("uniform_light_color", m_light.GetColor());
	m_deferred_program.loadVec3("uniform_light_dir", m_light.GetDirection());
	m_deferred_program.loadVec3("uniform_light_pos", m_light.GetPosition());

	m_deferred_program.loadFloat("uniform_light_umbra", m_light.GetUmbra());
	m_deferred_program.loadFloat("uniform_light_penumbra", m_light.GetPenumbra());

	m_deferred_program.loadVec3("uniform_camera_pos", m_camera_position);
	m_deferred_program.loadVec3("uniform_camera_dir", normalize(m_camera_target_position - m_camera_position));

	m_deferred_program.loadMat4("uniform_light_projection_view", m_light.GetProjectionMatrix() * m_light.GetViewMatrix());
	m_deferred_program.loadInt("uniform_cast_shadows", m_light.GetCastShadowsStatus() ? 1 : 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo_pos_texture);
	m_deferred_program.loadInt("uniform_tex_pos", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_fbo_normal_texture);
	m_deferred_program.loadInt("uniform_tex_normal", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_fbo_albedo_texture);
	m_deferred_program.loadInt("uniform_tex_albedo", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_fbo_mask_texture);
	m_deferred_program.loadInt("uniform_tex_mask", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
	m_deferred_program.loadInt("uniform_tex_depth", 4);

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, m_light.GetShadowMapDepthTexture());
	m_deferred_program.loadInt("uniform_shadow_map", 10);

	glBindVertexArray(m_vao_fbo);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	m_deferred_program.Unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthMask(GL_TRUE);
}

void Renderer::RenderGeometry()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_pos_texture, 0);

	GLenum drawbuffers[4] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3 };

	glDrawBuffers(4, drawbuffers);

	glViewport(0, 0, m_screen_width, m_screen_height);
	glClearColor(0.f, 0.8f, 1.f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_geometry_program.Bind();
	RenderStaticGeometry();
	RenderCollidableGeometry();

	m_geometry_program.Unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
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
		m_spot_light_shadow_map_program.Bind();

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
		int32_t primID;

		for (auto& node : this->m_collidables_nodes)
		{
			//node->intersectRay(m_camera_position, camera_dir, m_world_matrix, isectT, primID);

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

glm::mat4 Renderer::move(GeometryNode& object, glm::vec3 movement)
{
	object.model_matrix = glm::translate(glm::mat4(1.f), movement);
	return object.model_matrix;
}

glm::mat4 Renderer::rotate(GeometryNode& object, glm::vec3 rotation)
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

glm::mat4 Renderer::scale(GeometryNode& object, glm::vec3 scale)
{
	object.model_matrix = glm::scale(glm::mat4(1.f), scale);
	return object.model_matrix;
}

void Renderer::placeObject(bool &init, std::array<const char*, MAP_ASSETS::SIZE_ALL> &map_assets, MAP_ASSETS asset, glm::vec3 move, glm::vec3 rotate, glm::vec3 scale)
{
	OBJLoader loader;
	GeometricMesh* mesh;

	mesh = loader.load(map_assets[asset]);

	if (mesh != nullptr)
	{
		GeometryNode* temp;
		if (asset % 2 == 0 && asset != CORRIDOR_CURVE)
		{
			GeometryNode* node = new GeometryNode();
			node->Init(mesh);
			this->m_nodes.push_back(node);
			temp = node;

			int nextAsset = static_cast<int>(asset);
			nextAsset++;
			placeObject(init, map_assets, static_cast<MAP_ASSETS>(nextAsset), move, rotate, scale);
		}
		else
		{
			CollidableNode* node = new CollidableNode();
			node->Init(mesh);
			this->m_collidables_nodes.push_back(node);
			if (asset == CORRIDOR_CURVE) this->m_curve_positions.push_back(this->m_collidables_nodes.size() - 1);
			node->SetType(asset);
			temp = node;
		}
		temp->model_matrix = Renderer::move(*temp, move) * Renderer::rotate(*temp, rotate) * Renderer::scale(*temp, scale);
		temp->m_aabb.center = glm::vec3(temp->model_matrix * glm::vec4(temp->m_aabb.center, 1.f));
		delete mesh;
	}
	else
	{
		init = false;
	}
}

void Renderer::buildMap(bool &initialized, std::array<const char*, MAP_ASSETS::SIZE_ALL> mapAssets)
{
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
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(32.25f, 0.f, -51.75f), glm::vec3(180.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(44.f, 0.f, -74.0f), glm::vec3(0.f, 0.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(71.f, 0.f, -81.f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(91.f, 0.f, -81.f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(111.f, 0.f, -81.f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(126.25f, 0.f, -85.75f), glm::vec3(180.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(126.25f, 0.f, -107.25f), glm::vec3(180.f, 180.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(111.f, 0.f, -114.25f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(91.f, 0.f, -114.25f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(71.f, 0.f, -114.25f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(51.f, 0.f, -114.25f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_STRAIGHT, glm::vec3(31.f, 0.f, -114.25f), glm::vec3(0.f, -90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_CURVE, glm::vec3(4.f, 0.f, -119.f), glm::vec3(0.f, 90.f, 0.f));
	this->placeObject(initialized, mapAssets, CORRIDOR_FORK, glm::vec3(-0.75f, 0.f, -141.25f), glm::vec3(0.f, 180.f, 0.f));

	//camera_target
	//this->placeObject(initialized, mapAssets, BEAM, m_camera_target_position, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.5f, 0.5f, 0.5f));

	this->m_world_matrix = glm::mat4(1.f);
}

void Renderer::collisionDetection(CollidableNode* node)
{
	switch (node->GetType()) 
	{
		case MAP_ASSETS::CH_WALL:
			std::cout << "we hit a wall" << std::endl;
			break;

		case MAP_ASSETS::CH_BEAM:
			std::cout << "we hit a beam" << std::endl;
			break;

		default:
			std::cout << "we hit something else" << std::endl;
			break;
	}
}