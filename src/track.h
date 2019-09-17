#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"
#include "gl.h"
#include "smallvec.h"
#include "mesh.h"
#include "decal.h"

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

struct RailingMeshType
{
    bool flat;
    const char* meshName;
    f32 scale;
    const char* texName;
};
RailingMeshType railingMeshTypes[] = {
    { false, "world.Rail", 1.f, "concrete" },
    { true, "world.RumbleStrip", 0.5f, "rumble" },
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

    BoundingBox boundingBox;

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
        BoundingBox boundingBox;
        Track* track;

        BezierSegment(Track* track) : track(track) { }
        BezierSegment(BezierSegment const& other) = delete;
        BezierSegment(BezierSegment&& other) = default;
        BezierSegment& operator = (BezierSegment const& other) = delete;
        BezierSegment& operator = (BezierSegment && other) = default;

        ~BezierSegment()
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

        glm::vec3 pointOnCurve(f32 t) const
        {
            glm::vec3 p0 = track->points[pointIndexA].position;
            glm::vec3 p1 = track->points[pointIndexA].position + handleOffsetA;
            glm::vec3 p2 = track->points[pointIndexB].position + handleOffsetB;
            glm::vec3 p3 = track->points[pointIndexB].position;
            return pointOnBezierCurve(p0, p1, p2, p3, t);
        }

        glm::vec3 directionOnCurve(f32 t) const
        {
            glm::vec3 p0 = pointOnCurve(t);
            glm::vec3 p1 = pointOnCurve(t - 0.01f);
            return glm::normalize(p0 - p1);
        }

        f32 getLength() const
        {
            f32 totalLength = 0.f;
            glm::vec3 prevP = track->points[pointIndexA].position;
            for (u32 i=1; i<=10; ++i)
            {
                glm::vec3 p = pointOnCurve(i / 10.f);
                totalLength += glm::length(prevP - p);
                prevP = p;
            }
            return totalLength;
        }
    };

    std::vector<Point> points;
    std::vector<std::unique_ptr<BezierSegment>> connections;

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

    struct Railing : public Renderable
    {
        std::vector<RailingPoint> points;
        Mesh mesh;
        std::vector<Selection> selectedPoints;
        bool isDirty = true;
        PxRigidStatic* actor = nullptr;
        Track* track = nullptr;
        ActorUserData physicsUserData;
        PxShape* collisionShape = nullptr;
        f32 scale = 1.f;
        u32 meshTypeIndex = 0;
        Texture* tex;

        Railing(Track* track) : track(track) {};
        Railing(Railing const& other) = delete;
        Railing(Railing&& other) = default;
        Railing& operator = (Railing const& other) = delete;
        Railing& operator = (Railing && other) = default;
        ~Railing()
        {
            if (actor)
            {
                actor->release();
            }
            mesh.destroy();
        }

        void updateMesh();

        i32 getPriority() const override { return 15; }
        std::string getDebugString() const override { return "Track Marking"; };
        void onLitPassPriorityTransition(class Renderer* renderer) override;
        void onLitPass(class Renderer* renderer) override;
    };

    std::vector<std::unique_ptr<Railing>> railings;

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
    Scene* scene = nullptr;

    PxRigidStatic* actor = nullptr;
    ActorUserData physicsUserData;

    BezierSegment* getPointConnection(u32 pointIndex);
    void createSegmentMesh(BezierSegment& segment, Scene* scene);
    void computeBoundingBox();

public:
    Track()
    {
        points.push_back(Point{ glm::vec3(50, 0, 0.05f) });
        points.push_back(Point{ glm::vec3(-50, 0, 0.05f) });
        auto segment = std::make_unique<BezierSegment>(this);
        segment->handleOffsetA = glm::vec3(-10, 0, 0);
        segment->pointIndexA = 0;
        segment->handleOffsetB = glm::vec3(10, 0, 0);
        segment->pointIndexB = 1;
        connections.push_back(std::move(segment));
    }
    ~Track()
    {
    }
    void trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, struct GridSettings* gridSettings);
    glm::vec3 previewRailingPlacement(Scene* scene, Renderer* renderer, glm::vec3 const& camPos, glm::vec3 const& mouseRayDir);
    void placeRailing(glm::vec3 const& p);
    void placeMarking(glm::vec3 const& p);
    bool canConnect() const { return selectedPoints.size() == 2; }
    bool canConnectRailings() const
    {
        u32 count = 0;
        for (auto& railing : railings)
        {
            if (railing->selectedPoints.size() == 1)
            {
                ++count;
            }
        }
        return count == 2;
    }
    bool canSubdivide() const
    {
        for (auto& railing : railings)
        {
            if (railing->selectedPoints.size() == 2 &&
                (railing->selectedPoints[0].pointIndex - 1 == railing->selectedPoints[1].pointIndex
                 || railing->selectedPoints[0].pointIndex + 1 == railing->selectedPoints[1].pointIndex))
            {
                return true;
            }
        }
        return false;
    }
    bool canSplit() const
    {
        for (auto& railing : railings)
        {
            if (railing->selectedPoints.size() == 1)
            {
                return true;
            }
        }
        return false;
    }
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
    void connectRailings();
    void subdividePoints();
    void split();
    bool hasSelection() const { return selectedPoints.size() > 0; }
    i32 getSelectedPointIndex()
    {
        assert(hasSelection());
        return selectedPoints.back().pointIndex;
    }
    Point const& getPoint(i32 index)
    {
        assert(points.size() > index);
        return points[index];
    }
    glm::vec3 getPointDir(u32 pointIndex) const;
    void clearSelection();
    void buildTrackGraph(class TrackGraph* trackGraph);
    void drawTrackPreview(class TrackPreview2D* trackPreview, glm::mat4 const& orthoProjection);
    BoundingBox getBoundingBox() const { return boundingBox; }
    void applyDecal(Decal& decal) override
    {
        BoundingBox decalBoundingBox = decal.getBoundingBox();
        for (auto& c : connections)
        {
            if (decalBoundingBox.intersects(c->boundingBox))
            {
                decal.addMesh((f32*)c->vertices.data(), sizeof(Vertex),
                        c->indices.data(), c->indices.size(), glm::mat4(1.f));
            }
        }
    }

    // entity
    void onCreate(Scene* scene) override;
    void onRender(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    DataFile::Value serialize() override;
    void deserialize(DataFile::Value& data) override;

    // renderable
    std::string getDebugString() const override { return "Track"; }
    i32 getPriority() const override { return 0; }
    void onShadowPass(class Renderer* renderer) override;
    void onDepthPrepass(class Renderer* renderer) override;
    void onLitPass(class Renderer* renderer) override;
};
