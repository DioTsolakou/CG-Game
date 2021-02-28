#ifndef BIM_ENGINE_RENDERER_H
#define BIM_ENGINE_RENDERER_H

#include "GLEW\glew.h"
#include "glm\glm.hpp"
#include <vector>
#include "ShaderProgram.h"
#include "GeometryNode.h"
#include "CollidableNode.h"
#include "LightNode.h"

class Renderer
{
protected:
	int												m_screen_width, m_screen_height;
	glm::mat4										m_world_matrix;
	glm::mat4										m_view_matrix;
	glm::mat4										m_projection_matrix;
	glm::vec3										m_camera_position;
	glm::vec3										m_camera_target_position;
	glm::vec3										m_camera_up_vector;
	glm::vec2										m_camera_movement;
	glm::vec2										m_camera_look_angle_destination;
	bool											shoot_flag;
	bool											hit_flag;
	float											last_shoot = -2.f;
	float											last_hit = -2.f;
	
	float m_continous_time;
	float FOV;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	enum MAP_ASSETS
	{
		BEAM = 0,
		CH_BEAM,
		CANNON,
		CH_CANNON,
		CANNON_MOUNT,
		CH_CANNON_MOUNT,
		CORRIDOR_FORK,
		CH_CORRIDOR_FORK,
		CORRIDOR_STRAIGHT,
		CH_CORRIDOR_STRAIGHT,
		CORRIDOR_LEFT,
		CH_CORRIDOR_LEFT,
		CORRIDOR_RIGHT,
		CH_CORRIDOR_RIGHT,
		IRIS,
		CH_IRIS,
		PIPE,
		CH_PIPE,
		WALL,
		CH_WALL,
		CORRIDOR_CURVE,
		CH_CORRIDOR_CURVE,
		SIZE_ALL
	};

	bool InitShaders();
	bool InitGeometricMeshes();
	bool InitCommonItems();
	bool InitLights();
	bool InitIntermediateBuffers();
	void InitCamera();
	void RenderGeometry();
	void RenderDeferredShading();
	void RenderStaticGeometry();
	void RenderCollidableGeometry();
	void RenderShadowMaps();
	void RenderPostProcess();
	void PlaceObject(bool& init, std::array<const char*, MAP_ASSETS::SIZE_ALL>& map_assets, MAP_ASSETS asset, glm::vec3 move, glm::vec3 rotate, glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f));
	void ExtractPlanesFromFrustum(glm::mat4 MVP, bool normalize = false);

	std::vector<GeometryNode*> m_nodes;
	std::vector<CollidableNode*> m_collidables_nodes;
	glm::vec4 m_frustum_planes[6];

	LightNode									m_light;
	ShaderProgram								m_geometry_program;
	ShaderProgram								m_deferred_program;
	ShaderProgram								m_post_program;
	ShaderProgram								m_spot_light_shadow_map_program;

	GLuint m_fbo;
	GLuint m_vao_fbo;
	GLuint m_vbo_fbo_vertices;

	GLuint m_fbo_texture;

	GLuint m_fbo_depth_texture;
	GLuint m_fbo_pos_texture;
	GLuint m_fbo_normal_texture;
	GLuint m_fbo_albedo_texture;
	GLuint m_fbo_mask_texture;

public:

	Renderer();
	~Renderer();
	bool										Init(int SCREEN_WIDTH, int SCREEN_HEIGHT);
	void										Update(float dt);
	bool										ResizeBuffers(int SCREEN_WIDTH, int SCREEN_HEIGHT);
	void										UpdateGeometry(float dt);
	void										UpdateCamera(float dt);
	bool										ReloadShaders();
	void										Render();

	void										CameraMoveForward(bool enable);
	void										CameraMoveBackWard(bool enable);
	void										CameraMoveLeft(bool enable);
	void										CameraMoveRight(bool enable);
	void										CameraLook(glm::vec2 lookDir);
	void										CameraRollLeft(bool enable);
	void										CameraRollRight(bool enable);
	glm::mat4									Move(GeometryNode& object, glm::vec3 movement);
	glm::mat4									Rotate(GeometryNode& object, glm::vec3 rotation);
	glm::mat4									Scale(GeometryNode& object, glm::vec3 scale);
	void										BuildMap(bool &initialized, std::array<const char*, MAP_ASSETS::SIZE_ALL> mapAssets);
	void										CollisionDetection(glm::vec3& direction);
	bool										FrustumClipping(glm::mat4 MVP, GeometryNode& node);
	float										CalculateDistance(glm::vec3 u, glm::vec3);
	void										Shoot(bool shoot);
	void										Zoom(bool zoom);
};

#endif
