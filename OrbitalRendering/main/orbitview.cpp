//
// Created by Candy on 4/24/2026.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Orbitview.h" resolved

#include "main/orbitview.hpp"
#include "../Shader/ShaderSource.hpp"
#include <QDebug>
#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>
#include <cmath>

// Mouse wheel just pushes the camera in and out, then asks Qt for a redraw. Simple lil zoom gremlin (❁´◡`❁)
void Orbitview::wheelEvent(QWheelEvent* event)
{
    float zoomAmount = event->angleDelta().y() > 0 ? -0.5f : 0.5f;

    camera.AddZoom(zoomAmount);

    update();
}

// Save the click position so drag rotation knows where it started from. Otherwise the camera would just freestyle.
void Orbitview::mousePressEvent(QMouseEvent* event)
{
    lastMousePos = event->pos();
}

// Left-drag moves the camera yaw and pitch around the scene. Very spaceship movie energy tbh.
void Orbitview::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (event->buttons() & Qt::LeftButton)
    {
        camera.AddYaw(delta.x() * 0.3f);
        camera.AddPitch(-delta.y() * 0.3f);

        update();
    }
}

Orbitview::Orbitview(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // Set the three orbit visuals here once so the scene and the backend names stay in sync.
    // These numbers are mostly just our scene tuning knobs, aka the "looks right enough" department ¯\_(ツ)_/¯
    satellites[0].name = "SAT_1";
    satellites[0].visual.orbitRadius = 1.60f;
    satellites[0].visual.orbitSpeed = 0.40f;
    satellites[0].visual.orbitTiltDeg = 8.0f;
    satellites[0].visual.orbitPhaseDeg = 0.0f;
    satellites[0].visual.scale = 0.00008f;

    satellites[1].name = "SAT_2";
    satellites[1].visual.orbitRadius = 2.00f;
    satellites[1].visual.orbitSpeed = 0.25f;
    satellites[1].visual.orbitTiltDeg = 28.0f;
    satellites[1].visual.orbitPhaseDeg = 120.0f;
    satellites[1].visual.scale = 0.00008f;

    satellites[2].name = "SAT_3";
    satellites[2].visual.orbitRadius = 2.40f;
    satellites[2].visual.orbitSpeed = 0.15f;
    satellites[2].visual.orbitTiltDeg = -24.0f;
    satellites[2].visual.orbitPhaseDeg = 240.0f;
    satellites[2].visual.scale = 0.00008f;
}

void Orbitview::RebuildStarBackdrop(int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }

    if (starBackdropTexture != nullptr)
    {
        delete starBackdropTexture;
        starBackdropTexture = nullptr;
    }

    QImage starImage = OrbitViewSceneHelper::BuildStarBackdrop(w, h);
    starBackdropTexture = new QOpenGLTexture(starImage.flipped(Qt::Vertical));
    starBackdropTexture->setMinificationFilter(QOpenGLTexture::Linear);
    starBackdropTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    starBackdropTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

// The UI tells us which satellite got clicked and we remember that here. Tiny brain note for later.
void Orbitview::SetSelectedSatellite(const QString& satelliteName)
{
    selectedSatelliteName = satelliteName;
    update();
}

// This lets the render side tint the selected satellite based on the link condition from the UI. If the link is cooked, the color should snitch.
void Orbitview::SetSatelliteLinkStatus(const QString& linkStatus)
{
    satelliteLinkStatus = linkStatus;
    update();
}

void Orbitview::BeginSatelliteRepair(const QString& satelliteName)
{
    repairSatName = satelliteName;
    repairGlowAmount = 1.0f;
    update();
}

void Orbitview::TriggerSolarStorm()
{
    SetScenarioState(ScenarioState::SolarStormActive);
}

void Orbitview::ResetScenario()
{
    SetScenarioState(ScenarioState::Recovery);
}

void Orbitview::SetScenarioState(ScenarioState state)
{
    scenarioState = state;

    if (scenarioState == ScenarioState::Normal)
    {
        stormParticles.clear();
        stormTime = 0.0f;
        stormSpawnCarry = 0.0f;
        stormTintAmount = 0.0f;
        repairGlowAmount = 0.0f;
        repairSatName.clear();
    }
    else if (scenarioState == ScenarioState::SolarStormActive)
    {
        // Kick the pulse off from the start so the operator sees the storm right away. No slow intro, just chaos.
        stormTime = 0.0f;
    }

    update();
}

