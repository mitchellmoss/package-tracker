#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
#include <QInputDialog>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
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
#include <QLabel>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QToolButton>
#include <QCheckBox>
#include "archivedpackageswindow.h"

#define REFRESH_INTERVAL 900000 // 15 minutes
#define RETRY_DELAY 30000       // 30 seconds

// Implementation of FrostedGlassEffect
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
    
    auto blur = std::make_unique<QGraphicsBlurEffect>();
    blur->setBlurRadius(10);
    
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(temp));
    item.setGraphicsEffect(blur.get());
    scene.addItem(&item);
    
    scene.render(painter, QRectF(), QRectF(-offset.x(), -offset.y(), pixmap.width(), pixmap.height()));
}

// Implementation of PackageItemDelegate
PackageItemDelegate::PackageItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void PackageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    QString status = index.data(Qt::UserRole).toString().toUpper();
    QString note = index.data(Qt::UserRole + 1).toString();
    
    // Draw background
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    
    // Calculate positions
    QRect rect = opt.rect;
    int padding = 4;
    
    // Status color mapping
    QColor statusColor = QColor("#95a5a6"); // Default gray
    QString normalizedStatus = status.toUpper();
    if (index.data(Qt::DisplayRole).toString().startsWith("SHIPPO_")) {
        normalizedStatus = index.data(Qt::DisplayRole).toString().split("_")[1];
    }
    
    if (normalizedStatus == "DELIVERED") statusColor = QColor("#27ae60");
    else if (normalizedStatus == "TRANSIT") statusColor = QColor("#f39c12");
    else if (normalizedStatus == "PRE_TRANSIT") statusColor = QColor("#3498db");
    else if (normalizedStatus == "FAILURE") statusColor = QColor("#e74c3c");
    else if (normalizedStatus == "RETURNED") statusColor = QColor("#9b59b6");
    
    // Draw status indicator
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
    
    // Draw tracking number
    QRect textRect = rect;
    textRect.setLeft(iconRect.right() + padding * 2);
    QString trackingNumber = index.data(Qt::DisplayRole).toString();
    painter->drawText(textRect, Qt::AlignVCenter, trackingNumber);
    
    // Draw note if present
    if (!note.isEmpty()) {
        QFont italicFont = painter->font();
        italicFont.setItalic(true);
        painter->save();
        painter->setFont(italicFont);
        painter->setPen(QColor("#666666"));
        QRect noteRect = rect;
        noteRect.setLeft(textRect.left() + painter->fontMetrics().horizontalAdvance(trackingNumber) + padding * 4);
        painter->drawText(noteRect, Qt::AlignVCenter, "- " + note);
        painter->restore();
    }
}

QSize PackageItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(size.height() + 8);
    return size;
}

// MainWindow implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), mousePressed(false), isProcessingQueue(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    
    container = std::make_unique<QWidget>(this);
    container->setObjectName("container");
    
    initializeTimers();
    setupUI();
    setupTrayIcon();
    
    QString shippoToken = settings.value("shippoToken").toString();
    if (!shippoToken.isEmpty()) {
        shippoClient = std::make_unique<ShippoClient>(shippoToken, this);
        connectShippoSignals();
    }
    
    loadPackages();
    
    bool darkMode = settings.value("darkMode", false).toBool();
    applyTheme(darkMode);
    
    QTimer::singleShot(1000, this, &MainWindow::refreshPackages);
}

void MainWindow::initializeTimers()
{
    refreshTimer = std::make_unique<QTimer>(this);
    connect(refreshTimer.get(), &QTimer::timeout, this, &MainWindow::refreshPackages);
    refreshTimer->start(REFRESH_INTERVAL);
    
    retryTimer = std::make_unique<QTimer>(this);
    connect(retryTimer.get(), &QTimer::timeout, this, &MainWindow::retryFailedUpdates);
    retryTimer->start(RETRY_DELAY);
    
    queueProcessTimer = std::make_unique<QTimer>(this);
    connect(queueProcessTimer.get(), &QTimer::timeout, this, &MainWindow::processUpdateQueue);
    queueProcessTimer->start(1000);
}

