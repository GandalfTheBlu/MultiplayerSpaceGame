//------------------------------------------------------------------------------
//  @file physics.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "physics.h"
#include "core/idpool.h"
#include "render/gltf.h"
#include "debugrender.h"
#include "core/random.h"
#include "core/cvar.h"
#include <iostream>
namespace Physics
{

struct AABB
{
    glm::vec3 min = glm::vec3(1e30f);
    glm::vec3 max = glm::vec3(-1e30f);

    void Grow(glm::vec3 const& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    float Area()
    {
        glm::vec3 extent = max - min; // box extent
        return extent.x * extent.y + extent.y * extent.z + extent.z * extent.x;
    }
};

struct Bin { AABB bounds; int count = 0; };

static const uint N_OBJECTS = 1000;

static glm::vec3 objects[N_OBJECTS];
static AABB bboxes[N_OBJECTS];

struct BVHNode
{
    AABB bbox;
    uint index = -1; // left node, or index to first child if count is zero
    uint count = 0; // number of children
};

struct BVH
{
    BVHNode* nodes = nullptr;
    uint* bboxIndex = nullptr;
    uint rootNodeIndex = 0;
    uint nodesUsed = 0;
};

void UpdateNodeBounds(BVH* bvh, BVHNode* node);
void Subdivide(BVH* bvh, BVHNode* node);

BVH* BuildBVH(uint numObjects)
{
    auto start = std::chrono::high_resolution_clock::now();
    
    BVH* bvh = new BVH();
    bvh->nodes = new BVHNode[numObjects * 2 - 1];
    bvh->nodesUsed = 1;
    bvh->bboxIndex = new uint[numObjects];
    for (uint i = 0; i < numObjects; i++)
        bvh->bboxIndex[i] = i;

    BVHNode& root = bvh->nodes[bvh->rootNodeIndex];
    root.index = 0;
    root.count = numObjects;
    UpdateNodeBounds(bvh, &root);
    // subdivide recursively
    Subdivide(bvh, &root);

    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = stop - start;
 
    std::cout << "buildbvh: " << duration.count() << std::endl;

    return bvh;
}

void UpdateNodeBounds(BVH* bvh, BVHNode* node)
{
    node->bbox.min = glm::vec3(1e30f);
    node->bbox.max = glm::vec3(-1e30f);
    uint end = node->index + node->count;
    for (uint i = node->index; i < end; i++)
    {
        uint index = bvh->bboxIndex[i];
        AABB& leafBBox = bboxes[index];
        node->bbox.min = glm::min(node->bbox.min, leafBBox.min);
        node->bbox.max = glm::max(node->bbox.max, leafBBox.max);
    }
}

// surface area heuristic
float EvaluateSAH(BVH* bvh, BVHNode* node, int axis, float pos)
{
    // determine triangle counts and bounds for this split candidate
    AABB leftBox, rightBox;
    int leftCount = 0, rightCount = 0;
    for (uint i = 0; i < node->count; i++)
    {
        AABB const& bbox = bboxes[bvh->bboxIndex[node->index + i]];
        float center = (bbox.max[axis] + bbox.min[axis]) * 0.5f;
        if (center < pos)
        {
            leftCount++;
            leftBox.Grow(bbox.min);
            leftBox.Grow(bbox.max);
        }
        else
        {
            rightCount++;
            rightBox.Grow(bbox.min);
            rightBox.Grow(bbox.max);
        }
    }
    float cost = leftCount * leftBox.Area() + rightCount * rightBox.Area();
    return cost > 0 ? cost : 1e30f;
}

float FindBestSplitPlane(BVH* bvh, BVHNode* node, int& axis, float& splitPos)
{
    constexpr int intervals = 8;
    float bestCost = 1e30f;
    for (int a = 0; a < 3; a++)
    {
        float boundsMin = 1e30f, boundsMax = -1e30f;
        for (uint i = 0; i < node->count; i++)
        {
            AABB const& bbox = bboxes[bvh->bboxIndex[node->index + i]];
            float center = (bbox.max[a] + bbox.min[a]) * 0.5f;
            boundsMin = glm::min(boundsMin, center);
            boundsMax = glm::max(boundsMax, center);
        }
        if (boundsMin == boundsMax) continue;
        // populate the bins
        Bin bin[intervals];
        float scale = (float)intervals / (boundsMax - boundsMin);
        for (uint i = 0; i < node->count; i++)
        {
            AABB const& bbox = bboxes[bvh->bboxIndex[node->index + i]];
            float center = (bbox.max[a] + bbox.min[a]) * 0.5f;
            int binIdx = glm::min(intervals - 1, (int)((center - boundsMin) * scale));
            bin[binIdx].count++;
            bin[binIdx].bounds.Grow(bbox.min);
            bin[binIdx].bounds.Grow(bbox.max);
        }
        // gather data for the 7 planes between the 8 bins
        float leftArea[intervals - 1], rightArea[intervals - 1];
        int leftCount[intervals - 1], rightCount[intervals - 1];
        AABB leftBox, rightBox;
        int leftSum = 0, rightSum = 0;
        for (int i = 0; i < intervals - 1; i++)
        {
            leftSum += bin[i].count;
            leftCount[i] = leftSum;
            leftBox.Grow(bin[i].bounds.min);
            leftBox.Grow(bin[i].bounds.max);
            leftArea[i] = leftBox.Area();
            rightSum += bin[intervals - 1 - i].count;
            rightCount[intervals - 2 - i] = rightSum;
            rightBox.Grow(bin[intervals - 1 - i].bounds.min);
            rightBox.Grow(bin[intervals - 1 - i].bounds.max);
            rightArea[intervals - 2 - i] = rightBox.Area();
        }
        // calculate SAH cost for the 7 planes
        scale = (boundsMax - boundsMin) / intervals;
        for (int i = 0; i < intervals - 1; i++)
        {
            float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (planeCost < bestCost)
                axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
        }
    }
    return bestCost;
}

float CalculateNodeCost(BVHNode* node)
{
    return node->count * node->bbox.Area();
}

void Subdivide(BVH* bvh, BVHNode* node)
{
    if (node->count <= 2) return;

    // calculate splitting plane
    //glm::vec3 extent = node->bbox.max - node->bbox.min;
    //int axis = 0;
    //if (extent.y > extent.x) axis = 1;
    //if (extent.z > extent[axis]) axis = 2;
    //float splitPos = node->bbox.min[axis] + (extent[axis] * 0.5f);

    int axis;
    float splitPos;
    float splitCost = FindBestSplitPlane(bvh, node, axis, splitPos);
    float nosplitCost = CalculateNodeCost(node);
    if (splitCost >= nosplitCost) return;


    // split group into two halves
    // just swap elements to be to the left or right of a split in the aabb array
    int i = node->index;
    int j = i + node->count - 1;
    while (i <= j)
    {
        const uint idx = bvh->bboxIndex[i];
        float center = (bboxes[idx].min[axis] + bboxes[idx].max[axis]) * 0.5f;
        if (center < splitPos)
            i++;
        else
            std::swap(bvh->bboxIndex[i], bvh->bboxIndex[j--]);
    }

    int leftCount = i - node->index;
    if (leftCount == 0 || leftCount == node->count) return;
    // create child nodes
    int leftChildIdx = bvh->nodesUsed++;
    int rightChildIdx = bvh->nodesUsed++;
    bvh->nodes[leftChildIdx].index = node->index;
    bvh->nodes[leftChildIdx].count = leftCount;
    bvh->nodes[rightChildIdx].index = i;
    bvh->nodes[rightChildIdx].count = node->count - leftCount;
    node->index = leftChildIdx;
    node->count = 0;
    UpdateNodeBounds(bvh, bvh->nodes + leftChildIdx);
    UpdateNodeBounds(bvh, bvh->nodes + rightChildIdx);
    Subdivide(bvh, bvh->nodes + leftChildIdx);
    Subdivide(bvh, bvh->nodes + rightChildIdx);
}

BVH* bvh;
void SetupBVH()
{
    Core::CVar* debug_bvh_mode = Core::CVarCreate(Core::CVarType::CVar_Int, "debug_bvh_mode", "3");
    Core::CVar* debug_bvh_maxdepth = Core::CVarCreate(Core::CVarType::CVar_Int, "debug_bvh_maxdepth", "60");
    Core::CVar* debug_bvh_node_index = Core::CVarCreate(Core::CVarType::CVar_Int, "debug_bvh_node_index", "0");

    for (size_t i = 0; i < N_OBJECTS; i++)
    {
        const float span = 5.0f;
        objects[i] = glm::vec3(Core::RandomFloatNTP() * span, Core::RandomFloatNTP() * span, Core::RandomFloatNTP() * span);
        const float maxSize = 0.2f;
        const float minSize = 0.02f;
        glm::vec3 halfExtents = glm::vec3((minSize + Core::RandomFloat() * maxSize), (minSize + Core::RandomFloat() * maxSize), (minSize + Core::RandomFloat() * maxSize));
        bboxes[i] = { objects[i] - halfExtents, objects[i] + halfExtents };
    }
    
    bvh = BuildBVH(N_OBJECTS);
}

void DrawBVH(BVHNode* node, int depth, int maxDepth)
{
    if (depth == maxDepth) return;

    glm::vec3 center = (node->bbox.max + node->bbox.min) / 2.0f;
    Debug::DrawBox(glm::translate(center) * glm::scale(node->bbox.max - node->bbox.min), glm::vec4(glm::vec3(1), 1), Debug::RenderMode::WireFrame);

    if (node->count > 0)
    {
        // leaf node
    }
    else
    {
        DrawBVH(bvh->nodes + node->index, depth + 1, maxDepth);
        DrawBVH(bvh->nodes + node->index + 1, depth + 1, maxDepth);
    }
}

void VisualizeBVH()
{
    Core::CVar* debug_bvh_mode = Core::CVarGet("debug_bvh_mode");
    Core::CVar* debug_bvh_maxdepth = Core::CVarGet("debug_bvh_maxdepth");

    static glm::vec4 colors[N_OBJECTS];
    static bool once = true;
    for (size_t i = 0; once && i < N_OBJECTS; i++)
    {
        colors[i] = glm::vec4(Core::RandomFloat(), Core::RandomFloat(), Core::RandomFloat(), 1);
    } once = false;

    const int mode = Core::CVarReadInt(debug_bvh_mode);
    if (mode == 4)
    {
        const uint index = (uint)Core::CVarReadInt(Core::CVarGet("debug_bvh_node_index"));
        BVHNode* node = bvh->nodes + glm::min(index, bvh->nodesUsed - 1);
        const glm::vec3 center = (node->bbox.max + node->bbox.min) / 2.0f;
        Debug::DrawBox(glm::translate(center) * glm::scale(node->bbox.max - node->bbox.min), glm::vec4(glm::vec3(1), 1), Debug::RenderMode::WireFrame);
    }
    else
    {
        if (mode > 2)
        { // Draw objects and bboxes
            for (size_t i = 0; i < N_OBJECTS; i++)
            {
                Debug::DrawBox(glm::translate(objects[i]) * glm::scale(glm::vec3(0.015f)), { 1,1,1,1 });
            }

            for (size_t i = 0; i < N_OBJECTS; i++)
            {
                Debug::DrawBox(glm::translate(objects[i]) * glm::scale(bboxes[i].max - bboxes[i].min), colors[i], Debug::RenderMode::WireFrame);
            }
        }

        if (mode > 1)
        {
            const int maxDepth = Core::CVarReadInt(debug_bvh_maxdepth);
            DrawBVH(bvh->nodes, 0, maxDepth);
        }
    }
}

struct ColliderMesh
{
    struct Triangle
    {
        glm::vec3 vertices[3];
        glm::vec3 normal;
    };
    std::vector<Triangle> tris;
    float bSphereRadius;
};

struct Colliders
{
    std::vector<bool> active;
    std::vector<uint16_t> masks;
    std::vector<void*> userData;
    std::vector<glm::vec4> positionsAndScales;
    std::vector<glm::mat4> invTransforms;
    std::vector<ColliderMeshId> meshes;
};

static Colliders colliders;
static std::vector<ColliderMesh> meshes;
static Util::IdPool<ColliderMeshId> colliderMeshPool;
static Util::IdPool<ColliderId> colliderPool;

//------------------------------------------------------------------------------
/**
    templated with index type because gltf supports everything from 8 to 32 bits, signed or unsigned.
*/
template<typename INDEX_T> void
LoadFromIndexBuffer(fx::gltf::Document const& doc, ColliderMesh* mesh)
{
    fx::gltf::Primitive const& primitive = doc.meshes[0].primitives[0];

    fx::gltf::Accessor const& ibAccessor = doc.accessors[primitive.indices];
    fx::gltf::BufferView const& ibView = doc.bufferViews[ibAccessor.bufferView];
    fx::gltf::Buffer const& ib = doc.buffers[ibView.buffer];

    fx::gltf::Accessor const& vbAccessor = doc.accessors[primitive.attributes.find("POSITION")->second];
    fx::gltf::BufferView const& vbView = doc.bufferViews[vbAccessor.bufferView];
    fx::gltf::Buffer const& vb = doc.buffers[vbView.buffer];

    size_t numIndices = ibAccessor.count;
    INDEX_T const* indexBuffer = (INDEX_T const*)&ib.data[ibAccessor.byteOffset + ibView.byteOffset];

    float const* vertexBuffer = (float const*)&vb.data[vbAccessor.byteOffset + vbView.byteOffset];

#if _DEBUG
    assert(vbAccessor.type == fx::gltf::Accessor::Type::Vec3 || vbAccessor.type == fx::gltf::Accessor::Type::Vec4);
#endif
    size_t vSize = (vbAccessor.type == fx::gltf::Accessor::Type::Vec3) ? 3 : 4; // HACK: Assumes 3d or 4d vertex positions
    for (int i = 0; i < numIndices; i += 3)
    {
        ColliderMesh::Triangle tri;
        tri.vertices[0] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i]],
            vertexBuffer[vSize * indexBuffer[i] + 1],
            vertexBuffer[vSize * indexBuffer[i] + 2]
        );
        tri.vertices[1] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i + 1]],
            vertexBuffer[vSize * indexBuffer[i + 1] + 1],
            vertexBuffer[vSize * indexBuffer[i + 1] + 2]
        );
        tri.vertices[2] = glm::vec3(
            vertexBuffer[vSize * indexBuffer[i + 2]],
            vertexBuffer[vSize * indexBuffer[i + 2] + 1],
            vertexBuffer[vSize * indexBuffer[i + 2] + 2]
        );

        // compute plane's normal
        glm::vec3 AB = tri.vertices[1] - tri.vertices[0];
        glm::vec3 AC = tri.vertices[2] - tri.vertices[0];
        tri.normal = glm::cross(AC, AB);

        mesh->tris.push_back(std::move(tri));
    }

    // bounding sphere radius is max of x, y or z from aabb
    mesh->bSphereRadius = vbAccessor.max[0];
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, vbAccessor.max[1]);
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, vbAccessor.max[2]);
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[0]));
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[1]));
    mesh->bSphereRadius = std::max(mesh->bSphereRadius, fabs(vbAccessor.min[2]));
}