Orbitview::~Orbitview()
{
    if (context() != nullptr)
    {
        // OpenGL objects need the widget context to still be current while we destroy them. OpenGL gets very grumpy otherwise.
        makeCurrent();
        earthMesh.Destroy();
        satelliteMesh.Destroy();

        if (starBackdropTexture != nullptr)
        {
            delete starBackdropTexture;
            starBackdropTexture = nullptr;
        }

        if (starBackdropVbo.isCreated())
        {
            starBackdropVbo.destroy();
        }

        if (starBackdropVao.isCreated())
        {
            starBackdropVao.destroy();
        }

        if (starBackdropProgram != nullptr)
        {
            delete starBackdropProgram;
            starBackdropProgram = nullptr;
        }

        if (orbitRingVbo.isCreated())
        {
            orbitRingVbo.destroy();
        }

        if (orbitRingVao.isCreated())
        {
            orbitRingVao.destroy();
        }

        if (orbitRingProgram != nullptr)
        {
            delete orbitRingProgram;
            orbitRingProgram = nullptr;
        }

        if (satelliteGlowVbo.isCreated())
        {
            satelliteGlowVbo.destroy();
        }

        if (satelliteGlowVao.isCreated())
        {
            satelliteGlowVao.destroy();
        }

        if (satelliteGlowProgram != nullptr)
        {
            delete satelliteGlowProgram;
            satelliteGlowProgram = nullptr;
        }

        if (stormWaveVbo.isCreated())
        {
            stormWaveVbo.destroy();
        }

        if (stormWaveVao.isCreated())
        {
            stormWaveVao.destroy();
        }

        if (stormWaveProgram != nullptr)
        {
            delete stormWaveProgram;
            stormWaveProgram = nullptr;
        }

        if (stormRingVbo.isCreated())
        {
            stormRingVbo.destroy();
        }

        if (stormRingVao.isCreated())
        {
            stormRingVao.destroy();
        }

        if (stormRingProgram != nullptr)
        {
            delete stormRingProgram;
            stormRingProgram = nullptr;
        }

        if (nightTexture != nullptr)
        {
            delete nightTexture;
            nightTexture = nullptr;
        }

        doneCurrent();
    }
}

void Orbitview::SetupSolarStormGraphics()
{
    stormRingProgram = new QOpenGLShaderProgram(this);
    bool pulseVertexCreated = stormRingProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        OrbitViewShaders::SolarStormPulseVertexShader());

    if (!pulseVertexCreated)
    {
        qDebug().noquote() << "Solar storm pulse vertex shader compilation failed:";
        qDebug().noquote() << stormRingProgram->log();
    }

    bool pulseFragmentCreated = stormRingProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        OrbitViewShaders::SolarStormPulseFragmentShader());

    if (!pulseFragmentCreated)
    {
        qDebug().noquote() << "Solar storm pulse fragment shader compilation failed:";
        qDebug().noquote() << stormRingProgram->log();
    }

    if (stormRingProgram->link())
    {
        stormRingVertexCount = 96;

        if (stormRingVao.create())
        {
            stormRingVao.bind();

            if (stormRingVbo.create() && stormRingVbo.bind())
            {
                stormRingVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                stormRingVbo.allocate(stormRingVertexCount * 3 * static_cast<int>(sizeof(float)));

                stormRingProgram->bind();
                stormRingProgram->enableAttributeArray(0);
                stormRingProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));
                stormRingProgram->release();
                stormRingVbo.release();
            }

            stormRingVao.release();
        }
    }
    else
    {
        qDebug().noquote() << "Solar storm pulse shader link failed:";
        qDebug().noquote() << stormRingProgram->log();
    }

    stormWaveProgram = new QOpenGLShaderProgram(this);
    bool particleVertexCreated = stormWaveProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        OrbitViewShaders::SolarStormParticleVertexShader());

    if (!particleVertexCreated)
    {
        qDebug().noquote() << "Solar storm particle vertex shader compilation failed:";
        qDebug().noquote() << stormWaveProgram->log();
    }

    bool particleFragmentCreated = stormWaveProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        OrbitViewShaders::SolarStormParticleFragmentShader());

    if (!particleFragmentCreated)
    {
        qDebug().noquote() << "Solar storm particle fragment shader compilation failed:";
        qDebug().noquote() << stormWaveProgram->log();
    }

    if (stormWaveProgram->link())
    {
        if (stormWaveVao.create())
        {
            stormWaveVao.bind();

            if (stormWaveVbo.create() && stormWaveVbo.bind())
            {
                stormWaveVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

                stormWaveProgram->bind();
                stormWaveProgram->enableAttributeArray(0);
                stormWaveProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 7 * sizeof(float));
                stormWaveProgram->enableAttributeArray(1);
                stormWaveProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 7 * sizeof(float));
                stormWaveProgram->enableAttributeArray(2);
                stormWaveProgram->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 1, 7 * sizeof(float));
                stormWaveProgram->release();
                stormWaveVbo.release();
            }

            stormWaveVao.release();
        }
    }
    else
    {
        qDebug().noquote() << "Solar storm particle shader link failed:";
        qDebug().noquote() << stormWaveProgram->log();
    }
}

