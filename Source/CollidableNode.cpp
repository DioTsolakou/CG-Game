#include "GeometricMesh.h"
#include "CollidableNode.h"
#include "glm/gtx/intersect.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>

CollidableNode::CollidableNode(void) :
    GeometryNode()
{ /* Empty */
}

CollidableNode::~CollidableNode(void) { /* Empty */ }

void CollidableNode::Init(class GeometricMesh* mesh)
{
    this->triangles.resize(mesh->vertices.size() / 3);

    for (size_t t = 0; t < this->triangles.size(); ++t)
    {
        this->triangles[t].v0 = mesh->vertices[t * 3 + 0];
        this->triangles[t].v1 = mesh->vertices[t * 3 + 1];
        this->triangles[t].v2 = mesh->vertices[t * 3 + 2];
    }

    super::Init(mesh);
}

//private
bool CollidableNode::intersectRay(
    const glm::vec3& pOrigin_wcs,
    const glm::vec3& pDir_wcs,
    const glm::mat4& pWorldMatrix,
    float& pIsectDist,
    int32_t& pPrimID,
    float pTmax,
    float pTmin,
    float angleX,
    float angleY)
{
    if (pTmax < pTmin || glm::length(pDir_wcs) < glm::epsilon<float>()) return false;

    glm::vec3 normDir = pDir_wcs;// glm::normalize(pDir_wcs);

    glm::mat4 rotationX = glm::rotate(glm::mat4(1.f), glm::radians(angleX), glm::vec3(1.f, 0.f, 0.f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.f), glm::radians(angleY), glm::vec3(0.f, 1.f, 0.f));
    normDir = rotationX * rotationY * glm::vec4(normDir, 0.f);
    const glm::mat4 wordToModel = glm::inverse(pWorldMatrix * super::app_model_matrix);
    const glm::vec3 o_local = wordToModel * glm::vec4(pOrigin_wcs, 1.f);
    const glm::vec3 d_local = glm::normalize(glm::vec3(wordToModel * glm::vec4(normDir, 0.f)));

    pIsectDist = pTmax;
    float_t curMin = pTmax;
    bool found_isect = false;
    glm::vec3 isect(1.f);
    pPrimID = -1;

    for (uint32_t i = 0; i < this->triangles.size(); ++i)
    {
        auto& tr = this->triangles[i];
        glm::vec3 barycoord;

        if (glm::intersectRayTriangle(o_local, d_local, tr.v0, tr.v1, tr.v2, barycoord))
        {
            const glm::vec3 tmp_isect = tr.v0 * barycoord.x + tr.v1 * barycoord.y + tr.v2 * barycoord.z;
            float dist = glm::distance(o_local, tmp_isect);

            if (dist < curMin && dist >= pTmin)
            {
                curMin = dist;
                found_isect = true;
                isect = tmp_isect;
                pPrimID = i;
            }
        }
    }

    if (found_isect)
    {
        glm::vec3 isec_wcs = pWorldMatrix * super::app_model_matrix * glm::vec4(isect, 1.f);
        pIsectDist = glm::distance(pOrigin_wcs, isec_wcs);
    }

    return found_isect;
}

std::vector<float> CollidableNode::calculateCameraCollision(const glm::vec3& pOrigin, const glm::vec3& pDir, const glm::mat4& pWorldMatrix, float& pIsectDist, int32_t& pPrimID, float pTmax, float pTmin) 
{
    intersectionDistance.clear();
    int numOfRays = 8;
    for (int i = 0; i < numOfRays; i++) {
        if (this->intersectRay(pOrigin, pDir, pWorldMatrix, pIsectDist, pPrimID, pTmax, pTmin, 0.f, (float)i * 360 / numOfRays))
            intersectionDistance.push_back(pIsectDist);
        else 
            intersectionDistance.push_back(999.f);
    }
    return intersectionDistance;
}