//------------------------------------------------------------------------------
/**
*/
ColliderMeshId
LoadColliderMesh(std::string path)
{
    ColliderMeshId id;
    ColliderMesh* mesh;
    if (colliderMeshPool.Allocate(id))
    {
        ColliderMesh newMesh;
        meshes.push_back(std::move(newMesh));
    }
    mesh = &meshes[id.index];

    // Load mesh from file
    fx::gltf::Document doc;
    try
    {
        if (path.substr(path.find_last_of(".") + 1) == "glb")
            doc = fx::gltf::LoadFromBinary(path);
        else
            doc = fx::gltf::LoadFromText(path);
    }
    catch (const std::exception& err)
    {
        printf(err.what());
        assert(false);
        colliderMeshPool.Deallocate(id);
        return ColliderMeshId();
    }

    // HACK: currently only supports one primtive per collider mesh. Needs to be the only one in the GLTF as well...
    fx::gltf::Primitive const& primitive = doc.meshes[0].primitives[0];
    fx::gltf::Accessor const& ibAccessor = doc.accessors[primitive.indices];
    fx::gltf::Accessor::ComponentType componentType = ibAccessor.componentType;

    switch (componentType)
    {
    case fx::gltf::Accessor::ComponentType::Byte:
        LoadFromIndexBuffer<int8_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedByte:
        LoadFromIndexBuffer<uint8_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::Short:
        LoadFromIndexBuffer<int16_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedShort:
        LoadFromIndexBuffer<uint16_t>(doc, mesh);
        break;
    case fx::gltf::Accessor::ComponentType::UnsignedInt:
        LoadFromIndexBuffer<uint32_t>(doc, mesh);
        break;
    default:
        assert(false); // not supported
        break;
    }

    return id;
}