void Orbitview::UpdateSolarStorm(float dt, const QMatrix4x4& view)
{
    bool stormIsActive = (scenarioState == ScenarioState::SolarStormActive);
    bool recoveryIsActive = (scenarioState == ScenarioState::Recovery);

    if (!stormIsActive && !recoveryIsActive)
    {
        return;
    }

    stormTime += dt;

    if (stormIsActive)
    {
        stormTintAmount = std::min(1.0f, stormTintAmount + dt * 1.7f);
    }
    else
    {
        stormTintAmount = std::max(0.0f, stormTintAmount - dt * 0.9f);
    }

    bool invertible = false;
    QMatrix4x4 invView = view.inverted(&invertible);
    if (!invertible)
    {
        return;
    }

    QVector3D camRight(invView(0, 0), invView(1, 0), invView(2, 0));
    QVector3D camUp(invView(0, 1), invView(1, 1), invView(2, 1));
    QVector3D camForward(-invView(0, 2), -invView(1, 2), -invView(2, 2));

    if (stormIsActive)
    {
        QVector3D spawnBase = (-camUp * 1.55f) + (camForward * 0.18f);
        stormSpawnCarry += dt * 95.0f;
        int spawnCount = static_cast<int>(stormSpawnCarry);
        stormSpawnCarry -= static_cast<float>(spawnCount);

        for (int i = 0; i < spawnCount; ++i)
        {
            float sideOffset = static_cast<float>(QRandomGenerator::global()->generateDouble() * 2.4 - 1.2);
            float forwardOffset = static_cast<float>(QRandomGenerator::global()->generateDouble() * 0.4 - 0.2);
            float downOffset = static_cast<float>(0.18 + QRandomGenerator::global()->generateDouble() * 0.30);

            StormParticle particle;
            particle.position = spawnBase + (camRight * sideOffset) - (camUp * downOffset) + (camForward * forwardOffset);
            particle.velocity =
                (camUp * static_cast<float>(1.20 + QRandomGenerator::global()->generateDouble() * 0.35)) +
                (camForward * static_cast<float>(0.10 + QRandomGenerator::global()->generateDouble() * 0.20)) -
                (camRight * sideOffset * 0.08f);
            particle.color = OrbitViewSceneHelper::RandomStormColor();
            particle.pointSize = static_cast<float>(10.0 + QRandomGenerator::global()->generateDouble() * 10.0);
            particle.lifeSpan = static_cast<float>(1.15 + QRandomGenerator::global()->generateDouble() * 0.55);
            particle.lifeLeft = particle.lifeSpan;
            particle.swayPhase = static_cast<float>(QRandomGenerator::global()->generateDouble() * 6.2831853);

            stormParticles.push_back(particle);
        }
    }

    for (StormParticle& particle : stormParticles)
    {
        float lifeProgress = 1.0f - (particle.lifeLeft / particle.lifeSpan);
        QVector3D sideSway = camRight * qSin(lifeProgress * 9.5f + particle.swayPhase) * 0.22f;

        particle.position += (particle.velocity + sideSway) * dt;
        particle.velocity += camUp * (0.30f * dt);
        particle.lifeLeft -= dt;
    }

    stormParticles.erase(
        std::remove_if(
            stormParticles.begin(),
            stormParticles.end(),
            [](const StormParticle& particle)
            {
                return particle.lifeLeft <= 0.0f || particle.position.length() > 4.5f;
            }),
        stormParticles.end());

    if (stormParticles.size() > 650)
    {
        stormParticles.erase(stormParticles.begin(), stormParticles.begin() + 120);
    }

    // Once the tint has faded and the leftover particles are gone, we can fully calm the scene down.
    if (recoveryIsActive && stormTintAmount <= 0.01f && stormParticles.empty())
    {
        scenarioState = ScenarioState::Normal;
        stormTime = 0.0f;
        stormSpawnCarry = 0.0f;
    }
}

