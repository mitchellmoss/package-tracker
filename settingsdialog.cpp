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
    webhookUrlInput = new QLineEdit(this);
    formLayout->addRow("Shippo API Token:", shippoTokenInput);
    formLayout->addRow("Webhook URL:", webhookUrlInput);
    
    QPushButton* saveButton = new QPushButton("Save", this);
    connect(saveButton, &QPushButton::clicked, this, [this, parent]() {
        QString shippoToken = shippoTokenInput->text().trimmed();
        QString webhookUrl = webhookUrlInput->text().trimmed();
        
        if (shippoToken.isEmpty()) {
            QMessageBox::warning(this, "Invalid Credentials", 
                "Shippo API token must be provided");
            return;
        }
        
        QSettings settings;
        settings.setValue("shippoToken", shippoToken);
        settings.setValue("webhookUrl", webhookUrl);

        // Update client with new credentials
        MainWindow* mainWindow = qobject_cast<MainWindow*>(parent);
        if (mainWindow) {
            mainWindow->updateApiClients(shippoToken);
        }
        
        accept();
    });
    
    // Load existing values
    QSettings settings;
    shippoTokenInput->setText(settings.value("shippoToken").toString());
    webhookUrlInput->setText(settings.value("webhookUrl").toString());
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(saveButton);
    setLayout(mainLayout);
}
