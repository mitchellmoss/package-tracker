#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    QLineEdit* shippoTokenInput;
    QLineEdit* webhookUrlInput;
    
private:
    void setupUI();
};

#endif // SETTINGSDIALOG_H
