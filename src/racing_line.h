#pragma once

#include "math.h"
#include "track_graph.h"
#include <vector>

class RacingLine
{
public:
    struct Point
    {
        // serialized
        glm::vec3 position;
        f32 targetSpeed;

        // not serialized
        f32 distanceToHere;
        f32 trackProgress;
    };

    f32 length = 0.f;
    std::vector<Point> points;
    Point endPoint;

    void build(TrackGraph const& trackGraph)
    {
        assert(points.size() > 1);

        length = 0.f;
        points[0].distanceToHere = 0.f;
        for (u32 i=1; i<points.size(); ++i)
        {
            length += glm::distance(points[i].position, points[i-1].position);
            points[i].distanceToHere = length;
        }
        if (length > 0.f)
        {
            length += glm::distance(points[0].position, points[1].position);
        }
        endPoint = points[0];
        endPoint.distanceToHere = length;
        endPoint.trackProgress = 0.f;

        f32 previousTrackProgress = trackGraph.getStartNode()->t;
        for (auto& p : points)
        {
            p.trackProgress = trackGraph.findTrackProgressAtPoint(p.position, previousTrackProgress);
            previousTrackProgress = p.trackProgress;
        }
    }

    Point getPointAt(f32 distance) const
    {
        assert(points.size() > 1);

        // if the distance exceeds the length then wrap around
        distance -= (length * glm::floor(distance / length));

        i32 pointIndex = points.size() - 1;
        while (pointIndex > 0 && distance < points[pointIndex].distanceToHere)
        {
            --pointIndex;
        }
        Point pointA = points[pointIndex];
        Point pointB = endPoint;
        if (pointIndex != (i32)points.size() - 1)
        {
            pointB = points[pointIndex + 1];
        }

        f32 percentageBetween = (distance - pointA.distanceToHere)
            / (pointB.distanceToHere - pointA.distanceToHere);
        glm::vec3 position = pointA.position
            + (pointB.position - pointA.position) * percentageBetween;
        f32 targetSpeed = pointA.targetSpeed
            + (pointB.targetSpeed - pointA.targetSpeed) * percentageBetween;
        f32 trackProgress = pointA.trackProgress
            + (pointB.trackProgress - pointA.trackProgress) * percentageBetween;
        return { position, targetSpeed, distance, trackProgress };
    }

    Point getNearestPoint(glm::vec3 const& position, f32 currentTrackProgress) const
    {
        assert(points.size() > 1);

        f32 minDistance = FLT_MAX;
        Point nearestPoint;
        for (u32 i=0; i<points.size(); ++i)
        {
            Point const& pointA = points[i];
            Point const& pointB = (i != points.size() - 1) ? points[i+1] : endPoint;

            glm::vec3 ap = position - pointA.position;
            glm::vec3 ab = pointB.position - pointA.position;
            f32 distanceAlongLine = glm::clamp(glm::dot(ap, ab) / glm::length2(ab), 0.f, 1.f);
            glm::vec3 pointPosition = pointA.position
                + (pointB.position - pointA.position) * distanceAlongLine;
            f32 trackProgress = pointA.trackProgress
                + (pointB.trackProgress - pointA.trackProgress) * distanceAlongLine;
            f32 distanceSquaredToTestPosition = glm::distance2(pointPosition, position);
            if (glm::abs(currentTrackProgress - trackProgress) > 55.f)
            {
                distanceSquaredToTestPosition += square(30);
            }
            if (minDistance > distanceSquaredToTestPosition)
            {
                minDistance = distanceSquaredToTestPosition;
                nearestPoint.position = pointPosition;
                nearestPoint.distanceToHere = pointA.distanceToHere
                    + (pointB.distanceToHere - pointA.distanceToHere) * distanceAlongLine;
                nearestPoint.targetSpeed = pointA.targetSpeed
                    + (pointB.targetSpeed - pointA.targetSpeed) * distanceAlongLine;
                nearestPoint.trackProgress = trackProgress;
            }
        }

        return nearestPoint;
    }
};
