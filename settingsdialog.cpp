#include "settingsdialog.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("API Credentials");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    shippoTokenInput = new QLineEdit(this);
    formLayout->addRow("Shippo API Token:", shippoTokenInput);
    
    QPushButton* saveButton = new QPushButton("Save", this);
    connect(saveButton, &QPushButton::clicked, this, [this, parent]() {
        QString fedexKey = fedexKeyInput->text().trimmed();
        QString fedexSecret = fedexSecretInput->text().trimmed();
        QString upsId = upsIdInput->text().trimmed();
        QString upsSecret = upsSecretInput->text().trimmed();
        if (fedexKey.isEmpty() || fedexSecret.isEmpty() || 
            upsId.isEmpty() || upsSecret.isEmpty()) {
            QMessageBox::warning(this, "Invalid Credentials", 
                "All API credentials must be filled out");
            return;
        }
        
        QSettings settings;
        settings.setValue("fedexKey", fedexKey);
        settings.setValue("fedexSecret", fedexSecret);
        settings.setValue("upsId", upsId);
        settings.setValue("upsSecret", upsSecret);

        // Update clients with new credentials
        MainWindow* mainWindow = qobject_cast<MainWindow*>(parent);
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