//------------------------------------------------------------------------------
/**
*/
ColliderId
CreateCollider(ColliderMeshId meshId, glm::mat4 const& transform, uint16_t mask, void* userData)
{
#if _DEBUG
    {
        // Only allows uniform scaling along all axes
        float x = glm::length(glm::vec3(transform[0]));
        float y = glm::length(glm::vec3(transform[1]));
        float z = glm::length(glm::vec3(transform[2]));
        assert(fabs(x - y) < 0.00001f && fabs(x - z) < 0.00001f);
    }
#endif
    glm::vec4 PS = glm::vec4(transform[3]);
    PS.w = glm::length(transform[0]);

    ColliderId id;
    if (colliderPool.Allocate(id))
    {
        colliders.positionsAndScales.push_back(PS);
        colliders.invTransforms.push_back(glm::inverse(transform));
        colliders.meshes.push_back(meshId);
        colliders.active.push_back(true);
        colliders.userData.push_back(userData);
        colliders.masks.push_back(mask);
    }
    else
    {
        colliders.positionsAndScales[id.index] = PS;
        colliders.invTransforms[id.index] = glm::inverse(transform);
        colliders.meshes[id.index] = meshId;
        colliders.active[id.index] = true;
        colliders.userData[id.index] = userData;
        colliders.masks[id.index] = mask;
    }
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
SetTransform(ColliderId collider, glm::mat4 const& transform)
{
    assert(colliderPool.IsValid(collider));
#if _DEBUG
    {
        // Only allows uniform scaling along all axes
        float x = glm::length(glm::vec3(transform[0]));
        float y = glm::length(glm::vec3(transform[1]));
        float z = glm::length(glm::vec3(transform[2]));
        assert(fabs(x - y) < 0.00001f && fabs(x - z) < 0.00001f);
    }
#endif
    glm::vec4 PS = glm::vec4(transform[3]);
    PS.w = glm::length(transform[0]);
    colliders.positionsAndScales[collider.index] = PS;
    colliders.invTransforms[collider.index] = glm::inverse(transform);
}

//------------------------------------------------------------------------------
/**
    Cast ray from start point in direction. Make sure the direction is a unit vector.
*/
RaycastPayload
Raycast(glm::vec3 start, glm::vec3 dir, float maxDistance, uint16_t mask)
{
    RaycastPayload ret;
    ret.hitDistance = maxDistance;
    // TODO: spatial acceleration instead of just checking everything...
    int numColliders = (int)colliders.active.size();
    for (int colliderIndex = 0; colliderIndex < numColliders; colliderIndex++)
    {
        if (colliders.active[colliderIndex] && (mask == 0 || (colliders.masks[colliderIndex] & mask) != 0))
        {
            ColliderMesh const* const mesh = &meshes[colliders.meshes[colliderIndex].index];
            glm::vec3 bSphereCenter = colliders.positionsAndScales[colliderIndex];
            float radius = mesh->bSphereRadius * colliders.positionsAndScales[colliderIndex][3];

            // Coarse check against bounding sphere
            {
                glm::vec3 cDir = bSphereCenter - start;

                float r2 = radius * radius;
                float c2 = glm::dot(cDir, cDir);

                if (c2 < r2)
                    goto CHECK_MESH; // ray starts within sphere

                float d = glm::dot(cDir, dir);
                if (d < 0.0f)
                    continue; // ray is pointing away from sphere

                float discr = d * d - (c2 - r2);

                // A negative discriminant corresponds to ray missing sphere 
                if (discr < 0.0f)
                    continue;

                // NOTE: this should be equivalent to this: (sqrtf(c2) - radius > ret.hitDistance)), but faster
                if ((c2 > (ret.hitDistance * ret.hitDistance) + (2 * radius * ret.hitDistance) + r2))
                    continue; // ray is too short
            }

        CHECK_MESH:
            // transform ray into modelspace
            glm::mat4 const& invT = colliders.invTransforms[colliderIndex];
            glm::vec3 invRayStart = invT * glm::vec4(start, 1.0f);
            glm::vec3 invRayDir = invT * glm::vec4(dir, 0);

            // fine check against mesh
            int numTris = (int)mesh->tris.size();
            for (int i = 0; i < numTris; ++i)
            {
                glm::vec3 const& N = mesh->tris[i].normal;

                float NdotRayDirection = glm::dot(N, invRayDir);
                if (NdotRayDirection < 0)
                    continue; // backfacing surface

                glm::vec3 const& A = mesh->tris[i].vertices[0];
                glm::vec3 const& B = mesh->tris[i].vertices[1];
                glm::vec3 const& C = mesh->tris[i].vertices[2];

                float d = -glm::dot(N, A);
                float t = -(glm::dot(N, invRayStart) + d) / NdotRayDirection;

                if (t < 0)
                    continue;  //the triangle is behind the ray

                glm::vec3 P = invRayStart + invRayDir * t;

                // check triangle bounds
                glm::vec3 K;  //vector perpendicular to one of three subdivided triangles's plane 
                glm::vec3 edge0 = B - A;
                glm::vec3 vp0 = P - A;
                K = glm::cross(vp0, edge0);
                if (glm::dot(N, K) < 0)
                    continue;

                glm::vec3 edge1 = C - B;
                glm::vec3 vp1 = P - B;
                K = glm::cross(vp1, edge1);
                if (glm::dot(N, K) < 0)
                    continue;

                glm::vec3 edge2 = A - C;
                glm::vec3 vp2 = P - C;
                K = glm::cross(vp2, edge2);
                if (glm::dot(N, K) < 0)
                    continue;

                // intersection with at least one triangle
                if (ret.hitDistance >= t)
                {
                    ret.hit = true;
                    ret.hitDistance = t;
                    ret.collider = ColliderId::Create(colliderIndex, colliderPool.generations[colliderIndex]);
                }
            }
        }
    }

    if (ret.hit)
    {
        //calculate hitpoint
        ret.hitPoint = start + dir * ret.hitDistance;
    }

    return ret;
}

} // namespace Physics
