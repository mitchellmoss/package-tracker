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
#include <QLabel>  // Add this include for QLabel

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
    : QMainWindow(parent), mousePressed(false)
{
    // Enable translucent background
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    
    // Create a container widget for the frosted effect
    container = new QWidget(this);
    container->setObjectName("container");
    
    // Add window controls
    QHBoxLayout* titleBarLayout = new QHBoxLayout();
    titleBarLayout->setContentsMargins(10, 5, 10, 5);
    titleBarLayout->setSpacing(8);
    
    // Close button
    QToolButton* closeButton = new QToolButton(container);
    closeButton->setObjectName("closeButton");
    closeButton->setFixedSize(12, 12);
    closeButton->setStyleSheet(
        "background-color: #ff5f56;"
        "border-radius: 6px;"
        "border: none;"
        "margin: 0;"
        "padding: 0;"
    );
    
    // Minimize button
    QToolButton* minimizeButton = new QToolButton(container);
    minimizeButton->setObjectName("minimizeButton");
    minimizeButton->setFixedSize(12, 12);
    minimizeButton->setStyleSheet(
        "background-color: #ffbd2e;"
        "border-radius: 6px;"
        "border: none;"
        "margin: 0;"
        "padding: 0;"
    );
    
    // Maximize button
    QToolButton* maximizeButton = new QToolButton(container);
    maximizeButton->setObjectName("maximizeButton");
    maximizeButton->setFixedSize(12, 12);
    maximizeButton->setStyleSheet(
        "background-color: #27c93f;"
        "border-radius: 6px;"
        "border: none;"
        "margin: 0;"
        "padding: 0;"
    );
    
    // Add buttons to layout
    titleBarLayout->addWidget(closeButton);
    titleBarLayout->addWidget(minimizeButton);
    titleBarLayout->addWidget(maximizeButton);
    
    // Add title
    QLabel* titleLabel = new QLabel("C++ Package Tracker by Mitchell Moss", container);
    titleLabel->setStyleSheet("color: black; font-weight: bold; margin-left: 10px;");
    titleBarLayout->addWidget(titleLabel);
    titleBarLayout->addStretch();
    
    connect(closeButton, &QToolButton::clicked, this, &QMainWindow::close);
    connect(minimizeButton, &QToolButton::clicked, this, &QMainWindow::showMinimized);
    connect(maximizeButton, &QToolButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    
    // Apply frosted glass effect to container
    FrostedGlassEffect* effect = new FrostedGlassEffect(container);
    container->setGraphicsEffect(effect);
    
    // Main layout for container
    containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(15, 15, 15, 15);
    containerLayout->setSpacing(10);
    containerLayout->addLayout(titleBarLayout);
    
    // Set window styles
    setStyleSheet(R"(
        #container {
            background-color: rgba(255, 255, 255, 0.98);
            border-radius: 12px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            /* box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1); */
        }
        QMainWindow {
            background-color: transparent;
        }
        QListWidget {
            background-color: white;
            border-radius: 8px;
            border: 1px solid rgba(0, 0, 0, 0.08);
            padding: 5px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid rgba(0, 0, 0, 0.05);
        }
        QListWidget::item:hover {
            background-color: rgba(0, 0, 0, 0.03);
        }
        QListWidget::item:selected {
            background-color: rgba(0, 120, 212, 0.1);
            color: black;
        }
        QTextEdit {
            background-color: white;
            border-radius: 8px;
            border: 1px solid rgba(0, 0, 0, 0.08);
            padding: 12px;
            color: #333;
            font-size: 13px;
            line-height: 1.4;
        }
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.9);
            border-radius: 5px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            padding: 5px;
            color: black;
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
        #closeButton, #minimizeButton, #maximizeButton {
            border: none;
            padding: 0;
            margin: 0 4px;
            border-radius: 6px;
            min-width: 12px;
            min-height: 12px;
            max-width: 12px;
            max-height: 12px;
        }
        #closeButton:hover { background-color: #e0443e; }
        #minimizeButton:hover { background-color: #e6a723; }
        #maximizeButton:hover { background-color: #1faf2f; }
        QMenuBar::item {
            background-color: transparent;
            padding: 5px 10px;
        }
        QMenuBar::item:selected {
            background-color: rgba(200, 200, 200, 0.3);
            border-radius: 4px;
        }
    )");
    // Initialize API clients with saved credentials
    QSettings settings;
    QString fedexKey = settings.value("fedexKey").toString();
    QString fedexSecret = settings.value("fedexSecret").toString();
    QString upsId = settings.value("upsId").toString();
    QString upsSecret = settings.value("upsSecret").toString();
    QString upsAccessKey = settings.value("upsAccessKey").toString();

    qDebug() << "Loading credentials:";
    qDebug() << "FedEx Key:" << (fedexKey.isEmpty() ? "Not set" : "Set");
    qDebug() << "FedEx Secret:" << (fedexSecret.isEmpty() ? "Not set" : "Set");
    qDebug() << "UPS ID:" << (upsId.isEmpty() ? "Not set" : "Set");
    qDebug() << "UPS Secret:" << (upsSecret.isEmpty() ? "Not set" : "Set");
    qDebug() << "UPS Access Key:" << (upsAccessKey.isEmpty() ? "Not set" : "Set");

    fedexClient = new FedExClient(fedexKey, fedexSecret, this);
    upsClient = new UPSClient(upsId, upsSecret, upsAccessKey, this);
    
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

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        mousePressed = true;
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (mousePressed && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressed = false;
        event->accept();
    }
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
        SettingsDialog* dialog = new SettingsDialog(this);
        connect(dialog, &SettingsDialog::accepted, this, [this, dialog]() {
            QString fedexKey = dialog->fedexKeyInput->text();
            QString fedexSecret = dialog->fedexSecretInput->text();
            QString upsId = dialog->upsIdInput->text();
            QString upsSecret = dialog->upsSecretInput->text();
        
            if (fedexKey.isEmpty() || fedexSecret.isEmpty() || 
                upsId.isEmpty() || upsSecret.isEmpty()) {
                QMessageBox::warning(this, "Invalid Credentials", 
                    "All API credentials must be filled out");
                return;
            }
        
            updateApiClients(fedexKey, fedexSecret, upsId, upsSecret);
        });
        dialog->exec();
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
        QString carrier = detectCarrier(trackingNumber);
        
        if (carrier == "UPS") {
            upsClient->trackPackage(trackingNumber);
        } else if (carrier == "FedEx") {
            fedexClient->trackPackage(trackingNumber);
        } else {
            qDebug() << "Unknown carrier for tracking number:" << trackingNumber;
        }
    }
}

