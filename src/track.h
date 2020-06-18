#pragma once

#include "misc.h"
#include "math.h"
#include "entity.h"
#include "gl.h"
#include "mesh.h"
#include "decal.h"
#include "spline.h"

class Track : public Entity
{
public:
    struct Vertex
    {
        Vec3 position;
        Vec3 normal;
    };

    struct Point
    {
        Vec3 position;

        u32 trackGraphNodeIndex = UINT32_MAX;

        void serialize(Serializer& s) { s.field(position); }
    };

    BoundingBox boundingBox;

    struct TrackItem
    {
        const char* name;
        const char* icon;
        struct Curve
        {
            Vec3 offset;
            Vec3 handleOffset;
        };
        SmallArray<Curve> curves;
    };
    TrackItem prefabTrackItems[3] = {
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


private:
    struct BezierSegment
    {
        Vec3 handleOffsetA;
        i32 pointIndexA;
        Vec3 handleOffsetB;
        i32 pointIndexB;
        f32 widthA = 12.f;
        f32 widthB = 12.f;

        bool isDirty = true;
        Array<Vertex> vertices;
        Array<u32> indices;
        GLuint vao = 0, vbo = 0, ebo = 0;
        PxShape* collisionShape = nullptr;
        BoundingBox boundingBox;
        Track* track;
        u32 trackGraphNodeIndexA = UINT32_MAX;
        u32 trackGraphNodeIndexB = UINT32_MAX;

        void serialize(Serializer& s)
        {
            s.field(handleOffsetA);
            s.field(pointIndexA);
            s.field(handleOffsetB);
            s.field(pointIndexB);
            s.field(widthA);
            s.field(widthB);
        }

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

        Vec3 pointOnCurve(f32 t) const
        {
            Vec3 p0 = track->points[pointIndexA].position;
            Vec3 p1 = track->points[pointIndexA].position + handleOffsetA;
            Vec3 p2 = track->points[pointIndexB].position + handleOffsetB;
            Vec3 p3 = track->points[pointIndexB].position;
            return pointOnBezierCurve(p0, p1, p2, p3, t);
        }

        Vec3 directionOnCurve(f32 t) const
        {
            Vec3 p0 = pointOnCurve(t);
            Vec3 p1 = pointOnCurve(t - 0.01f);
            return normalize(p0 - p1);
        }

        f32 getLength() const
        {
            f32 totalLength = 0.f;
            Vec3 prevP = track->points[pointIndexA].position;
            for (u32 i=1; i<=10; ++i)
            {
                Vec3 p = pointOnCurve(i / 10.f);
                totalLength += length(prevP - p);
                prevP = p;
            }
            return totalLength;
        }
    };

    Array<Point> points;
    Array<OwnedPtr<BezierSegment>> connections;

    struct Selection
    {
        i32 pointIndex;
        Vec3 dragStartPoint;
    };

    Vec2 selectMousePos;
    Vec3 dragStartPoint;
    i32 dragConnectionIndex = -1;
    i32 dragConnectionHandle = -1;
    i32 dragOppositeConnectionIndex = -1;
    i32 dragOppositeConnectionHandle = -1;
    bool isDragging = false;
    Vec3 dragOffset;
    Array<Selection> selectedPoints;
    Scene* scene = nullptr;

    PxRigidStatic* actor = nullptr;
    ActorUserData physicsUserData;

    BezierSegment* getPointConnection(i32 pointIndex);
    void createSegmentMesh(BezierSegment& segment, Scene* scene);
    void computeBoundingBox();

    ShaderHandle colorShader = getShaderHandle("track");
    ShaderHandle depthShader = getShaderHandle("track", { {"DEPTH_ONLY"} });

    Mesh previewMesh;
    void buildPreviewMesh(Scene* scene);

public:
    Track()
    {
        points.push_back(Point{ Vec3(50, 0, 0.05f) });
        points.push_back(Point{ Vec3(-50, 0, 0.05f) });
        OwnedPtr<BezierSegment> segment(new BezierSegment);
        segment->track = this;
        segment->handleOffsetA = Vec3(-10, 0, 0);
        segment->pointIndexA = 0;
        segment->handleOffsetB = Vec3(10, 0, 0);
        segment->pointIndexB = 1;
        connections.push_back(std::move(segment));
    }
    ~Track()
    {
        previewMesh.destroy();
    }
    void trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime,
            bool& isMouseHandled, struct GridSettings* gridSettings);
    bool canConnect() const { return selectedPoints.size() == 2; }
    bool canExtendTrack() const
    {
        if (!hasSelection())
        {
            return false;
        }
        i32 pointIndex = selectedPoints.back().pointIndex;
        Vec3 xDir = getPointDir(pointIndex);
        return length2(xDir) > 0.f;
    }
    void matchZ(bool lowest);
    void extendTrack(i32 prefabCurveIndex);
    void connectPoints();
    void subdividePoints();
    bool hasSelection() const { return selectedPoints.size() > 0; }
    i32 getSelectedPointIndex()
    {
        assert(hasSelection());
        return selectedPoints.back().pointIndex;
    }
    Point const& getPoint(i32 index)
    {
        assert((i32)points.size() > index);
        return points[index];
    }
    Vec3 getPointDir(i32 pointIndex) const;
    void clearSelection();
    void buildTrackGraph(class TrackGraph* trackGraph, Mat4 const& startTransform);
    BoundingBox getBoundingBox() const { return boundingBox; }
    void applyDecal(Decal& decal) override
    {
        BoundingBox decalBoundingBox = decal.getBoundingBox();
        for (auto& c : connections)
        {
            if (decalBoundingBox.intersects(c->boundingBox))
            {
                decal.addMesh((f32*)c->vertices.data(), sizeof(Vertex),
                        c->indices.data(), (u32)c->indices.size(), Mat4(1.f));
            }
        }
    }
    Mesh* getPreviewMesh(Scene* scene)
    {
        if (!previewMesh.vao)
        {
            buildPreviewMesh(scene);
        }
        return &previewMesh;
    }

    // entity
    void onCreate(Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void serializeState(Serializer& s) override;
};
