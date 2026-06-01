//
// Created by Candy on 5/8/2026.
//

#ifndef SOUL_SATELLITEVISUAL_HPP
#define SOUL_SATELLITEVISUAL_HPP

#include <QMatrix4x4>
#include <QVector3D>

class SatelliteVisual
{
public:
    // These are the orbit knobs.
    // radius = how far from Earth we are
    // angle = where the satellite currently is
    // speed = how fast it moves around
    // tilt = how slanted the orbit plane is
    // phase = where the orbit starts so all satellites are not stacked in the same place
    // scale = how chunky the model looks in the scene
    float orbitRadius = 2.1f;
    float orbitAngle = 0.0f;
    float orbitSpeed = 20.0f;
    float orbitTiltDeg = 0.0f;
    float orbitPhaseDeg = 0.0f;
    float scale = 0.00002f;

    // Moves the satellite forward around its orbit using the frame time step.
    void Update(float dt);

    // Gives the current world position of the satellite on its orbit.
    QVector3D GetOrbitPosition() const;

    // Same as above but for any angle we want, which is handy for drawing orbit trails.
    QVector3D GetOrbitPositionForAngle(float angleDeg) const;

    // Builds the model matrix for the satellite from its orbit settings.
    QMatrix4x4 GetModelMatrix() const;

    // TODO: If we ever want elliptical orbits, this class is where the likkle upgrade should live.
};

#endif //SOUL_SATELLITEVISUAL_HPP
