TEMPLATE = app
TARGET = PackageTracker

QT += core gui widgets network
CONFIG += ssl

# Network configuration for Shippo API
DEFINES += SHIPPO_API_BASE_URL=\\\"https://api.goshippo.com\\\"

# Add icon - use absolute path
ICON = $$PWD/icons/app.icns

SOURCES += main.cpp \
           mainwindow.cpp \
           shippoclient.cpp \
           settingsdialog.cpp

HEADERS += mainwindow.h \
           shippoclient.h \
           settingsdialog.h

# macOS specific configuration
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
QMAKE_APPLE_DEVICE_ARCHS = arm64

# Qt configuration for Homebrew
QT_PREFIX = $$system(brew --prefix qt)
INCLUDEPATH += $$QT_PREFIX/include
LIBS += -L$$QT_PREFIX/lib

# Silence SDK version warning
CONFIG += sdk_no_version_check

# Add resources
RESOURCES += resources.qrc
