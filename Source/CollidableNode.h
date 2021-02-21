#pragma once

#include "GeometryNode.h"

class CollidableNode : public GeometryNode
{
public:

    CollidableNode(void);
    ~CollidableNode(void);

    CollidableNode(const CollidableNode&) = default;
    CollidableNode(CollidableNode&&) = default;
    CollidableNode& operator=(const CollidableNode&) = default;
    CollidableNode& operator=(CollidableNode&&) = default;

    void Init(GeometricMesh* mesh) override;
    //bool intersectRays(const glm::vec3& pOrigin, const glm::vec3& pDir, const glm::mat4& pWorldMatrix, float& pIsectDist, int32_t& pPrimID, float pTmax = 1.e+15f, float pTmin = 0.f);
    std::vector<float> calculateCameraCollision(const glm::vec3& pOrigin, const glm::vec3& pDir, const glm::mat4& pWorldMatrix, float& pIsectDist, int32_t& pPrimID, float pTmax = 1.e+15f, float pTmin = 0.f);
    bool intersectRay(const glm::vec3& pOrigin, const glm::vec3& pDir, const glm::mat4& pWorldMatrix, float& pIsectDist, int32_t& pPrimID, float pTmax = 1.e+15f, float pTmin = 0.f, float angleX = 0.f, float angleY = 0.f);

protected:

    // Empty

private:

    typedef GeometryNode super;

    struct triangle { glm::vec3 v0, v1, v2; };
    std::vector<float> intersectionDistance;

    std::vector<triangle> triangles;
    //bool intersectRay(const glm::vec3& pOrigin, const glm::vec3& pDir, const glm::mat4& pWorldMatrix, float& pIsectDist, int32_t& pPrimID, float pTmax = 1.e+15f, float pTmin = 0.f, float angleX = 0.f, float angleY = 0.f);
};