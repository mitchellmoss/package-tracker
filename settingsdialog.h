#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    QLineEdit* fedexKeyInput;
    QLineEdit* fedexSecretInput;
    QLineEdit* upsIdInput;
    QLineEdit* upsSecretInput;
};

#endif // SETTINGSDIALOG_H
