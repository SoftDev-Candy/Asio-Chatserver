//
// Created by Candy on 4/24/2026.
//

#ifndef SOUL_ORBITVIEW_HPP
#define SOUL_ORBITVIEW_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPoint>
#include <QWheelEvent>
#include <QMouseEvent>
#include <array>
#include <vector>
#include "../../Common/ScenarioState.hpp"
#include "../render/TexturedMesh.hpp"
#include "Camera.hpp"
#include "OrbitViewSceneHelper.hpp"
#include "SatelliteVisual.hpp"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class Orbitview;
}

QT_END_NAMESPACE

class Orbitview : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    struct SatRenderData
    {
        // Name is what we line up with the UI table and the SQLite rows.
        QString name;

        // Visual holds the orbit math and model transform knobs.
        SatelliteVisual visual;
    };

private:
    // Shader for the textured Earth and the tiny satellite mesh.
    QOpenGLShaderProgram* program = nullptr;

    // Fake star backdrop now lives in a proper GL texture so Earth can cover it normally.
    QOpenGLShaderProgram* starBackdropProgram = nullptr;
    QOpenGLVertexArrayObject starBackdropVao;
    QOpenGLBuffer starBackdropVbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLTexture* starBackdropTexture = nullptr;

    // Separate little shader because orbit trails are just colored lines, not textured models.
    QOpenGLShaderProgram* orbitRingProgram = nullptr;

    // Separate point shader so satellites do not vanish into the void the second we blink.
    QOpenGLShaderProgram* satelliteGlowProgram = nullptr;

    // These buffers are reused for each satellite trail instead of making three separate copies.
    QOpenGLVertexArrayObject orbitRingVao;

    QOpenGLBuffer orbitRingVbo{QOpenGLBuffer::VertexBuffer};
    int orbitRingVertexCount = 0;

    // Same idea here, one tiny glow point buffer reused for whichever satellite we are drawing.
    QOpenGLVertexArrayObject satelliteGlowVao;
    QOpenGLBuffer satelliteGlowVbo{QOpenGLBuffer::VertexBuffer};

protected:
    // Zooms the camera in or out using the mouse wheel.
    void wheelEvent(QWheelEvent* event) override;

    // Saves the mouse position so drag rotation can start from the right spot.
    void mousePressEvent(QMouseEvent* event) override;

    // Rotates the camera while the left mouse button is dragged.
    void mouseMoveEvent(QMouseEvent* event) override;

public:
    // Builds the OpenGL widget and turns on the mouse input used by the camera.
    explicit Orbitview(QWidget* parent = nullptr);

    // Cleans up the OpenGL resources owned by the widget before shutdown.
    ~Orbitview() override;

    // Creates the shader program, uploads the Earth mesh, and loads the textures it needs.
    void initializeGL() override;

    // Keeps the OpenGL viewport matched to the widget size.
    void resizeGL(int w, int h) override;

    // Draws the Earth and satellite with the current camera and shader uniforms.
    void paintGL() override;

    // Stores which satellite the operator picked so we can highlight it in the scene.
    void SetSelectedSatellite(const QString& satelliteName);

    // Stores the current link status for the selected satellite so we can tint it a bit.
    void SetSatelliteLinkStatus(const QString& linkStatus);

    // Switches the scene into storm mode.
    void TriggerSolarStorm();

    // Starts the recovery fade back toward normal visuals.
    void ResetScenario();

    // Lets the UI set the scenario directly if we add more modes later.
    void SetScenarioState(ScenarioState state);

    // Gives the selected satellite a quick visual "repair is happening" moment.
    void BeginSatelliteRepair(const QString& satelliteName);

    // Tiny Earth spin value so the planet does not stay completely static.
    float earthRotation = 0.0f;

    // Earth mesh and day texture live together here so Orbitview stays smaller.
    TexturedMesh earthMesh;

    // Satellite mesh uses the same shader for now so it can be added with minimal changes.
    TexturedMesh satelliteMesh;

    // Night texture stays separate because the Earth shader still samples two maps.
    QOpenGLTexture* nightTexture = nullptr;

    // Camera used for orbit controls and matrix generation.
    Camera camera;

    // These visuals line up with SAT_1, SAT_2, and SAT_3 in the backend.
    std::array<SatRenderData, 3> satellites;
    QString selectedSatelliteName;
    QString satelliteLinkStatus = "Disconnected";
    ScenarioState scenarioState = ScenarioState::Normal;
    float stormTintAmount = 0.0f;
    QString repairSatName;
    float repairGlowAmount = 0.0f;

    // Mouse drag remembers the last click spot so orbit controls do not jump.
    QPoint lastMousePos;

private:
    struct StormParticle
    {
        QVector3D position;
        QVector3D velocity;
        QVector3D color;
        float pointSize = 10.0f;
        float lifeLeft = 0.0f;
        float lifeSpan = 1.0f;
        float swayPhase = 0.0f;
    };

    // Rebuilds the star backdrop texture when the widget size changes.
    void RebuildStarBackdrop(int w, int h);

    // Updates the solar storm particles and pulse timing when the storm mode is on.
    void UpdateSolarStorm(float dt, const QMatrix4x4& view);

    // Draws the molten pulse ring around Earth while the storm is active.
    void DrawSolarStormPulse(const QMatrix4x4& view, const QMatrix4x4& projection);

    // Draws the particle wave that comes from the bottom of the screen perspective-wise.
    void DrawSolarStormParticles(const QMatrix4x4& view, const QMatrix4x4& projection);

    // Small GL setup helper so initializeGL does not get too long.
    void SetupSolarStormGraphics();

    QOpenGLShaderProgram* stormRingProgram = nullptr;
    QOpenGLVertexArrayObject stormRingVao;
    QOpenGLBuffer stormRingVbo{QOpenGLBuffer::VertexBuffer};
    int stormRingVertexCount = 0;

    QOpenGLShaderProgram* stormWaveProgram = nullptr;
    QOpenGLVertexArrayObject stormWaveVao;
    QOpenGLBuffer stormWaveVbo{QOpenGLBuffer::VertexBuffer};

    std::vector<StormParticle> stormParticles;
    float stormTime = 0.0f;
    float stormSpawnCarry = 0.0f;
};

#endif //SOUL_ORBITVIEW_HPP
