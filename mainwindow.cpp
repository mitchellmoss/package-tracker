#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
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
            color: black;  /* Ensure text color is black */
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid rgba(0, 0, 0, 0.05);
            color: black;  /* Ensure text color is black */
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
    // Initialize Shippo client with saved credentials
    QSettings settings;
    QString shippoToken = settings.value("shippoToken").toString();
    
    qDebug() << "Loading credentials:";
    qDebug() << "Shippo Token:" << (shippoToken.isEmpty() ? "Not set" : "Set");

    if (!shippoToken.isEmpty()) {
        shippoClient = new ShippoClient(shippoToken, this);
        
        // Connect Shippo signals
        connect(shippoClient, &ShippoClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
            QString trackingNumber = info["tracking_number"].toString();
            packageDetails[trackingNumber] = info;
            updatePackageStatus(trackingNumber, info["tracking_status"].toObject()["status"].toString());
            this->showPackageDetails(trackingNumber);
        });
        
        connect(shippoClient, &ShippoClient::trackingError, this, [this](const QString& error) {
            QMessageBox::warning(this, "Tracking Error", error);
        });
    }

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

PackageItemDelegate::PackageItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void PackageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    // Get the status from the item's data
    QString status = index.data(Qt::UserRole).toString();
    
    // Draw the background
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    
    // Calculate text and icon positions
    QRect rect = opt.rect;
    int padding = 4;
    
    // Draw the status icon
    QColor statusColor;
    if (status == "Delivered") {
        statusColor = QColor("#27ae60"); // Green
    } else if (status == "In Transit") {
        statusColor = QColor("#f39c12"); // Orange
    } else {
        statusColor = QColor("#95a5a6"); // Gray
    }
    
    QRect iconRect = rect;
    iconRect.setWidth(16);
    iconRect.setHeight(16);
    iconRect.moveTop(rect.top() + (rect.height() - iconRect.height()) / 2);
    iconRect.moveLeft(rect.left() + padding);
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(statusColor);
    painter->drawEllipse(iconRect);
    painter->restore();
    
    // Draw the text
    QRect textRect = rect;
    textRect.setLeft(iconRect.right() + padding * 2);
    QString text = index.data(Qt::DisplayRole).toString();
    painter->drawText(textRect, Qt::AlignVCenter, text);
}

QSize PackageItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(size.height() + 8); // Add some vertical padding
    return size;
}

