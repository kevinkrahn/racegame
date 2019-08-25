#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"
#include "gl.h"

class Track : public Renderable, public Entity
{
    friend class Editor;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };

    struct Point
    {
        glm::vec3 position;
    };

    struct BezierSegment
    {
        glm::vec3 handleOffsetA;
        i32 pointIndexA;
        glm::vec3 handleOffsetB;
        i32 pointIndexB;
        f32 widthA;
        f32 widthB;
        bool isDirty = true;
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        GLuint vao = 0, vbo = 0, ebo = 0;
        PxShape* collisionShape = nullptr;

        void destroy()
        {
            glDeleteBuffers(0, &vbo);
            glDeleteBuffers(0, &ebo);
            glDeleteVertexArrays(0, &vao);
        }
    };

    std::vector<Point> points;
    std::vector<BezierSegment> connections;

    glm::vec2 selectMousePos;
    glm::vec3 dragStartPoint;
    i32 dragConnectionIndex = -1;
    i32 dragConnectionHandle = -1;
    bool isDragging = false;
    glm::vec3 dragOffset;
    struct Selection
    {
        i32 pointIndex;
        glm::vec3 dragStartPoint;
    };
    std::vector<Selection> selectedPoints;

    PxRigidStatic* actor = nullptr;
    std::unique_ptr<struct ActorUserData> physicsUserData;

    glm::vec3 getPointOnBezierCurve(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, f32 t) const
    {
        f32 u = 1.f - t;
        f32 t2 = t * t;
        f32 u2 = u * u;
        f32 u3 = u2 * u;
        f32 t3 = t2 * t;
        return glm::vec3(u3 * p0 + (3.f * u2 * t) * p1 + (3.f * u * t2) * p2 + t3 * p3);
    }

    glm::vec3 getPointDir(u32 pointIndex);
    BezierSegment* getPointConnection(u32 pointIndex);
    void createSegmentMesh(BezierSegment& segment, Scene* scene);
    void offset(Scene* scene, BezierSegment const& segment);

public:
    Track()
    {
        points.push_back(Point{ glm::vec3(50, 0, 0.05f) });
        points.push_back(Point{ glm::vec3(-50, 0, 0.05f) });
        connections.push_back(BezierSegment{ glm::vec3(-10, 0, 0), 0, glm::vec3(10, 0, 0), 1 });
    }
    ~Track()
    {
        for (auto& c : connections)
        {
            c.destroy();
        }
    }
    void trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, struct GridSettings* gridSettings);
    glm::mat4 getStart()
    {
        BezierSegment* c = getPointConnection(0);
        glm::vec3 xDir = glm::normalize(c->handleOffsetA);
        glm::vec3 yDir = glm::cross(glm::vec3(0, 0, 1), xDir);
        glm::vec3 zDir = glm::cross(xDir, yDir);
        glm::mat4 m(1.f);
        m[0] = glm::vec4(xDir, m[0].w);
        m[1] = glm::vec4(yDir, m[1].w);
        m[2] = glm::vec4(zDir, m[2].w);
        return glm::translate(glm::mat4(1.f),
                points[0].position + glm::vec3(0, 0, 3.f) + glm::normalize(c->handleOffsetA) * 50.f) * m;
    }

    // entity
    void onCreate(Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;

    // renderable
    std::string getDebugString() const override { return "Track"; }
    i32 getPriority() const override { return 50; }
    void onShadowPass(class Renderer* renderer) override;
    void onDepthPrepass(class Renderer* renderer) override;
    void onLitPass(class Renderer* renderer) override;
};