void MainWindow::connectShippoSignals()
{
    if (!shippoClient) return;
    
    connect(shippoClient.get(), &ShippoClient::trackingInfoReceived, this, 
        [this](const QJsonObject& info) {
            QString trackingNumber = info["tracking_number"].toString();
            
            auto& package = packages[trackingNumber];
            package.details = info;
            package.status = info["status"].toString();
            package.retryCount = 0;
            
            updatePackageStatus(trackingNumber, package.status);
            
            if (packageList->currentItem() && packageList->currentItem()->text() == trackingNumber) {
                showPackageDetails(trackingNumber);
            }
        });
    
    connect(shippoClient.get(), &ShippoClient::trackingError, this, 
        [this](const QString& error) {
            // Get the current tracking number from the active update
            QString trackingNumber;
            if (!updateQueue.empty()) {
                trackingNumber = updateQueue.front();
            }
            
            if (!trackingNumber.isEmpty()) {
                auto it = packages.find(trackingNumber);
                if (it != packages.end()) {
                    auto& package = it.value();
                    package.retryCount++;
                    package.lastUpdateAttempt = QDateTime::currentDateTime();
                    
                    if (package.retryCount >= MAX_RETRY_ATTEMPTS) {
                        QMessageBox::warning(this, "Tracking Error", 
                            QString("Failed to update %1 after %2 attempts: %3")
                            .arg(trackingNumber)
                            .arg(MAX_RETRY_ATTEMPTS)
                            .arg(error));
                    }
                }
            }
        });
    
    connect(shippoClient.get(), &ShippoClient::webhookReceived, this, &MainWindow::handleWebhookEvent);
}