void MainWindow::setupUI()
{
    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    trackingInput = new QLineEdit(this);
    trackingInput->setPlaceholderText("Enter test number (SHIPPO_TRANSIT, SHIPPO_DELIVERED, etc) or tracking number...");
    
    addButton = new QPushButton("Add", this);
    removeButton = new QPushButton("Remove", this);
    refreshButton = new QPushButton("Refresh All", this);
    
    inputLayout->addWidget(trackingInput);
    inputLayout->addWidget(addButton);
    inputLayout->addWidget(removeButton);
    inputLayout->addWidget(refreshButton);
    
    // Package list
    packageList = new QListWidget(this);
    packageList->setItemDelegate(new PackageItemDelegate(packageList));
    
    // Details view
    detailsView = new QTextEdit(this);
    detailsView->setReadOnly(true);
    
    // Add menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* settingsMenu = menuBar->addMenu("Settings");
    settingsMenu->addAction("API Credentials", this, [this]() {
        SettingsDialog* dialog = new SettingsDialog(this);
        connect(dialog, &SettingsDialog::accepted, this, [this, dialog]() {
            QString shippoToken = dialog->shippoTokenInput->text();
            if (shippoToken.isEmpty()) {
                QMessageBox::warning(this, "Invalid Credentials", 
                    "Shippo API token must be provided");
                return;
            }
        
            updateApiClients(shippoToken);
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
        QListWidgetItem* item = new QListWidgetItem(trackingNumber);
        item->setData(Qt::UserRole, "Pending");
        packageList->addItem(item);
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
        
        if (shippoClient) {
            shippoClient->trackPackage(trackingNumber);
        } else {
            qDebug() << "Unknown carrier for tracking number:" << trackingNumber;
        }
    }
}

QString MainWindow::detectCarrier(const QString& trackingNumber)
{
    static const QStringList validTestNumbers = {
        "SHIPPO_PRE_TRANSIT",
        "SHIPPO_TRANSIT",
        "SHIPPO_DELIVERED",
        "SHIPPO_RETURNED", 
        "SHIPPO_FAILURE",
        "SHIPPO_UNKNOWN"
    };
    
    // Validate test tracking numbers
    if (trackingNumber.startsWith("SHIPPO_")) {
        if (!validTestNumbers.contains(trackingNumber)) {
            QMessageBox::warning(nullptr, "Invalid Test Number",
                "Valid test numbers are: SHIPPO_PRE_TRANSIT, SHIPPO_TRANSIT, "
                "SHIPPO_DELIVERED, SHIPPO_RETURNED, SHIPPO_FAILURE, SHIPPO_UNKNOWN");
            return QString();
        }
        return "shippo";
    }
    
    // For now, assume all other numbers are shippo
    return "shippo";
}

void MainWindow::showPackageDetails(QListWidgetItem *item)
{
    this->showPackageDetails(item->text());
}

void MainWindow::updatePackageStatus(const QString& trackingNumber, const QString& status)
{
    // Find and update the list item
    for (int i = 0; i < packageList->count(); ++i) {
        QListWidgetItem* item = packageList->item(i);
        if (item->text() == trackingNumber) {
            item->setData(Qt::UserRole, status);
            packageList->update(packageList->indexFromItem(item));
            break;
        }
    }
}

void MainWindow::showPackageDetails(const QString& trackingNumber)
{
    QString carrier = detectCarrier(trackingNumber);
    
    if (packageDetails.contains(trackingNumber)) {
        QJsonObject info = packageDetails[trackingNumber];
        QString carrier = info.contains("carrier") ? info["carrier"].toString() : "Unknown Carrier";
        
        // Format the tracking number with carrier logo/icon
        QString details = QString(
            "<div style='background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);'>"
            "  <div style='display: flex; align-items: center; margin-bottom: 16px;'>"
            "    <div style='flex-grow: 1;'>"
            "      <h2 style='margin: 0; color: #2c3e50; font-size: 24px;'>%1</h2>"
            "      <p style='color: #7f8c8d; margin: 4px 0 0 0; font-size: 16px;'>%2</p>"
            "    </div>"
            "  </div>"
        ).arg(trackingNumber).arg(carrier);

        // Current Status Section
        QString statusColor = info["status"].toString().contains("Delivered") ? "#27ae60" : "#f39c12";
        details += QString(
            "  <div style='background: #f8f9fa; padding: 16px; border-radius: 8px; margin-bottom: 20px;'>"
            "    <h3 style='margin: 0 0 8px 0; color: #2c3e50; font-size: 18px;'>Current Status</h3>"
            "    <p style='margin: 0; color: %1; font-weight: 500; font-size: 16px;'>%2</p>"
            "  </div>"
        ).arg(statusColor).arg(info["status"].toString());

        // Estimated Delivery Section
        if (info.contains("estimatedDelivery")) {
            details += QString(
                "  <div style='background: #f8f9fa; padding: 16px; border-radius: 8px; margin-bottom: 20px;'>"
                "    <h3 style='margin: 0 0 8px 0; color: #2c3e50; font-size: 18px;'>Estimated Delivery</h3>"
                "    <p style='margin: 0; color: #2c3e50; font-size: 16px;'>%1</p>"
                "  </div>"
            ).arg(info["estimatedDelivery"].toString());
        }

        // Tracking History Section
        if (info.contains("events")) {
            details += "<h3 style='margin: 0 0 16px 0; color: #2c3e50; font-size: 18px;'>Tracking History</h3>";
            details += "<div style='max-height: 400px; overflow-y: auto;'>";
            
            QJsonArray events = info["events"].toArray();
            for (const QJsonValue& event : events) {
                QJsonObject e = event.toObject();
                
                // Parse and format the timestamp
                QString timestamp = e["timestamp"].toString();
                QDateTime dateTime = QDateTime::fromString(timestamp, "yyyyMMdd HHmmss");
                QString formattedDate = dateTime.toString("MMM d, yyyy h:mm AP");
                
                details += QString(
                    "<div style='background: white; padding: 16px; border-radius: 8px; margin-bottom: 12px; "
                    "     border: 1px solid #e0e0e0;'>"
                    "  <div style='color: #7f8c8d; font-size: 14px; margin-bottom: 4px;'>%1</div>"
                    "  <div style='color: #2c3e50; font-weight: 500; font-size: 16px; margin-bottom: 4px;'>%2</div>"
                    "  <div style='color: #7f8c8d; font-size: 14px;'>%3</div>"
                    "</div>"
                ).arg(formattedDate)
                 .arg(e["description"].toString())
                 .arg(e["location"].toString());
            }
            details += "</div>";
        }
        
        details += "</div>"; // Close main container
        
        detailsView->setHtml("<div style='padding: 8px;'>" + details + "</div>");
    } else {
        detailsView->setText("Loading details for: " + trackingNumber);
        
        QString carrier = detectCarrier(trackingNumber);
        if (shippoClient) {
            shippoClient->trackPackage(trackingNumber);
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

void MainWindow::updateApiClients(const QString& shippoToken)
{
    if (shippoClient) {
        shippoClient->deleteLater();
    }
    
    shippoClient = new ShippoClient(shippoToken, this);

    // Connect signals
    connect(shippoClient, &ShippoClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["tracking_number"].toString();
        packageDetails[trackingNumber] = info;
        this->showPackageDetails(trackingNumber);
    });
    
    connect(shippoClient, &ShippoClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "Tracking Error", error);
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
