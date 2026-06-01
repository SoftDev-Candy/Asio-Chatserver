//
// Created by Candy on 5/2/2026.
//

#ifndef SOUL_CAMERA_HPP
#define SOUL_CAMERA_HPP

#include <QMatrix4x4>
#include <QVector3D>

class Camera
{
public:
    // Builds the camera and throws it back to the default orbit settings.
    Camera();

    // Returns the view matrix from the current yaw, pitch, and zoom values.
    QMatrix4x4 GetViewMatrix() const;

    // Returns the perspective matrix for the current lens settings.
    QMatrix4x4 GetProjectionMatrix(float aspect) const;

    // Yaw is the left-right orbit move. Very spinny, much wow.
    void AddYaw(float yaw_delta);

    // Pitch is the up-down orbit move. We clamp it so the camera does not flip over.
    void AddPitch(float pitch_delta);

    // Zoom pushes the camera closer or farther from Earth.
    void AddZoom(float zoom_delta);

    // TODO: Either wire this up for keyboard input later or delete it if it stays homeless.
    void CameraControls();

    // Resets the camera back to the starting view.
    void CameraReset();

private:
    // Target is the thing the camera stares at. Right now it is Earth chilling at the origin.
    QVector3D cam_target = QVector3D(0.0f, 0.0f, 0.0f);

    // Distance from the target. Bigger number = camera took a few steps back.
    float cam_distance = 4.0f;

    // Horizontal orbit around Earth.
    float cam_yaw = 0.0f;

    // Vertical orbit angle.
    float cam_pitch = 0.0f;

    // Width of the camera lens in degrees.
    float cam_fov = 45.0f;

    // Distance limits so zooming stays in a useful range.
    float cam_minDistance = 1.5f;
    float cam_maxDistance = 20.0f;
};

#endif //SOUL_CAMERA_HPP