void MainWindow::setupUI()
{
    setupWindowControls();
    setupInputArea();
    setupPackageList();
    setupDetailsView();
    setupMenuBar();
    setupLayout();
    setupConnections();
    
    resize(900, 700);
    setMinimumSize(600, 400);
    
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

void MainWindow::setupWindowControls()
{
    auto titleBarLayout = std::make_unique<QHBoxLayout>();
    titleBarLayout->setContentsMargins(10, 5, 10, 5);
    titleBarLayout->setSpacing(8);
    
    auto closeButton = std::make_unique<QToolButton>(container.get());
    closeButton->setObjectName("closeButton");
    closeButton->setFixedSize(12, 12);
    closeButton->setStyleSheet("background-color: #ff5f56; border-radius: 6px; border: none;");
    connect(closeButton.get(), &QToolButton::clicked, this, &QMainWindow::close);
    
    auto minimizeButton = std::make_unique<QToolButton>(container.get());
    minimizeButton->setObjectName("minimizeButton");
    minimizeButton->setFixedSize(12, 12);
    minimizeButton->setStyleSheet("background-color: #ffbd2e; border-radius: 6px; border: none;");
    connect(minimizeButton.get(), &QToolButton::clicked, this, &QMainWindow::showMinimized);
    
    auto maximizeButton = std::make_unique<QToolButton>(container.get());
    maximizeButton->setObjectName("maximizeButton");
    maximizeButton->setFixedSize(12, 12);
    maximizeButton->setStyleSheet("background-color: #27c93f; border-radius: 6px; border: none;");
    connect(maximizeButton.get(), &QToolButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    
    auto titleLabel = std::make_unique<QLabel>("C++ Package Tracker by Mitchell Moss", container.get());
    titleLabel->setStyleSheet("color: black; font-weight: bold; margin-left: 10px;");
    
    titleBarLayout->addWidget(closeButton.release());
    titleBarLayout->addWidget(minimizeButton.release());
    titleBarLayout->addWidget(maximizeButton.release());
    titleBarLayout->addWidget(titleLabel.release());
    titleBarLayout->addStretch();
    
    containerLayout = std::make_unique<QVBoxLayout>(container.get());
    containerLayout->setContentsMargins(15, 15, 15, 15);
    containerLayout->setSpacing(10);
    containerLayout->addLayout(titleBarLayout.release());
}

void MainWindow::setupInputArea()
{
    auto inputLayout = std::make_unique<QHBoxLayout>();
    
    trackingInput = std::make_unique<QLineEdit>(this);
    trackingInput->setPlaceholderText("Enter test number (SHIPPO_TRANSIT, SHIPPO_DELIVERED, etc) or tracking number...");
    
    noteInput = std::make_unique<QLineEdit>(this);
    noteInput->setPlaceholderText("Add a note (optional)...");
    
    addButton = std::make_unique<QPushButton>("Add", this);
    removeButton = std::make_unique<QPushButton>("Remove", this);
    refreshButton = std::make_unique<QPushButton>("Refresh All", this);
    
    inputLayout->addWidget(trackingInput.get());
    inputLayout->addWidget(noteInput.get());
    inputLayout->addWidget(addButton.get());
    inputLayout->addWidget(removeButton.get());
    inputLayout->addWidget(refreshButton.get());
    
    containerLayout->addLayout(inputLayout.release());
}

void MainWindow::setupPackageList()
{
    packageList = std::make_unique<QListWidget>(this);
    packageList->setSelectionMode(QAbstractItemView::SingleSelection);
    packageList->setContextMenuPolicy(Qt::CustomContextMenu);
    packageList->setItemDelegate(new PackageItemDelegate(packageList.get()));
    
    // Create a horizontal layout for the toggle area
    auto toggleArea = new QWidget(this);
    auto toggleLayout = new QHBoxLayout(toggleArea);
    toggleLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create the checkbox with text directly on it
    auto toggleArchived = new QCheckBox("Show Archived Packages", toggleArea);
    toggleArchived->setChecked(false);
    toggleArchived->setStyleSheet(R"(
        QCheckBox {
            color: #2c3e50;
            font-size: 13px;
            padding: 5px;
        }
        QCheckBox:hover {
            background-color: rgba(52, 152, 219, 0.1);
            border-radius: 4px;
        }
    )");
    
    connect(toggleArchived, &QCheckBox::toggled, this, [this](bool checked) {
        showArchived = checked;
        refreshPackageList();
    });
    
    toggleLayout->addWidget(toggleArchived);
    toggleLayout->addStretch();
    
    containerLayout->addWidget(toggleArea);
    containerLayout->addWidget(packageList.get());
}

void MainWindow::setupDetailsView()
{
    detailsView = std::make_unique<QTextEdit>(this);
    detailsView->setReadOnly(true);
    containerLayout->addWidget(detailsView.get());
}

void MainWindow::setupMenuBar()
{
    auto menuBar = std::make_unique<QMenuBar>(this);
    auto settingsMenu = menuBar->addMenu("Settings");
    
    settingsMenu->addAction("API Credentials", this, [this]() {
        settingsDialog = std::make_unique<SettingsDialog>(this);
        connect(settingsDialog.get(), &SettingsDialog::accepted, this, [this]() {
            QString shippoToken = settingsDialog->shippoTokenInput->text();
            if (shippoToken.isEmpty()) {
                QMessageBox::warning(this, "Invalid Credentials", 
                    "Shippo API token must be provided");
                return;
            }
            updateApiClients(shippoToken);
        });
        settingsDialog->exec();
    });
    
    QMenu* viewMenu = menuBar->addMenu("View");
    QAction* showArchivedAction = viewMenu->addAction("Show Archived Packages", this, [this](){
        // Create and show the Archived Packages window (modal dialog)
        auto archivedWindow = new ArchivedPackagesWindow(this);
        connect(archivedWindow, &ArchivedPackagesWindow::requestUnarchive, this, &MainWindow::unarchivePackage);
        archivedWindow->exec();
    });
    
    setMenuBar(menuBar.release());
}

void MainWindow::setupLayout()
{
    auto shadow = std::make_unique<QGraphicsDropShadowEffect>();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(4, 4);
    container->setGraphicsEffect(shadow.release());
    
    setCentralWidget(container.get());
}

void MainWindow::setupConnections()
{
    connect(addButton.get(), &QPushButton::clicked, this, &MainWindow::addPackage);
    connect(removeButton.get(), &QPushButton::clicked, this, &MainWindow::removePackage);
    connect(refreshButton.get(), &QPushButton::clicked, this, &MainWindow::refreshPackages);
    
    packageList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(packageList.get(), &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Context menu"), this);
        
        // Get the item at the context position
        QListWidgetItem* item = packageList->itemAt(pos);
        if (!item)
            return;
        
        QAction *editNoteAction = contextMenu.addAction("Edit Note");
        connect(editNoteAction, &QAction::triggered, this, &MainWindow::editNote);
        
        // Add Archive/Unarchive action based on current state
        bool isArchived = item->data(Qt::UserRole + 2).toBool();
        QAction *archiveAction = contextMenu.addAction(isArchived ? "Unarchive" : "Archive");
        connect(archiveAction, &QAction::triggered, this, [this, item]() {
            QString trackingNumber = item->text();
            auto it = packages.find(trackingNumber);
            if (it != packages.end()) {
                it.value().archived = !it.value().archived; // Toggle archived status.
                item->setData(Qt::UserRole + 2, it.value().archived);
                // (Optionally, update the UI appearance of the item to reflect the archived state.)
            }
            savePackages();
        });
        
        contextMenu.exec(packageList->mapToGlobal(pos));
    });
    
    connect(packageList.get(), &QListWidget::itemClicked, this, 
        QOverload<QListWidgetItem*>::of(&MainWindow::showPackageDetails));
}

void MainWindow::setupTrayIcon()
{
    trayIcon = std::make_unique<QSystemTrayIcon>(this);
    
    QPixmap iconPixmap(64, 64);
    iconPixmap.fill(Qt::transparent);
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(100, 149, 237));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(8, 8, 48, 48);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 32));
    painter.drawText(iconPixmap.rect(), Qt::AlignCenter, "📦");
    
    trayIcon->setIcon(QIcon(iconPixmap));
    
    auto trayMenu = std::make_unique<QMenu>(this);
    trayMenu->addAction("Show", this, &QWidget::show);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    
    trayIcon->setContextMenu(trayMenu.release());
    trayIcon->show();
}

