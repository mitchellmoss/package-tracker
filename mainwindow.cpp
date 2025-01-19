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
    // TODO: Implement API calls to refresh all packages
    QMessageBox::information(this, "Info", "Refresh functionality not yet implemented");
}

void MainWindow::showPackageDetails(QListWidgetItem *item)
{
    // TODO: Implement API call to get package details
    detailsView->setText("Package details for: " + item->text());
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
