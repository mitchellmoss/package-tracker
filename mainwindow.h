#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QMenu>
#include <QCoreApplication>
#include <QGraphicsEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include "shippoclient.h"
#include "settingsdialog.h"

// Forward declarations
class ShippoClient;
class SettingsDialog;

class FrostedGlassEffect : public QGraphicsEffect
{
    Q_OBJECT
public:
    FrostedGlassEffect(QObject* parent = nullptr) : QGraphicsEffect(parent) {}
protected:
    void draw(QPainter* painter) override;
};

class PackageItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PackageItemDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

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
    void showPackageDetails(const QString& trackingNumber);
    void setupTrayIcon();
    void updatePackageStatus(const QString& trackingNumber, const QString& status);

private:
    void setupUI();
    void loadPackages();
    void savePackages();
public:
    void updateApiClients(const QString& shippoToken);
    QString detectCarrier(const QString& trackingNumber);
    
    QListWidget *packageList;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *refreshButton;
    QLineEdit *trackingInput;
    QTextEdit *detailsView;
    
    QSettings settings;
    ShippoClient* shippoClient = nullptr;
    QMap<QString, QJsonObject> packageDetails;
    QSystemTrayIcon* trayIcon;
    SettingsDialog* settingsDialog;
    QWidget* container;
    QVBoxLayout* containerLayout;
    QPoint dragPosition;
    bool mousePressed;
    
    // Mouse event handlers
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif // MAINWINDOW_H
