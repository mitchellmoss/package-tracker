#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QScreen>
#include <QWindow>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QTimer>

void FrostedGlassEffect::draw(QPainter* painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::PadToEffectiveBoundingRect);
    
    QImage temp(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
    temp.fill(0);
    
    QPainter tempPainter(&temp);
    tempPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tempPainter.drawPixmap(offset, pixmap);
    tempPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tempPainter.fillRect(temp.rect(), QColor(255, 255, 255, 180));
    tempPainter.end();
    
    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(10);
    
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(temp));
    item.setGraphicsEffect(blur);
    scene.addItem(&item);
    
    scene.render(painter, QRectF(), QRectF(-offset.x(), -offset.y(), pixmap.width(), pixmap.height()));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Enable translucent background
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    
    // Create frosted glass effect
    FrostedGlassEffect* effect = new FrostedGlassEffect(this);
    setGraphicsEffect(effect);
    
    // Set window background color with transparency
    setStyleSheet(R"(
        QMainWindow {
            background-color: rgba(255, 255, 255, 0.7);
            border-radius: 10px;
        }
        QListWidget, QTextEdit, QLineEdit {
            background-color: rgba(255, 255, 255, 0.8);
            border-radius: 5px;
            padding: 5px;
        }
        QPushButton {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 5px;
            padding: 5px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 1.0);
        }
    )");
    // Initialize API clients with your credentials
    fedexClient = new FedExClient("YOUR_FEDEX_API_KEY", "YOUR_FEDEX_API_SECRET", this);
    upsClient = new UPSClient("YOUR_UPS_CLIENT_ID", "YOUR_UPS_CLIENT_SECRET", this);
    // Connect FedEx signals
    connect(fedexClient, &FedExClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["trackingNumber"].toString();
        packageDetails[trackingNumber] = info;
        packageDetails[trackingNumber]["carrier"] = "FedEx";
        this->showPackageDetails(trackingNumber);
    });
    connect(fedexClient, &FedExClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "FedEx Tracking Error", error);
    });

    // Connect UPS signals
    connect(upsClient, &UPSClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["trackingNumber"].toString();
        packageDetails[trackingNumber] = info;
        packageDetails[trackingNumber]["carrier"] = "UPS";
        this->showPackageDetails(trackingNumber);
    });
    connect(upsClient, &UPSClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "UPS Tracking Error", error);
    });

    setupUI();
    setupTrayIcon();
    loadPackages();
    
    // Auto-refresh every 15 minutes
    QTimer* refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshPackages);
    refreshTimer->start(15 * 60 * 1000);
}

MainWindow::~MainWindow()
{
    savePackages();
}

void MainWindow::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon::fromTheme("package"));
    
    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &QWidget::show);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void MainWindow::setupUI()
{
    // Main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    trackingInput = new QLineEdit(this);
    trackingInput->setPlaceholderText("Enter tracking number...");
    
    addButton = new QPushButton("Add", this);
    removeButton = new QPushButton("Remove", this);
    refreshButton = new QPushButton("Refresh All", this);
    
    inputLayout->addWidget(trackingInput);
    inputLayout->addWidget(addButton);
    inputLayout->addWidget(removeButton);
    inputLayout->addWidget(refreshButton);
    
    // Package list
    packageList = new QListWidget(this);
    
    // Details view
    detailsView = new QTextEdit(this);
    detailsView->setReadOnly(true);
    
    // Add menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* settingsMenu = menuBar->addMenu("Settings");
    settingsMenu->addAction("API Credentials", this, [this]() {
        settingsDialog = new SettingsDialog(this);
        settingsDialog->exec();
    });
    setMenuBar(menuBar);

    // Add to main layout
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(packageList);
    mainLayout->addWidget(detailsView);
    
    // Add drop shadow
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 0);
    centralWidget->setGraphicsEffect(shadow);
    
    setCentralWidget(centralWidget);
    
    // Adjust window size and position
    resize(800, 600);
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addPackage);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removePackage);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPackages);
    connect(packageList, &QListWidget::itemClicked, this, QOverload<QListWidgetItem*>::of(&MainWindow::showPackageDetails));
}

void MainWindow::addPackage()
{
    QString trackingNumber = trackingInput->text().trimmed();
    if (!trackingNumber.isEmpty()) {
        packageList->addItem(trackingNumber);
        trackingInput->clear();
        savePackages();
    }
}

void MainWindow::removePackage()
{
    QListWidgetItem *item = packageList->currentItem();
    if (item) {
        delete packageList->takeItem(packageList->row(item));
        savePackages();
    }
}

void MainWindow::refreshPackages()
{
    for (int i = 0; i < packageList->count(); ++i) {
        QString trackingNumber = packageList->item(i)->text();
        
        // Try to detect carrier based on tracking number format
        if (trackingNumber.startsWith("1Z")) {
            upsClient->trackPackage(trackingNumber);
        } else {
            fedexClient->trackPackage(trackingNumber);
        }
    }
}

void MainWindow::showPackageDetails(QListWidgetItem *item)
{
    this->showPackageDetails(item->text());
}

void MainWindow::showPackageDetails(const QString& trackingNumber)
{
    if (packageDetails.contains(trackingNumber)) {
        QJsonObject info = packageDetails[trackingNumber];
        QString carrier = info.contains("carrier") ? info["carrier"].toString() : "Unknown Carrier";
        QString details = QString("Carrier: %1\nTracking Number: %2\nStatus: %3\n")
            .arg(carrier)
            .arg(trackingNumber)
            .arg(info["status"].toString());
        
        if (info.contains("estimatedDelivery")) {
            details += QString("Estimated Delivery: %1\n")
                .arg(info["estimatedDelivery"].toString());
        }
        
        if (info.contains("events")) {
            details += "\nTracking History:\n";
            QJsonArray events = info["events"].toArray();
            for (const QJsonValue& event : events) {
                QJsonObject e = event.toObject();
                details += QString("- %1: %2\n")
                    .arg(e["timestamp"].toString())
                    .arg(e["description"].toString());
            }
        }
        
        detailsView->setText(details);
    } else {
        detailsView->setText("Loading details for: " + trackingNumber);
        fedexClient->trackPackage(trackingNumber);
    }
}

void MainWindow::loadPackages()
{
    QStringList packages = settings.value("trackingNumbers").toStringList();
    packageList->addItems(packages);
}

void MainWindow::savePackages()
{
    QStringList packages;
    for (int i = 0; i < packageList->count(); ++i) {
        packages << packageList->item(i)->text();
    }
    settings.setValue("trackingNumbers", packages);
}
