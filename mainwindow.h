#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSettings>
#include <QSystemTrayIcon>
#include "fedexclient.h"
#include "upsclient.h"
#include "settingsdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addPackage();
    void removePackage();
    void refreshPackages();
    void showPackageDetails(QListWidgetItem* item);

private:
    void setupUI();
    void loadPackages();
    void savePackages();
    
    QListWidget *packageList;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *refreshButton;
    QLineEdit *trackingInput;
    QTextEdit *detailsView;
    
    QSettings settings;
    FedExClient* fedexClient;
    UPSClient* upsClient;
    QMap<QString, QJsonObject> packageDetails;
    QSystemTrayIcon* trayIcon;
    SettingsDialog* settingsDialog;
};

#endif // MAINWINDOW_H
