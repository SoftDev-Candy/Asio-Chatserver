#include "OperatorActionBox.hpp"

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <Qt>

void OperatorActionBox::Build(QWidget* parent)
{
    if (parent == nullptr || stormButton != nullptr)
    {
        return;
    }

    // Build once only.
    // If this ran twice we would stack duplicate buttons on top of each other, which would get messy fast (┬┬﹏┬┬)
    stormWarningLabel = new QLabel("SOLAR STORM ACTIVE", parent);
    stormWarningLabel->setGeometry(440, 232, 260, 28);
    stormWarningLabel->setAlignment(Qt::AlignCenter);
    stormWarningLabel->setStyleSheet(
        "QLabel {"
        " color: rgb(255, 236, 214);"
        " background-color: rgba(153, 36, 14, 215);"
        " border: 1px solid rgba(255, 145, 84, 220);"
        " border-radius: 12px;"
        " font-weight: 700;"
        " padding: 4px 10px;"
        "}");
    stormWarningLabel->hide();
    stormWarningLabel->raise();

    stormButton = new QPushButton("Trigger Solar Storm", parent);
    stormButton->setGeometry(720, 230, 158, 34);
    stormButton->setCursor(Qt::PointingHandCursor);

    resetButton = new QPushButton("Reset Scenario", parent);
    resetButton->setGeometry(888, 230, 150, 34);
    resetButton->setCursor(Qt::PointingHandCursor);

    repairButton = new QPushButton("Repair Selected Satellite", parent);
    repairButton->setGeometry(720, 270, 318, 34);
    repairButton->setCursor(Qt::PointingHandCursor);
    repairButton->setEnabled(false);

    operatorMessageLabel = new QLabel(parent);
    operatorMessageLabel->setGeometry(720, 308, 318, 24);
    operatorMessageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    operatorMessageLabel->setStyleSheet(
        "QLabel {"
        " color: rgb(223, 232, 241);"
        " background: transparent;"
        " font-weight: 600;"
        "}");
    operatorMessageLabel->hide();

    const QString buttonStyle =
        "QPushButton {"
        " background-color: rgba(32, 44, 64, 228);"
        " color: rgb(236, 240, 245);"
        " border: 1px solid rgba(255, 138, 76, 150);"
        " border-radius: 10px;"
        " padding: 6px 12px;"
        " font-weight: 600;"
        "}"
        "QPushButton:hover {"
        " background-color: rgba(74, 48, 34, 235);"
        " border: 1px solid rgba(255, 162, 92, 220);"
        "}"
        "QPushButton:pressed {"
        " background-color: rgba(113, 52, 23, 235);"
        "}";

    stormButton->setStyleSheet(buttonStyle);
    resetButton->setStyleSheet(buttonStyle);
    repairButton->setStyleSheet(buttonStyle);

    stormButton->raise();
    resetButton->raise();
    repairButton->raise();
    operatorMessageLabel->raise();
}

void OperatorActionBox::SetStormWarningVisible(bool visible)
{
    if (stormWarningLabel == nullptr)
    {
        return;
    }

    // If the label was never built, there is nothing to toggle yet. Tiny guard  doing its job (❁´◡`❁)
    stormWarningLabel->setVisible(visible);
    stormWarningLabel->raise();
}

void OperatorActionBox::SetRepairEnabled(bool enabled)
{
    if (repairButton == nullptr)
    {
        return;
    }

    // The repair button only makes sense once a satellite is actually picked. Otherwise we are just pressing buttons for vibes.
    repairButton->setEnabled(enabled);
}

void OperatorActionBox::ShowMessage(const QString& message)
{
    if (operatorMessageLabel == nullptr)
    {
        return;
    }

    operatorMessageLabel->setText(message);
    operatorMessageLabel->show();
    operatorMessageLabel->raise();
}

void OperatorActionBox::HideMessage()
{
    if (operatorMessageLabel == nullptr)
    {
        return;
    }

    operatorMessageLabel->hide();
}

QPushButton* OperatorActionBox::StormButton() const
{
    return stormButton;
}

QPushButton* OperatorActionBox::ResetButton() const
{
    return resetButton;
}

QPushButton* OperatorActionBox::RepairButton() const
{
    return repairButton;
}