void MainWindow::addPackage()
{
    QString trackingNumber = trackingInput->text().trimmed();
    QString note = noteInput->text().trimmed();
    
    auto validatedNumber = validateTrackingNumber(trackingNumber);
    if (!validatedNumber) {
        QMessageBox::warning(this, "Invalid Tracking Number",
            "Please enter a valid tracking number or test number (SHIPPO_TRANSIT, etc)");
        return;
    }
    
    auto item = std::make_unique<QListWidgetItem>(*validatedNumber);
    item->setData(Qt::UserRole, "UNKNOWN");
    item->setData(Qt::UserRole + 1, note);
    
    PackageData packageData("UNKNOWN", note);
    packages[*validatedNumber] = packageData;
    
    packageList->insertItem(0, item.release());
    
    trackingInput->clear();
    noteInput->clear();
    
    savePackages();
    scheduleUpdate(*validatedNumber);
}

void MainWindow::removePackage()
{
    auto item = packageList->currentItem();
    if (!item) return;
    
    QString trackingNumber = item->text();
    packages.remove(trackingNumber);
    delete packageList->takeItem(packageList->row(item));
    savePackages();
}

void MainWindow::refreshPackages()
{
    if (!shippoClient) return;
    
    for (const auto& trackingNumber : packages.keys()) {
        scheduleUpdate(trackingNumber);
    }
}

void MainWindow::updatePackageStatus(const QString& trackingNumber, const QString& status)
{
    for (int i = 0; i < packageList->count(); ++i) {
        auto item = packageList->item(i);
        if (item->text() == trackingNumber) {
            QString oldStatus = item->data(Qt::UserRole).toString();
            
            if (oldStatus != status) {
                item->setData(Qt::UserRole, status);
                packageList->update(packageList->indexFromItem(item));
                
                QString notificationMsg = QString("Package %1 status changed from %2 to %3")
                    .arg(trackingNumber)
                    .arg(oldStatus)
                    .arg(status);
                showNotification("Package Status Update", notificationMsg);
            }
            break;
        }
    }
}

void MainWindow::showPackageDetails(QListWidgetItem* item)
{
    if (!item) return;
    showPackageDetails(item->text());
}

