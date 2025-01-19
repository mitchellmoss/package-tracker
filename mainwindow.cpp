#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Initialize FedEx client with your API credentials
    fedexClient = new FedExClient("YOUR_FEDEX_API_KEY", "YOUR_FEDEX_API_SECRET", this);
    connect(fedexClient, &FedExClient::trackingInfoReceived, this, [this](const QJsonObject& info) {
        QString trackingNumber = info["trackingNumber"].toString();
        packageDetails[trackingNumber] = info;
        showPackageDetails(trackingNumber);
    });
    connect(fedexClient, &FedExClient::trackingError, this, [this](const QString& error) {
        QMessageBox::warning(this, "Tracking Error", error);
    });

    setupUI();
    loadPackages();
}

MainWindow::~MainWindow()
{
    savePackages();
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
    
    // Add to main layout
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(packageList);
    mainLayout->addWidget(detailsView);
    
    setCentralWidget(centralWidget);
    
    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addPackage);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removePackage);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPackages);
    connect(packageList, &QListWidget::itemClicked, this, &MainWindow::showPackageDetails);
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
        fedexClient->trackPackage(trackingNumber);
    }
}

void MainWindow::showPackageDetails(QListWidgetItem *item)
{
    showPackageDetails(item->text());
}

void MainWindow::showPackageDetails(const QString& trackingNumber)
{
    if (packageDetails.contains(trackingNumber)) {
        QJsonObject info = packageDetails[trackingNumber];
        QString details = QString("Tracking Number: %1\nStatus: %2\n")
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