void Orbitview::DrawSolarStormPulse(const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (stormTintAmount <= 0.01f ||
        stormRingProgram == nullptr ||
        !stormRingVao.isCreated() ||
        stormRingVertexCount <= 0)
    {
        return;
    }

    bool invertible = false;
    QMatrix4x4 invView = view.inverted(&invertible);
    if (!invertible)
    {
        return;
    }

    QVector3D camRight(invView(0, 0), invView(1, 0), invView(2, 0));
    QVector3D camUp(invView(0, 1), invView(1, 1), invView(2, 1));

    float cycle = std::fmod(stormTime, 1.8f) / 1.8f;
    float pulseRadius = 0.70f + cycle * 1.45f;
    float pulseAlpha = 0.55f * (1.0f - cycle) * stormTintAmount;

    std::vector<float> pulseVerts;
    pulseVerts.reserve(stormRingVertexCount * 3);

    for (int i = 0; i < stormRingVertexCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(stormRingVertexCount);
        float angle = t * 6.2831853f;
        QVector3D ringPoint =
            (camRight * (qCos(angle) * pulseRadius)) +
            (camUp * (qSin(angle) * pulseRadius));

        pulseVerts.push_back(ringPoint.x());
        pulseVerts.push_back(ringPoint.y());
        pulseVerts.push_back(ringPoint.z());
    }

    stormRingVbo.bind();
    stormRingVbo.allocate(pulseVerts.data(), static_cast<int>(pulseVerts.size() * sizeof(float)));
    stormRingVbo.release();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    stormRingProgram->bind();
    stormRingProgram->setUniformValue("uMVP", projection * view);
    stormRingProgram->setUniformValue("uColor", QVector3D(1.0f, 0.42f + 0.20f * (1.0f - cycle), 0.10f));
    stormRingProgram->setUniformValue("uAlpha", pulseAlpha);
    stormRingVao.bind();
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_LOOP, 0, stormRingVertexCount);
    stormRingVao.release();
    stormRingProgram->release();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Orbitview::DrawSolarStormParticles(const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if ((scenarioState != ScenarioState::SolarStormActive && scenarioState != ScenarioState::Recovery) ||
        stormWaveProgram == nullptr ||
        !stormWaveVao.isCreated() ||
        stormParticles.empty())
    {
        return;
    }

    std::vector<float> particleVerts;
    particleVerts.reserve(stormParticles.size() * 7);

    for (const StormParticle& particle : stormParticles)
    {
        float lifeAlpha = particle.lifeSpan > 0.0f
            ? particle.lifeLeft / particle.lifeSpan
            : 0.0f;
        float drawSize = particle.pointSize * (0.65f + lifeAlpha * 0.55f) * std::max(0.45f, stormTintAmount);

        particleVerts.push_back(particle.position.x());
        particleVerts.push_back(particle.position.y());
        particleVerts.push_back(particle.position.z());
        particleVerts.push_back(particle.color.x());
        particleVerts.push_back(particle.color.y());
        particleVerts.push_back(particle.color.z());
        particleVerts.push_back(drawSize);
    }

    stormWaveVbo.bind();
    stormWaveVbo.allocate(particleVerts.data(), static_cast<int>(particleVerts.size() * sizeof(float)));
    stormWaveVbo.release();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);

    stormWaveProgram->bind();
    stormWaveProgram->setUniformValue("uMVP", projection * view);
    stormWaveVao.bind();
    glDrawArrays(GL_POINTS, 0, static_cast<int>(stormParticles.size()));
    stormWaveVao.release();
    stormWaveProgram->release();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Orbitview::initializeGL()
{
    initializeOpenGLFunctions();

    // Space is not really black here because plain black looked a little dead and boring.
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Build the shared shader once, then both Earth and satellite can use the same program.
    program = new QOpenGLShaderProgram(this);
    bool vertexCreated = program->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        VertexShader());

    if (!vertexCreated)
    {
        qDebug().noquote() << "Vertex shader compilation failed:";
        qDebug().noquote() << program->log();
    }

    bool fragmentCreated = program->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        FragmentShader());

    if (!fragmentCreated)
    {
        qDebug().noquote() << "Fragment shader compilation failed:";
        qDebug().noquote() << program->log();
    }

    bool linked = program->link();

    if (!linked)
    {
        qDebug().noquote() << "Shader program link failed:";
        qDebug().noquote() << program->log();
        return;
    }

    starBackdropProgram = new QOpenGLShaderProgram(this);
    bool starBackdropVertexCreated = starBackdropProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        OrbitViewShaders::StarBackdropVertexShader());

    if (!starBackdropVertexCreated)
    {
        qDebug().noquote() << "Star backdrop vertex shader compilation failed:";
        qDebug().noquote() << starBackdropProgram->log();
    }

    bool starBackdropFragmentCreated = starBackdropProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        OrbitViewShaders::StarBackdropFragmentShader());

    if (!starBackdropFragmentCreated)
    {
        qDebug().noquote() << "Star backdrop fragment shader compilation failed:";
        qDebug().noquote() << starBackdropProgram->log();
    }

    bool starBackdropLinked = starBackdropProgram->link();

    if (!starBackdropLinked)
    {
        qDebug().noquote() << "Star backdrop shader link failed:";
        qDebug().noquote() << starBackdropProgram->log();
    }
    else
    {
        const float quadVerts[] = {
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        if (!starBackdropVao.create())
        {
            qDebug() << "Failed to create star backdrop VAO";
        }
        else
        {
            starBackdropVao.bind();

            if (!starBackdropVbo.create() || !starBackdropVbo.bind())
            {
                qDebug() << "Failed to create or bind star backdrop VBO";
            }
            else
            {
                //This mess took me a week ahhhhhhhhhh//
                starBackdropVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
                starBackdropVbo.allocate(quadVerts, static_cast<int>(sizeof(quadVerts)));

                starBackdropProgram->bind();
                starBackdropProgram->enableAttributeArray(0);
                starBackdropProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
                starBackdropProgram->enableAttributeArray(1);
                starBackdropProgram->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
                starBackdropProgram->release();
                starBackdropVbo.release();
            }

            starBackdropVao.release();
        }
    }

    // Earth keeps its day texture inside the reusable mesh helper.
    if (!earthMesh.Initialize(
        "C:/SOUL/assets/sphere-cylcoords-16k.obj",
        "C:/SOUL/assets/Texture/earth albedo.jpg",
        program))
    {
        qDebug() << "Failed to initialize Earth mesh";
        return;
    }

    // Load the extra night texture here because the Earth shader samples two maps.
    QImage nightImage("C:/SOUL/assets/Texture/2k_earth_nightmap.jpg");
    if (nightImage.isNull())
    {
        qDebug() << "Failed to load night texture";
    }
    else
    {
        nightImage = nightImage.convertToFormat(QImage::Format_RGBA8888);
        nightTexture = new QOpenGLTexture(QImage(nightImage).flipped());
        nightTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        nightTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        nightTexture->setWrapMode(QOpenGLTexture::Repeat);
    }

    if (!satelliteMesh.Initialize(
        "C:/SOUL/assets/SAT1.obj",
        "C:/SOUL/assets/Texture/10477_Satellite_v1_Diffuse.jpg",
        program))
    {
        qDebug() << "Failed to initialize satellite mesh";
    }

    // Build a tiny line shader because the Earth shader would throw a tantrum over plain vertices.
    orbitRingProgram = new QOpenGLShaderProgram(this);
    bool orbitRingVertexCreated = orbitRingProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        OrbitViewShaders::OrbitRingVertexShader());

    if (!orbitRingVertexCreated)
    {
        qDebug().noquote() << "Orbit ring vertex shader compilation failed:";
        qDebug().noquote() << orbitRingProgram->log();
    }

    bool orbitRingFragmentCreated = orbitRingProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        OrbitViewShaders::OrbitRingFragmentShader());

    if (!orbitRingFragmentCreated)
    {
        qDebug().noquote() << "Orbit ring fragment shader compilation failed:";
        qDebug().noquote() << orbitRingProgram->log();
    }

    bool orbitRingLinked = orbitRingProgram->link();

    if (!orbitRingLinked)
    {
        qDebug().noquote() << "Orbit ring shader link failed:";
        qDebug().noquote() << orbitRingProgram->log();
    }
    else
    {
        orbitRingVertexCount = 72;

        if (!orbitRingVao.create())
        {
            qDebug() << "Failed to create orbit ring VAO";
        }
        else
        {
            orbitRingVao.bind();

            if (!orbitRingVbo.create() || !orbitRingVbo.bind())
            {
                qDebug() << "Failed to create or bind orbit ring VBO";
            }
            else
            {
                // This buffer gets rewritten every frame because the visible trail follows a moving satellite.
                orbitRingVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                orbitRingVbo.allocate(orbitRingVertexCount * 3 * static_cast<int>(sizeof(float)));

                orbitRingProgram->bind();
                orbitRingProgram->enableAttributeArray(0);
                orbitRingProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));
                orbitRingProgram->release();
                orbitRingVbo.release();
            }

            orbitRingVao.release();
        }
    }

    satelliteGlowProgram = new QOpenGLShaderProgram(this);
    bool satelliteGlowVertexCreated = satelliteGlowProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        OrbitViewShaders::SatelliteGlowVertexShader());

    if (!satelliteGlowVertexCreated)
    {
        qDebug().noquote() << "Satellite glow vertex shader compilation failed:";
        qDebug().noquote() << satelliteGlowProgram->log();
    }

    bool satelliteGlowFragmentCreated = satelliteGlowProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        OrbitViewShaders::SatelliteGlowFragmentShader());

    if (!satelliteGlowFragmentCreated)
    {
        qDebug().noquote() << "Satellite glow fragment shader compilation failed:";
        qDebug().noquote() << satelliteGlowProgram->log();
    }

    bool satelliteGlowLinked = satelliteGlowProgram->link();

    if (!satelliteGlowLinked)
    {
        qDebug().noquote() << "Satellite glow shader link failed:";
        qDebug().noquote() << satelliteGlowProgram->log();
    }
    else
    {
        if (!satelliteGlowVao.create())
        {
            qDebug() << "Failed to create satellite glow VAO";
        }
        else
        {
            satelliteGlowVao.bind();

            if (!satelliteGlowVbo.create() || !satelliteGlowVbo.bind())
            {
                qDebug() << "Failed to create or bind satellite glow VBO";
            }
            else
            {
                satelliteGlowVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                satelliteGlowVbo.allocate(3 * static_cast<int>(sizeof(float)));

                satelliteGlowProgram->bind();
                satelliteGlowProgram->enableAttributeArray(0);
                satelliteGlowProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));
                satelliteGlowProgram->release();
                satelliteGlowVbo.release();
            }

            satelliteGlowVao.release();
        }
    }

    SetupSolarStormGraphics();
    RebuildStarBackdrop(width(), height());

    glEnable(GL_DEPTH_TEST);
    program->release();
}

