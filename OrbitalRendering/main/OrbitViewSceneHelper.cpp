#include "OrbitViewSceneHelper.hpp"

#include <QColor>
#include <QPainter>
#include <QRandomGenerator>
#include <random>

QVector3D OrbitViewSceneHelper::TrailColorForSatellite(int satelliteIndex)
{
    switch (satelliteIndex)
    {
    case 0:
        return QVector3D(0.44f, 0.76f, 0.96f);
    case 1:
        return QVector3D(0.96f, 0.72f, 0.32f);
    default:
        return QVector3D(0.56f, 0.90f, 0.55f);
    }
}

QVector3D OrbitViewSceneHelper::GlowColorForSatellite(int satelliteIndex)
{
    switch (satelliteIndex)
    {
    case 0:
        return QVector3D(0.76f, 0.92f, 1.0f);
    case 1:
        return QVector3D(1.0f, 0.86f, 0.48f);
    default:
        return QVector3D(0.76f, 1.0f, 0.72f);
    }
}

QVector3D OrbitViewSceneHelper::RandomStormColor()
{
    double roll = QRandomGenerator::global()->generateDouble();

    if (roll < 0.34)
    {
        return QVector3D(1.0f, 0.78f, 0.28f);
    }

    if (roll < 0.68)
    {
        return QVector3D(1.0f, 0.44f, 0.12f);
    }

    return QVector3D(0.92f, 0.18f, 0.12f);
}

QImage OrbitViewSceneHelper::BuildStarBackdrop(int width, int height)
{
    constexpr int starCount = 3200;

    QImage starImage(width, height, QImage::Format_RGBA8888);
    starImage.fill(QColor(13, 20, 31, 255));

    QPainter painter(&starImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    std::mt19937 rng(42u);
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(width));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(height));
    std::uniform_real_distribution<float> sizeDist(0.55f, 1.35f);
    std::uniform_int_distribution<int> alphaDist(95, 180);
    std::uniform_int_distribution<int> coolTintDist(0, 26);

    for (int i = 0; i < starCount; ++i)
    {
        float x = xDist(rng);
        float y = yDist(rng);
        float size = sizeDist(rng);
        int tint = coolTintDist(rng);
        QColor starColor(214 + tint / 3, 224 + tint / 2, 235 + tint, alphaDist(rng));

        painter.setBrush(starColor);
        painter.drawEllipse(QRectF(x, y, size, size));
    }

    painter.end();
    return starImage;
}
