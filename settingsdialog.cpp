#include "settingsdialog.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("API Credentials");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    fedexKeyInput = new QLineEdit(this);
    fedexSecretInput = new QLineEdit(this);
    upsIdInput = new QLineEdit(this);
    upsSecretInput = new QLineEdit(this);
    
    formLayout->addRow("FedEx API Key:", fedexKeyInput);
    formLayout->addRow("FedEx API Secret:", fedexSecretInput);
    formLayout->addRow("UPS Client ID:", upsIdInput);
    formLayout->addRow("UPS Client Secret:", upsSecretInput);
    
    QPushButton* saveButton = new QPushButton("Save", this);
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        QSettings settings;
        settings.setValue("fedexKey", fedexKeyInput->text());
        settings.setValue("fedexSecret", fedexSecretInput->text());
        settings.setValue("upsId", upsIdInput->text());
        settings.setValue("upsSecret", upsSecretInput->text());
        
        // Update clients with new credentials
        MainWindow* mainWindow = qobject_cast<MainWindow*>(parent());
        if (mainWindow) {
            mainWindow->updateApiClients(
                fedexKeyInput->text(),
                fedexSecretInput->text(),
                upsIdInput->text(),
                upsSecretInput->text()
            );
        }
        
        accept();
    });
    
    // Load existing values
    QSettings settings;
    fedexKeyInput->setText(settings.value("fedexKey").toString());
    fedexSecretInput->setText(settings.value("fedexSecret").toString());
    upsIdInput->setText(settings.value("upsId").toString());
    upsSecretInput->setText(settings.value("upsSecret").toString());
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(saveButton);
    setLayout(mainLayout);
}
