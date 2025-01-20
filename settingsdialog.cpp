#include "settingsdialog.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QCheckBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("API Credentials");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    shippoTokenInput = new QLineEdit(this);
    webhookUrlInput = new QLineEdit(this);
    darkModeCheckbox = new QCheckBox("Dark Mode", this);
    
    formLayout->addRow("Shippo API Token:", shippoTokenInput);
    formLayout->addRow("Webhook URL:", webhookUrlInput);
    formLayout->addRow(darkModeCheckbox);
    
    QPushButton* saveButton = new QPushButton("Save", this);
    connect(saveButton, &QPushButton::clicked, this, [this, parent]() {
        QString shippoToken = shippoTokenInput->text().trimmed();
        QString webhookUrl = webhookUrlInput->text().trimmed();
        bool darkMode = darkModeCheckbox->isChecked();
        
        if (shippoToken.isEmpty()) {
            QMessageBox::warning(this, "Invalid Credentials", 
                "Shippo API token must be provided");
            return;
        }
        
        QSettings settings;
        settings.setValue("shippoToken", shippoToken);
        settings.setValue("webhookUrl", webhookUrl);
        settings.setValue("darkMode", darkMode);

        // Update client with new credentials
        MainWindow* mainWindow = qobject_cast<MainWindow*>(parent);
        if (mainWindow) {
            mainWindow->updateApiClients(shippoToken);
            mainWindow->applyTheme(darkMode);
        }
        
        accept();
    });
    
    // Load existing values
    QSettings settings;
    shippoTokenInput->setText(settings.value("shippoToken").toString());
    webhookUrlInput->setText(settings.value("webhookUrl").toString());
    darkModeCheckbox->setChecked(settings.value("darkMode", false).toBool());
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(saveButton);
    setLayout(mainLayout);
}