void Orbitview::resizeGL(int w, int h)
{
    // Keep the GL viewport the same size as the widget box Qt gives us.
    glViewport(0, 0, w, h);
    RebuildStarBackdrop(w, h);
}

void Orbitview::paintGL()
{
    earthRotation += 0.03f;

    // Tiny fake frame step for now. Not perfect, but the satellites stay smooth enough and do not start tweaking.
    // TODO: Swap this for real delta time if we want speed to stay consistent on slower machines.
    float frameStep = 1.0f / 60.0f;

    // Everybody moves every frame now, even before the operator clicks anything.
    for (SatRenderData& satelliteState : satellites)
    {
        satelliteState.visual.Update(frameStep);
    }

    //TODO - Calculate Earth's proper rotation here using Quaternion.///

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the important GL state every frame so earlier fancy passes do not leak into Earth.
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (!program)
    {
        return;
    }

    QMatrix4x4 view = camera.GetViewMatrix();

    // This calculates the aspect ratio.
    float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
    QMatrix4x4 projection = camera.GetProjectionMatrix(aspect);

    UpdateSolarStorm(frameStep, view);
    if (repairGlowAmount > 0.0f)
    {
        // The repair pulse fades on its own after a short bit.
        // This keeps the feedback visible without leaving the satellite highlighted forever.
        repairGlowAmount = std::max(0.0f, repairGlowAmount - frameStep * 0.65f);
        if (repairGlowAmount <= 0.01f)
        {
            repairGlowAmount = 0.0f;
            repairSatName.clear();
        }
    }

    bool stormVisualActive = stormTintAmount > 0.01f;

    if (starBackdropProgram != nullptr && starBackdropTexture != nullptr && starBackdropVao.isCreated())
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        // Draw the fake star wallpaper first so everything else can cover it like a normal scene.
        starBackdropProgram->bind();
        starBackdropTexture->bind(0);
        starBackdropProgram->setUniformValue("uStarMap", 0);
        starBackdropVao.bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        starBackdropVao.release();
        starBackdropProgram->release();

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    // Earth draw pass
    program->bind();

    QMatrix4x4 model;
    model.setToIdentity();

    // Scale the sphere mesh down from OBJ space into our scene scale.
    model.scale(0.009f);

    // Add a small spin so the planet feels alive.
    model.rotate(earthRotation, 0.0f, 1.0f, 0.0f);

    QMatrix4x4 mvp = projection * view * model;

    QMatrix3x3 normalMatrix = model.normalMatrix();

    // Set the transforms and lighting used by the Earth pass.
    program->setUniformValue("uMVP", mvp);
    program->setUniformValue("uModel", model);
    program->setUniformValue("uNormalMatrix", normalMatrix);

    program->setUniformValue("uSunDir", QVector3D(-1.0f, 0.0f, 0.0f).normalized());
    program->setUniformValue("uViewPos", QVector3D(0.0f, 0.0f, 3.0f));
    program->setUniformValue("uAmbientFloor", 0.20f);
    program->setUniformValue("uNightStrength", 4.6f);
    program->setUniformValue("uNightBlend", 1.0f);
    program->setUniformValue("uTintColor", QVector3D(1.0f, 1.0f, 1.0f));
    program->setUniformValue("uTintStrength", 0.0f);

    if (nightTexture != nullptr)
    {
        nightTexture->bind(1);
        program->setUniformValue("uNightMap", 1);
    }

    earthMesh.Draw(program, 0, "uDayMap");
    DrawSolarStormPulse(view, projection);
    DrawSolarStormParticles(view, projection);
    program->bind();

    bool hasSelection = !selectedSatelliteName.isEmpty();

    // Now loop all satellites. If one is selected, the others sit out so the view stays clean.
    for (int satelliteIndex = 0; satelliteIndex < static_cast<int>(satellites.size()); ++satelliteIndex)
    {
        SatRenderData& satelliteState = satellites[satelliteIndex];
        SatelliteVisual& satelliteVisual = satelliteState.visual;
        bool isSelected = satelliteState.name == selectedSatelliteName;
        bool repairIsActiveForSat = satelliteState.name == repairSatName && repairGlowAmount > 0.01f;

        if (hasSelection && !isSelected)
        {
            continue;
        }

        QMatrix4x4 satelliteModel = satelliteVisual.GetModelMatrix();
        QMatrix4x4 satelliteMvp = projection * view * satelliteModel;
        QMatrix3x3 satelliteNormalMatrix = satelliteModel.normalMatrix();

        QVector3D trailColor = OrbitViewSceneHelper::TrailColorForSatellite(satelliteIndex);
        QVector3D glowColor = OrbitViewSceneHelper::GlowColorForSatellite(satelliteIndex);

        if (isSelected)
        {
            trailColor *= 1.20f;
            glowColor *= 1.25f;
        }

        if (isSelected && satelliteLinkStatus == "Degraded")
        {
            trailColor = QVector3D(0.88f, 0.72f, 0.32f);
            glowColor = QVector3D(1.0f, 0.82f, 0.38f);
        }

        if (isSelected && satelliteLinkStatus == "Disconnected")
        {
            trailColor = QVector3D(0.45f, 0.48f, 0.52f);
            glowColor = QVector3D(0.58f, 0.62f, 0.68f);
        }

        if (stormVisualActive)
        {
            QVector3D stormTrailColor = isSelected ? QVector3D(1.0f, 0.52f, 0.20f) : QVector3D(1.0f, 0.42f, 0.16f);
            QVector3D stormGlowColor = isSelected ? QVector3D(1.0f, 0.66f, 0.28f) : QVector3D(1.0f, 0.60f, 0.24f);
            trailColor = trailColor * (1.0f - stormTintAmount) + stormTrailColor * stormTintAmount;
            glowColor = glowColor * (1.0f - stormTintAmount) + stormGlowColor * stormTintAmount;
        }

        if (repairIsActiveForSat)
        {
            QVector3D repairTrailColor(0.60f, 0.98f, 0.78f);
            QVector3D repairGlowColor(0.82f, 1.0f, 0.86f);
            trailColor = trailColor * (1.0f - repairGlowAmount) + repairTrailColor * repairGlowAmount;
            glowColor = glowColor * (1.0f - repairGlowAmount) + repairGlowColor * repairGlowAmount;
        }

        // Orbit trail draw pass
        if (orbitRingProgram != nullptr && orbitRingVao.isCreated() && orbitRingVertexCount > 0)
        {
            std::vector<float> orbitTrailVertices;
            orbitTrailVertices.reserve(orbitRingVertexCount * 3);

            float trailSpan = 180.0f;

            // Open trail only, no full donut nonsense. It starts behind the satellite and keeps chasing it.
            for (int i = 0; i < orbitRingVertexCount; ++i)
            {
                float t = orbitRingVertexCount > 1
                    ? static_cast<float>(i) / static_cast<float>(orbitRingVertexCount - 1)
                    : 0.0f;
                float angleDeg = satelliteVisual.orbitAngle - t * trailSpan;
                QVector3D trailPoint = satelliteVisual.GetOrbitPositionForAngle(angleDeg);

                orbitTrailVertices.push_back(trailPoint.x());
                orbitTrailVertices.push_back(trailPoint.y());
                orbitTrailVertices.push_back(trailPoint.z());
            }

            orbitRingVbo.bind();
            orbitRingVbo.allocate(
                orbitTrailVertices.data(),
                static_cast<int>(orbitTrailVertices.size() * sizeof(float)));
            orbitRingVbo.release();

            orbitRingProgram->bind();
            orbitRingProgram->setUniformValue("uMVP", projection * view);
            orbitRingProgram->setUniformValue("uColor", trailColor);

            glLineWidth(2.0f);
            orbitRingVao.bind();
            glDrawArrays(GL_LINE_STRIP, 0, orbitRingVertexCount);
            orbitRingVao.release();
            orbitRingProgram->release();
            program->bind();
        }

        // Satellite draw pass.
        // Reuse the same shader with a different model transform for the orbiting satellite.
        program->setUniformValue("uMVP", satelliteMvp);
        program->setUniformValue("uModel", satelliteModel);
        program->setUniformValue("uNormalMatrix", satelliteNormalMatrix);
        program->setUniformValue("uAmbientFloor", 0.74f);
        program->setUniformValue("uNightStrength", 1.0f);
        program->setUniformValue("uNightBlend", 0.0f);
        program->setUniformValue("uNightMap", 0);
        program->setUniformValue(
            "uTintColor",
            stormVisualActive ? QVector3D(1.0f, 0.58f, 0.24f) : QVector3D(1.0f, 1.0f, 1.0f));
        float satStormTint = 0.48f * stormTintAmount;
        if (repairIsActiveForSat)
        {
            satStormTint *= (1.0f - repairGlowAmount * 0.90f);
        }
        program->setUniformValue("uTintStrength", satStormTint);
        satelliteMesh.Draw(program, 0, "uDayMap");

        if (satelliteGlowProgram != nullptr && satelliteGlowVao.isCreated())
        {
            QVector3D glowPos = satelliteVisual.GetOrbitPosition();
            float glowVertex[3] = {glowPos.x(), glowPos.y(), glowPos.z()};

            satelliteGlowVbo.bind();
            satelliteGlowVbo.allocate(glowVertex, static_cast<int>(sizeof(glowVertex)));
            satelliteGlowVbo.release();

            // Draw a small glow marker so the satellite stays easy to spot.
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);

            satelliteGlowProgram->bind();
            satelliteGlowProgram->setUniformValue("uMVP", projection * view);
            satelliteGlowProgram->setUniformValue("uColor", glowColor);
            float glowSize = isSelected ? 42.0f : 34.0f;
            if (repairIsActiveForSat)
            {
                glowSize += 14.0f * repairGlowAmount;
            }
            satelliteGlowProgram->setUniformValue("uPointSize", glowSize);
            satelliteGlowVao.bind();
            glDrawArrays(GL_POINTS, 0, 1);
            satelliteGlowVao.release();
            satelliteGlowProgram->release();

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            program->bind();
        }
    }

    program->release();

    // Keep the widget animating so the scene updates continuously.
    update();
}
