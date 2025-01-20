#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    QLineEdit* shippoTokenInput;
    QLineEdit* webhookUrlInput;
    QCheckBox* darkModeCheckbox;
    
    // Add method to update theme
    void updateTheme(bool darkMode);
    
private:
    void setupUI();
    QPushButton* saveButton;  // Add this to access the save button for theming
};

#endif // SETTINGSDIALOG_H