void MainWindow::showPackageDetails(const QString& trackingNumber)
{
    auto it = packages.find(trackingNumber);
    if (it == packages.end()) return;
    
    const auto& package = it.value();
    if (package.details.isEmpty()) {
        detailsView->setHtml(QString("<div style='color: %1; font-family: -apple-system;'>Loading details for: %2</div>")
            .arg(settings.value("darkMode", false).toBool() ? "#ffffff" : "#2c3e50")
            .arg(trackingNumber));
        
        if (shippoClient) {
            scheduleUpdate(trackingNumber);
        }
        return;
    }
    
    QString carrier = package.details["carrier"].toString();
    if (carrier.isEmpty()) carrier = "Unknown Carrier";
    
    bool isDarkMode = settings.value("darkMode", false).toBool();
    QString bgColor = isDarkMode ? "#2d2d2d" : "white";
    QString textColor = isDarkMode ? "#ffffff" : "#2c3e50";
    QString sectionBgColor = isDarkMode ? "#1e1e1e" : "#f8f9fa";
    QString borderColor = isDarkMode ? "rgba(255, 255, 255, 0.15)" : "rgba(0, 0, 0, 0.1)";
    
    QString details = formatPackageDetails(package.details, bgColor, borderColor, textColor, sectionBgColor);
    detailsView->setHtml(details);
}

QString MainWindow::formatPackageDetails(const QJsonObject& info, const QString& bgColor,
    const QString& borderColor, const QString& textColor, const QString& sectionBgColor)
{
    QString details = QString(
        "<div style='background: %1; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px %2;'>"
        "  <div style='display: flex; align-items: center; margin-bottom: 16px;'>"
        "    <div style='flex-grow: 1;'>"
        "      <h2 style='margin: 0; color: %3; font-size: 24px;'>%4</h2>"
        "      <p style='color: %5; margin: 4px 0 0 0; font-size: 16px;'>%6</p>"
        "    </div>"
        "  </div>"
    ).arg(bgColor).arg(borderColor).arg(textColor)
      .arg(info["tracking_number"].toString())
      .arg(textColor)
      .arg(info["carrier"].toString());
    
    QString statusColor = info["status"].toString().contains("Delivered") ? "#27ae60" : "#f39c12";
    details += formatStatusSection(info["status"].toString(), sectionBgColor, borderColor, textColor, statusColor);
    
    if (info.contains("estimatedDelivery")) {
        details += formatDeliverySection(info["estimatedDelivery"].toString(), 
            sectionBgColor, borderColor, textColor);
    }
    
    if (info.contains("events")) {
        details += formatTrackingHistory(info["events"].toArray(), 
            bgColor, borderColor, textColor);
    }
    
    details += "</div>";
    return details;
}

QString MainWindow::formatStatusSection(const QString& status, const QString& bgColor,
    const QString& borderColor, const QString& textColor, const QString& statusColor)
{
    return QString(
        "  <div style='background: %1; padding: 16px; border-radius: 8px; margin-bottom: 20px; border: 1px solid %2;'>"
        "    <h3 style='margin: 0 0 8px 0; color: %3; font-size: 18px;'>Current Status</h3>"
        "    <p style='margin: 0; color: %4; font-weight: 500; font-size: 16px;'>%5</p>"
        "  </div>"
    ).arg(bgColor).arg(borderColor).arg(textColor).arg(statusColor).arg(status);
}

QString MainWindow::formatDeliverySection(const QString& estimatedDelivery, const QString& bgColor,
    const QString& borderColor, const QString& textColor)
{
    return QString(
        "  <div style='background: %1; padding: 16px; border-radius: 8px; margin-bottom: 20px; border: 1px solid %2;'>"
        "    <h3 style='margin: 0 0 8px 0; color: %3; font-size: 18px;'>Estimated Delivery</h3>"
        "    <p style='margin: 0; color: %3; font-size: 16px;'>%4</p>"
        "  </div>"
    ).arg(bgColor).arg(borderColor).arg(textColor).arg(estimatedDelivery);
}

QString MainWindow::formatTrackingHistory(const QJsonArray& events, const QString& bgColor,
    const QString& borderColor, const QString& textColor)
{
    QString history = QString("<h3 style='margin: 0 0 16px 0; color: %1; font-size: 18px;'>Tracking History</h3>"
                            "<div style='max-height: 400px; overflow-y: auto;'>").arg(textColor);
    
    for (const auto& event : events) {
        QJsonObject e = event.toObject();
        QDateTime dateTime = QDateTime::fromString(e["timestamp"].toString(), "yyyyMMdd HHmmss");
        QString formattedDate = dateTime.toString("MMM d, yyyy h:mm AP");
        
        history += QString(
            "<div style='background: %1; padding: 16px; border-radius: 8px; margin-bottom: 12px; "
            "     border: 1px solid %2;'>"
            "  <div style='color: %3; font-size: 14px; margin-bottom: 4px;'>%4</div>"
            "  <div style='color: %3; font-weight: 500; font-size: 16px; margin-bottom: 4px;'>%5</div>"
            "  <div style='color: %3; font-size: 14px;'>%6</div>"
            "</div>"
        ).arg(bgColor).arg(borderColor).arg(textColor)
         .arg(formattedDate)
         .arg(e["description"].toString())
         .arg(e["location"].toString());
    }
    
    history += "</div>";
    return history;
}

