#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>  // Add this include
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
    
    // Create a container widget for the frosted effect
    container = new QWidget(this);
    container->setObjectName("container");
    
    // Apply frosted glass effect to container
    FrostedGlassEffect* effect = new FrostedGlassEffect(container);
    container->setGraphicsEffect(effect);
    
    // Main layout for container
    containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(15, 15, 15, 15);
    containerLayout->setSpacing(10);
    
    // Set window styles
    setStyleSheet(R"(
        #container {
            background-color: rgba(245, 245, 245, 0.95);
            border-radius: 10px;
            border: 1px solid rgba(200, 200, 200, 0.5);
        }
        QMainWindow {
            background-color: transparent;
        }
        QListWidget {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 8px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 5px;
        }
        QTextEdit {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 8px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 10px;
        }
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 5px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 5px;
        }
        QPushButton {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 5px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 5px 10px;
            min-width: 60px;
            color: black;
        }
        QPushButton:hover {
            background-color: rgba(240, 240, 240, 1.0);
            color: black;
        }
        QMenuBar {
            background-color: transparent;
            border-bottom: 1px solid rgba(200, 200, 200, 0.3);
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 5px 10px;
        }
        QMenuBar::item:selected {
            background-color: rgba(200, 200, 200, 0.3);
            border-radius: 4px;
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
    
    // Create a proper icon
    QPixmap iconPixmap(64, 64);
    iconPixmap.fill(Qt::transparent);
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(100, 149, 237)); // Cornflower blue
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(8, 8, 48, 48);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 32));
    painter.drawText(iconPixmap.rect(), Qt::AlignCenter, "📦");
    
    trayIcon->setIcon(QIcon(iconPixmap));
    
    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &QWidget::show);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void MainWindow::setupUI()
{
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

    // Add widgets to container
    containerLayout->addLayout(inputLayout);
    containerLayout->addWidget(packageList);
    containerLayout->addWidget(detailsView);
    
    // Add drop shadow to container
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(4, 4);
    container->setGraphicsEffect(shadow);
    
    setCentralWidget(container);
    
    // Adjust window size and position
    resize(900, 700);
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Set minimum size
    setMinimumSize(600, 400);
    
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
