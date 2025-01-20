#include "settingsdialog.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QCheckBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("API Credentials");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    shippoTokenInput = new QLineEdit(this);
    webhookUrlInput = new QLineEdit(this);
    darkModeCheckbox = new QCheckBox("Dark Mode", this);
    
    // Set object names for styling
    shippoTokenInput->setObjectName("settingsInput");
    webhookUrlInput->setObjectName("settingsInput");
    darkModeCheckbox->setObjectName("settingsCheckbox");
    
    formLayout->addRow("Shippo API Token:", shippoTokenInput);
    formLayout->addRow("Webhook URL:", webhookUrlInput);
    formLayout->addRow(darkModeCheckbox);
    
    saveButton = new QPushButton("Save", this);
    saveButton->setObjectName("settingsSaveButton");
    
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

    // Apply initial theme
    updateTheme(settings.value("darkMode", false).toBool());
}

void SettingsDialog::updateTheme(bool darkMode)
{
    QString lightStyle = R"(
        QDialog {
            background-color: white;
            color: #333333;
        }
        QLineEdit#settingsInput {
            background-color: white;
            color: #333333;
            border: 1px solid rgba(0, 0, 0, 0.15);
            border-radius: 5px;
            padding: 5px;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QCheckBox#settingsCheckbox {
            color: #333333;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QCheckBox#settingsCheckbox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid rgba(0, 0, 0, 0.15);
            border-radius: 3px;
            background-color: white;
        }
        QCheckBox#settingsCheckbox::indicator:checked {
            background-color: #0078d4;
            border-color: #0078d4;
        }
        QPushButton#settingsSaveButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 16px;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QPushButton#settingsSaveButton:hover {
            background-color: #106ebe;
        }
        QPushButton#settingsSaveButton:pressed {
            background-color: #005a9e;
        }
        QLabel {
            color: #333333;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
    )";

    QString darkStyle = R"(
        QDialog {
            background-color: #1e1e1e;
            color: #ffffff;
        }
        QLineEdit#settingsInput {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid rgba(255, 255, 255, 0.15);
            border-radius: 5px;
            padding: 5px;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QCheckBox#settingsCheckbox {
            color: #ffffff;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QCheckBox#settingsCheckbox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid rgba(255, 255, 255, 0.15);
            border-radius: 3px;
            background-color: #2d2d2d;
        }
        QCheckBox#settingsCheckbox::indicator:checked {
            background-color: #0078d4;
            border-color: #0078d4;
        }
        QPushButton#settingsSaveButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 16px;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
        QPushButton#settingsSaveButton:hover {
            background-color: #106ebe;
        }
        QPushButton#settingsSaveButton:pressed {
            background-color: #005a9e;
        }
        QLabel {
            color: #ffffff;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            font-size: 13px;
        }
    )";

    setStyleSheet(darkMode ? darkStyle : lightStyle);
}