void MainWindow::showNotification(const QString& title, const QString& message)
{
    if (trayIcon && QSystemTrayIcon::supportsMessages()) {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::handleWebhookEvent(const QString& event, const QJsonObject& data)
{
    if (event != "track_updated") return;
    
    QString trackingNumber = data["tracking_number"].toString();
    QJsonObject trackingStatus = data["tracking_status"].toObject();
    QString status = trackingStatus["status"].toString();
    QString details = trackingStatus["status_details"].toString();
    
    updatePackageStatus(trackingNumber, status);
    showPackageDetails(trackingNumber);
    
    QString notificationMsg = QString("Package %1: %2\n%3")
        .arg(trackingNumber)
        .arg(status)
        .arg(details);
    showNotification("Package Update", notificationMsg);
}

void MainWindow::editNote()
{
    auto currentItem = packageList->currentItem();
    if (!currentItem) return;
    
    QString trackingNumber = currentItem->text();
    QString currentNote = currentItem->data(Qt::UserRole + 1).toString();
    
    bool ok;
    QString newNote = QInputDialog::getText(this, "Edit Note",
        "Enter note for " + trackingNumber + ":",
        QLineEdit::Normal, currentNote, &ok);
    
    if (ok) {
        currentItem->setData(Qt::UserRole + 1, newNote);
        auto it = packages.find(trackingNumber);
        if (it != packages.end()) {
            it.value().note = newNote;
        }
        savePackages();
    }
}

void MainWindow::savePackages()
{
    QStringList packageListKeys;
    QMap<QString, QVariant> notes;
    QMap<QString, QVariant> archivedMap;
    
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        packageListKeys << it.key();
        notes[it.key()] = it.value().note;
        archivedMap[it.key()] = it.value().archived;
    }
    
    settings.setValue("trackingNumbers", packageListKeys);
    settings.setValue("packageNotes", notes);
    settings.setValue("packageArchived", archivedMap);
    settings.sync();
}

void MainWindow::loadPackages()
{
    QStringList savedPackages = settings.value("trackingNumbers").toStringList();
    QMap<QString, QVariant> notes = settings.value("packageNotes").toMap();
    QMap<QString, QVariant> archivedMap = settings.value("packageArchived").toMap();

    packages.clear(); // Clear any existing package data.
    for (const QString& trackingNumber : savedPackages) {
        bool isArchived = archivedMap.contains(trackingNumber) ? archivedMap[trackingNumber].toBool() : false;
        PackageData packageData("UNKNOWN", notes[trackingNumber].toString());
        packageData.archived = isArchived;
        packages[trackingNumber] = packageData;
    }
    // Instead of adding items here, refresh the list according to the current toggle.
    refreshPackageList();
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

MainWindow::~MainWindow()
{
    savePackages();
    cleanupResources();
}

void MainWindow::cleanupResources()
{
    if (refreshTimer) refreshTimer->stop();
    if (retryTimer) retryTimer->stop();
    if (queueProcessTimer) queueProcessTimer->stop();
    
    while (!updateQueue.empty()) {
        updateQueue.pop();
    }
}

std::optional<QString> MainWindow::validateTrackingNumber(const QString& number) const
{
    if (number.isEmpty()) {
        return std::nullopt;
    }
    
    static const QStringList validTestNumbers = {
        "SHIPPO_PRE_TRANSIT",
        "SHIPPO_TRANSIT",
        "SHIPPO_DELIVERED",
        "SHIPPO_RETURNED",
        "SHIPPO_FAILURE",
        "SHIPPO_UNKNOWN"
    };
    
    if (number.startsWith("SHIPPO_") && !validTestNumbers.contains(number)) {
        return std::nullopt;
    }
    
    return number;
}

void MainWindow::scheduleUpdate(const QString& trackingNumber)
{
    auto it = packages.find(trackingNumber);
    if (it != packages.end() && it.value().archived) {
        // Skip scheduling API update for archived packages.
        return;
    }
    
    if (!updateQueue.empty() && updateQueue.back() == trackingNumber) {
        return;
    }
    updateQueue.push(trackingNumber);
}

void MainWindow::applyTheme(bool darkMode)
{
    settings.setValue("darkMode", darkMode);
    settings.sync();
    
    QString bgColor = darkMode ? "#2d2d2d" : "white";
    QString textColor = darkMode ? "#ffffff" : "#2c3e50";
    QString borderColor = darkMode ? "rgba(255, 255, 255, 0.15)" : "rgba(0, 0, 0, 0.1)";
    
    // Main window styling
    container->setStyleSheet(QString(
        "QWidget#container {"
        "  background-color: %1;"
        "  border-radius: 10px;"
        "  border: 1px solid %2;"
        "}"
    ).arg(bgColor).arg(borderColor));
    
    // Input area styling
    QString inputStyle = QString(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton {"
        "  background-color: #3498db;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2980b9;"
        "}"
    ).arg(darkMode ? "#1e1e1e" : "#f8f9fa")
     .arg(textColor)
     .arg(borderColor);
    
    trackingInput->setStyleSheet(inputStyle);
    noteInput->setStyleSheet(inputStyle);
    addButton->setStyleSheet(inputStyle);
    removeButton->setStyleSheet(inputStyle);
    refreshButton->setStyleSheet(inputStyle);
    
    // Package list styling
    packageList->setStyleSheet(QString(
        "QListWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid %3;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #3498db;"
        "  color: white;"
        "}"
    ).arg(bgColor).arg(textColor).arg(borderColor));
    
    // Details view styling
    detailsView->setStyleSheet(QString(
        "QTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "}"
    ).arg(bgColor).arg(textColor).arg(borderColor));
    
    // If a package is currently selected, refresh its details to update colors
    if (packageList->currentItem()) {
        showPackageDetails(packageList->currentItem());
    }
}

void MainWindow::updateApiClients(const QString& shippoToken)
{
    settings.setValue("shippoToken", shippoToken);
    settings.sync();
    
    shippoClient = std::make_unique<ShippoClient>(shippoToken, this);
    connectShippoSignals();
    
    // Refresh all packages with new client
    refreshPackages();
}

void MainWindow::retryFailedUpdates()
{
    if (!shippoClient) return;
    
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        auto& package = it.value();
        if (package.retryCount > 0 && package.retryCount < MAX_RETRY_ATTEMPTS) {
            scheduleUpdate(it.key());
        }
    }
}

void MainWindow::processUpdateQueue()
{
    if (isProcessingQueue || updateQueue.empty() || !shippoClient) {
        return;
    }
    
    isProcessingQueue = true;
    QString trackingNumber = updateQueue.front();
    updateQueue.pop();
    
    auto it = packages.find(trackingNumber);
    if (it != packages.end()) {
        auto& package = it.value();
        
        if (package.lastUpdateAttempt.isValid() && 
            package.lastUpdateAttempt.secsTo(QDateTime::currentDateTime()) < RETRY_DELAY / 1000) {
            updateQueue.push(trackingNumber); // Re-queue for later
        } else {
            shippoClient->trackPackage(trackingNumber);
            package.lastUpdateAttempt = QDateTime::currentDateTime();
        }
    }
    
    isProcessingQueue = false;
}

void MainWindow::unarchivePackage(const QString& trackingNumber)
{
    auto it = packages.find(trackingNumber);
    if (it != packages.end()) {
        it.value().archived = false;
        savePackages();
        // Refresh the list so the unarchived package is removed when in "archived" view
        refreshPackageList();
    }
}

void MainWindow::refreshPackageList()
{
    packageList->clear();
    // Loop through all packages and add only those matching the current view.
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        if (it.value().archived == showArchived) {
            auto item = std::make_unique<QListWidgetItem>(it.key());
            item->setData(Qt::UserRole, it.value().status);
            item->setData(Qt::UserRole + 1, it.value().note);
            item->setData(Qt::UserRole + 2, it.value().archived);
            packageList->addItem(item.release());
        }
    }
}
