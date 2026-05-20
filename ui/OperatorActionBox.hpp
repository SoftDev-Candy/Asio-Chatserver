#ifndef OPERATORACTIONBOX_HPP
#define OPERATORACTIONBOX_HPP

#include <QString>

class QLabel;
class QPushButton;
class QWidget;

// Small helper class for the hand-built operator controls.
// MainWindow was getting chunky, so this keeps the storm/repair widgets in one box (❁´◡`❁)
class OperatorActionBox
{
public:
    // Builds the warning label, buttons, and little feedback line on top of the main window.
    void Build(QWidget* parent);

    // Shows or hides the storm banner without making MainWindow babysit raw labels all day.
    void SetStormWarningVisible(bool visible);

    // Turns the repair button on or off depending on whether a satellite is selected.
    void SetRepairEnabled(bool enabled);

    // Gives the operator a short status line like "repair sent" or "backend sleeping".
    void ShowMessage(const QString& message);

    // Hides the status line when the screen is back in a neutral state.
    void HideMessage();

    // These accessors let MainWindow hook up button clicks while this helper owns the widgets.
    QPushButton* StormButton() const;
    QPushButton* ResetButton() const;
    QPushButton* RepairButton() const;

private:
    QLabel* stormWarningLabel = nullptr;
    QLabel* operatorMessageLabel = nullptr;
    QPushButton* stormButton = nullptr;
    QPushButton* resetButton = nullptr;
    QPushButton* repairButton = nullptr;
};

#endif // OPERATORACTIONBOX_HPP
