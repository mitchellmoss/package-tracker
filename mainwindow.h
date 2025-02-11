#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Base Qt includes
#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QSettings>
#include <QTimer>
#include <QMap>
#include <QPoint>

// Qt JSON
#include <QJsonObject>
#include <QJsonArray>

// Qt GUI
#include <QMainWindow>
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsEffect>
#include <QGraphicsScene>
#include <QScreen>

// Qt Widgets
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QStyle>
#include <QLabel>

// STL
#include <memory>
#include <optional>
#include <queue>

// Project headers
#include "shippoclient.h"
#include "settingsdialog.h"

// Forward declarations
class ShippoClient;
class SettingsDialog;
class PackageUpdateWorker;

// Constants
constexpr int REFRESH_INTERVAL = 5 * 60 * 1000; // 5 minutes
constexpr int MAX_RETRY_ATTEMPTS = 3;
constexpr int RETRY_DELAY = 5000; // 5 seconds

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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void updateApiClients(const QString& shippoToken);
    void applyTheme(bool darkMode);
    void unarchivePackage(const QString& trackingNumber);
    void refreshPackageList();

private slots:
    void addPackage();
    void removePackage();
    void refreshPackages();
    void editNote();
    void showPackageDetails(QListWidgetItem* item);
    void showPackageDetails(const QString& trackingNumber);
    void setupTrayIcon();
    void updatePackageStatus(const QString& trackingNumber, const QString& status);
    void showNotification(const QString& title, const QString& message);
    void retryFailedUpdates();
    void processUpdateQueue();
    void connectShippoSignals();

private:
    struct PackageData {
        QString status = "UNKNOWN";
        QString note;
        QJsonObject details;
        int retryCount = 0;
        QDateTime lastUpdateAttempt;
        bool archived = false;
        
        PackageData() = default;
        PackageData(const QString& s, const QString& n) 
            : status(s), note(n) {}
    };

    // Setup functions
    void setupUI();
    void setupWindowControls();
    void setupInputArea();
    void setupPackageList();
    void setupDetailsView();
    void setupMenuBar();
    void setupLayout();
    void setupConnections();
    void setupSearchBar();
    
    // Core functionality
    void loadPackages();
    void savePackages();
    void initializeTimers();
    void cleanupResources();
    std::optional<QString> validateTrackingNumber(const QString& number) const;
    void scheduleUpdate(const QString& trackingNumber);
    
    // Package details formatting
    QString formatPackageDetails(const QJsonObject& info, const QString& bgColor,
        const QString& borderColor, const QString& textColor, const QString& sectionBgColor);
    QString formatStatusSection(const QString& status, const QString& bgColor,
        const QString& borderColor, const QString& textColor, const QString& statusColor);
    QString formatDeliverySection(const QString& estimatedDelivery, const QString& bgColor,
        const QString& borderColor, const QString& textColor);
    QString formatTrackingHistory(const QJsonArray& events, const QString& bgColor,
        const QString& borderColor, const QString& textColor);

public:
    void handleWebhookEvent(const QString& event, const QJsonObject& data);
    QString detectCarrier(const QString& trackingNumber);

private:
    // UI Components
    std::unique_ptr<QListWidget> packageList;
    std::unique_ptr<QPushButton> addButton;
    std::unique_ptr<QPushButton> removeButton;
    std::unique_ptr<QPushButton> refreshButton;
    std::unique_ptr<QLineEdit> trackingInput;
    std::unique_ptr<QLineEdit> noteInput;
    std::unique_ptr<QLineEdit> searchBar;
    std::unique_ptr<QTextEdit> detailsView;
    
    // Core Components
    QSettings settings;
    std::unique_ptr<ShippoClient> shippoClient;
    std::unique_ptr<QSystemTrayIcon> trayIcon;
    std::unique_ptr<SettingsDialog> settingsDialog;
    std::unique_ptr<QWidget> container;
    std::unique_ptr<QVBoxLayout> containerLayout;
    
    // Data Storage
    QMap<QString, PackageData> packages;
    std::queue<QString> updateQueue;
    
    // Timers
    std::unique_ptr<QTimer> refreshTimer;
    std::unique_ptr<QTimer> retryTimer;
    std::unique_ptr<QTimer> queueProcessTimer;
    
    // State
    QPoint dragPosition;
    bool mousePressed;
    bool isProcessingQueue;
    
    // New member variable to track if archived packages are shown.
    bool showArchived = false;
    
    // Mouse event handlers
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif // MAINWINDOW_H
