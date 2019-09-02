#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"
#include "gl.h"
#include "smallvec.h"
#include "mesh.h"

struct TrackItem
{
    const char* name;
    const char* icon;
    struct Curve
    {
        glm::vec3 offset;
        glm::vec3 handleOffset;
    };
    SmallVec<Curve> curves;
};

TrackItem prefabTrackItems[] = {
    { "Straight", "straight_track_icon", {
        { { 50.f, 0.f, 0.f }, { -10.f, 0.f, 0.f } },
    }},
    { "Left Turn", "left_turn_track_icon", {
        { { 20.f, 0.f, 0.f }, { -10.f, 0.f, 0.f } },
        { { 20.f, -20.f, 0.f }, { 0.f, 10.f, 0.f } },
    }},
    { "Right Turn", "right_turn_track_icon", {
        { { 20.f, 0.f, 0.f }, { -10.f, 0.f, 0.f } },
        { { 20.f, 20.f, 0.f }, { 0.f, -10.f, 0.f } },
    }},
};

class Track : public Renderable, public Entity
{
public:
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

private:
    struct BezierSegment
    {
        glm::vec3 handleOffsetA;
        i32 pointIndexA;
        glm::vec3 handleOffsetB;
        i32 pointIndexB;
        f32 widthA = 12.f;
        f32 widthB = 12.f;

        bool isDirty = true;
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        GLuint vao = 0, vbo = 0, ebo = 0;
        PxShape* collisionShape = nullptr;

        void destroy(Track* track)
        {
            if (vao)
            {
                glDeleteBuffers(0, &vbo);
                glDeleteBuffers(0, &ebo);
                glDeleteVertexArrays(0, &vao);
            }
            if (collisionShape)
            {
                track->actor->detachShape(*collisionShape);
            }
        }

        glm::vec3 pointOnCurve(std::vector<Point> const& points, f32 t) const
        {
            f32 u = 1.f - t;
            f32 t2 = t * t;
            f32 u2 = u * u;
            f32 u3 = u2 * u;
            f32 t3 = t2 * t;
            glm::vec3 p0 = points[pointIndexA].position;
            glm::vec3 p1 = points[pointIndexA].position + handleOffsetA;
            glm::vec3 p2 = points[pointIndexB].position + handleOffsetB;
            glm::vec3 p3 = points[pointIndexB].position;
            return glm::vec3(u3 * p0 + (3.f * u2 * t) * p1 + (3.f * u * t2) * p2 + t3 * p3);
        }

        glm::vec3 directionOnCurve(std::vector<Point> const& points, f32 t) const
        {
            glm::vec3 p0 = pointOnCurve(points, t);
            glm::vec3 p1 = pointOnCurve(points, t - 0.01f);
            return glm::normalize(p0 - p1);
        }

        f32 getLength(std::vector<Point> const& points) const
        {
            f32 totalLength = 0.f;
            glm::vec3 prevP = points[pointIndexA].position;
            for (u32 i=1; i<=10; ++i)
            {
                glm::vec3 p = pointOnCurve(points, i / 10.f);
                totalLength += glm::length(prevP - p);
                prevP = p;
            }
            return totalLength;
        }
    };

    std::vector<Point> points;
    std::vector<BezierSegment> connections;

    struct Selection
    {
        i32 pointIndex;
        glm::vec3 dragStartPoint;
    };

    struct RailingPoint
    {
        glm::vec3 position;
        glm::vec3 handleOffsetA;
        glm::vec3 handleOffsetB;
    };

    struct Railing
    {
        std::vector<RailingPoint> points;
        Mesh mesh;
        std::vector<Selection> selectedPoints;
        bool isDirty = false;
    };

    std::vector<Railing> railings;

    glm::vec2 selectMousePos;
    glm::vec3 dragStartPoint;
    i32 dragRailingIndex = -1;
    i32 dragConnectionIndex = -1;
    i32 dragConnectionHandle = -1;
    i32 dragOppositeConnectionIndex = -1;
    i32 dragOppositeConnectionHandle = -1;
    bool isDragging = false;
    glm::vec3 dragOffset;
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

    BezierSegment* getPointConnection(u32 pointIndex);
    void createSegmentMesh(BezierSegment& segment, Scene* scene);

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
            c.destroy(this);
        }
    }
    void trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, struct GridSettings* gridSettings);
    glm::mat4 getStart()
    {
        BezierSegment* c = getPointConnection(0);
        f32 length = c->getLength(points);
        f32 t = 20.f / length;
        glm::vec3 xDir = glm::normalize(c->directionOnCurve(points, t));
        glm::vec3 yDir = glm::cross(glm::vec3(0, 0, 1), xDir);
        glm::vec3 zDir = glm::cross(xDir, yDir);
        glm::mat4 m(1.f);
        m[0] = glm::vec4(xDir, m[0].w);
        m[1] = glm::vec4(yDir, m[1].w);
        m[2] = glm::vec4(zDir, m[2].w);
        return glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 3) + c->pointOnCurve(points, t)) * m;
    }
    glm::vec3 previewRailingPlacement(Scene* scene, Renderer* renderer, glm::vec3 const& camPos, glm::vec3 const& mouseRayDir);
    void placeRailing(glm::vec3 const& p);
    bool canConnect() const { return selectedPoints.size() == 2; }
    bool canSubdivide() const { return false; } // TODO: implement
    bool canExtendTrack() const
    {
        if (!hasSelection())
        {
            return false;
        }
        i32 pointIndex = selectedPoints.back().pointIndex;
        glm::vec3 xDir = getPointDir(pointIndex);
        return glm::length2(xDir) > 0.f;
    }
    void extendTrack(i32 prefabCurveIndex);
    void connectPoints();
    void subdividePoints();
    bool hasSelection() const { return selectedPoints.size() > 0; }
    i32 getSelectedPointIndex() {
        assert(hasSelection());
        return selectedPoints.back().pointIndex;
    }
    Point const& getPoint(i32 index) {
        assert(points.size() > index);
        return points[index];
    }
    glm::vec3 getPointDir(u32 pointIndex) const;
    void clearSelection();
    void buildTrackGraph(class TrackGraph* trackGraph);

    // entity
    void onCreate(Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;

    // renderable
    std::string getDebugString() const override { return "Track"; }
    i32 getPriority() const override { return 50; }
    void onShadowPass(class Renderer* renderer) override;
    void onDepthPrepass(class Renderer* renderer) override;
    void onLitPass(class Renderer* renderer) override;
};
