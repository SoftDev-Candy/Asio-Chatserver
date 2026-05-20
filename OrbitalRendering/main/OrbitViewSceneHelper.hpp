#ifndef ORBITVIEWSCENEHELPER_HPP
#define ORBITVIEWSCENEHELPER_HPP

#include <QImage>
#include <QVector3D>

// Small scene helper for OrbitView.
// Keeps the color picks and star backdrop generation outside the widget so orbitview.cpp stays less crowded.
class OrbitViewSceneHelper
{
public:
    static QVector3D TrailColorForSatellite(int satelliteIndex);
    static QVector3D GlowColorForSatellite(int satelliteIndex);
    static QVector3D RandomStormColor();
    static QImage BuildStarBackdrop(int width, int height);
};

#endif // ORBITVIEWSCENEHELPER_HPP