QString MainWindow::detectCarrier(const QString& trackingNumber)
{
    // UPS tracking numbers start with 1Z and are 18 digits
    if (trackingNumber.startsWith("1Z") && trackingNumber.length() == 18) {
        return "UPS";
    }
    // FedEx tracking numbers are 12, 15, or 20 digits
    else if (trackingNumber.length() == 12 || trackingNumber.length() == 15 || trackingNumber.length() == 20) {
        return "FedEx";
    }
    return "Unknown";
}

void MainWindow::showPackageDetails(QListWidgetItem *item)
{
    this->showPackageDetails(item->text());
}

void MainWindow::showPackageDetails(const QString& trackingNumber)
{
    QString carrier = detectCarrier(trackingNumber);
    
    if (packageDetails.contains(trackingNumber)) {
        QJsonObject info = packageDetails[trackingNumber];
        QString carrier = info.contains("carrier") ? info["carrier"].toString() : "Unknown Carrier";
        QString details = QString("<h2 style='margin: 0; padding: 0;'>%1</h2>"
                                "<p style='color: #666; margin: 4px 0;'>%2</p>"
                                "<div style='margin: 16px 0; padding: 12px; background: %3; border-radius: 6px;'>"
                                "  <h3 style='margin: 0 0 8px 0;'>Current Status</h3>"
                                "  <p style='margin: 0;'>%4</p>"
                                "</div>")
            .arg(trackingNumber)
            .arg(carrier)
            .arg(info["status"].toString().contains("Delivered") ? "#e8f5e9" : "#fff3e0")
            .arg(info["status"].toString());
        
        if (info.contains("estimatedDelivery")) {
            details += QString("<div style='margin: 16px 0; padding: 12px; background: #e3f2fd; border-radius: 6px;'>"
                             "  <h3 style='margin: 0 0 8px 0;'>Estimated Delivery</h3>"
                             "  <p style='margin: 0;'>%1</p>"
                             "</div>")
                .arg(info["estimatedDelivery"].toString());
        }
        
        if (info.contains("events")) {
            details += "<h3 style='margin: 16px 0 8px 0;'>Tracking History</h3>";
            QJsonArray events = info["events"].toArray();
            for (const QJsonValue& event : events) {
                QJsonObject e = event.toObject();
                details += QString("<div style='margin: 8px 0; padding: 12px; background: white; border-radius: 6px; border: 1px solid rgba(0, 0, 0, 0.08);'>"
                                 "  <div style='color: #666; font-size: 0.9em;'>%1</div>"
                                 "  <div style='margin: 4px 0; font-weight: 500;'>%2</div>"
                                 "  <div style='color: #666; font-size: 0.9em;'>%3</div>"
                                 "</div>")
                    .arg(e["timestamp"].toString())
                    .arg(e["description"].toString())
                    .arg(e["location"].toString());
            }
        }
        
        detailsView->setHtml("<div style='padding: 8px;'>" + details + "</div>");
        
        detailsView->setText(details);
    } else {
        detailsView->setText("Loading details for: " + trackingNumber);
        
        QString carrier = detectCarrier(trackingNumber);
        if (carrier == "UPS") {
            upsClient->trackPackage(trackingNumber);
        } else if (carrier == "FedEx") {
            fedexClient->trackPackage(trackingNumber);
        } else {
            detailsView->setText("Unknown carrier for tracking number: " + trackingNumber);
        }
    }
}

void MainWindow::loadPackages()
{
    QStringList packages = settings.value("trackingNumbers").toStringList();
    packageList->addItems(packages);
}

void MainWindow::updateApiClients(const QString& fedexKey, const QString& fedexSecret,
                                 const QString& upsId, const QString& upsSecret,
                                 const QString& upsAccessKey)
{
    fedexClient->deleteLater();
    upsClient->deleteLater();
    
    fedexClient = new FedExClient(fedexKey, fedexSecret, this);
    upsClient = new UPSClient(upsId, upsSecret, upsAccessKey, this);

    // Reconnect signals
    connect(fedexClient, &FedExClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["trackingNumber"].toString();
        packageDetails[trackingNumber] = info;
        packageDetails[trackingNumber]["carrier"] = "FedEx";
        this->showPackageDetails(trackingNumber);
    });
    connect(fedexClient, &FedExClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "FedEx Tracking Error", error);
    });

    connect(upsClient, &UPSClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["trackingNumber"].toString();
        packageDetails[trackingNumber] = info;
        packageDetails[trackingNumber]["carrier"] = "UPS";
        this->showPackageDetails(trackingNumber);
    });
    connect(upsClient, &UPSClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "UPS Tracking Error", error);
    });
}

void MainWindow::savePackages()
{
    QStringList packages;
    for (int i = 0; i < packageList->count(); ++i) {
        packages << packageList->item(i)->text();
    }
    settings.setValue("trackingNumbers", packages);
}
