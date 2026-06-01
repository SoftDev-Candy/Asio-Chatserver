//
// Created by Candy on 5/2/2026.
//

#include "Camera.hpp"
#include <QtMath>

Camera::Camera()
{
    CameraReset();
}


QMatrix4x4 Camera::GetViewMatrix() const
{
    float yawRadius = qDegreesToRadians(cam_yaw);
    float pitchRadius = qDegreesToRadians(cam_pitch);

    // Break the orbit into plain xyz pieces first so the math stays easy to follow.
    float x = cam_distance * qCos(pitchRadius) * qSin(yawRadius);
    float y = cam_distance * qSin(pitchRadius);
    float z = cam_distance * qCos(pitchRadius) * qCos(yawRadius);

    // Build the eye position as an orbital offset from the current target.
    QVector3D cameraPos = cam_target + QVector3D(x, y, z);

    QMatrix4x4 view;
    view.setToIdentity();
    view.lookAt(cameraPos, cam_target, QVector3D(0.0f, 1.0f, 0.0f));

    return view;
}


QMatrix4x4 Camera::GetProjectionMatrix(float aspect) const
{
    // Basic perspective projection for the current widget aspect ratio.
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.perspective(cam_fov, aspect, 0.1f, 100.0f);
    return projection;
}


void Camera::AddYaw(float yaw_delta)
{
    cam_yaw += yaw_delta;
}


void Camera::AddPitch(float pitch_delta)
{
    cam_pitch += pitch_delta;

    if (cam_pitch > 85.0f)
    {
        cam_pitch = 85.0f;
    }

    if (cam_pitch < -85.0f)
    {
        cam_pitch = -85.0f;
    }
}


void Camera::AddZoom(float zoom_delta)
{
    // Basic zoom math with clamp checks so the camera stays usable.
    cam_distance += zoom_delta;

    if (cam_distance < cam_minDistance)
    {
        cam_distance = cam_minDistance;
    }

    if (cam_distance > cam_maxDistance)
    {
        cam_distance = cam_maxDistance;
    }
}


void Camera::CameraReset()
{
    // FIXME: If we ever add saved views, this should probably pull from settings instead.
    cam_target = QVector3D(0.0f, 0.0f, 0.0f);
    cam_distance = 12.0f;
    cam_yaw = 0.0f;
    cam_pitch = 0.0f;
    cam_fov = 45.0f;
    cam_minDistance = 1.5f;
    cam_maxDistance = 20.0f;
}
