TEMPLATE = app
TARGET = PackageTracker

QT       += core gui widgets network
CONFIG   += c++17

# Use Qt's built-in module paths
QMAKE_MAC_SDK = macosx

# Network configuration for Shippo API
DEFINES += SHIPPO_API_BASE_URL=\\\"https://api.goshippo.com\\\"

# Add icon - use absolute path
ICON = $$PWD/icons/app.icns

SOURCES += main.cpp \
           mainwindow.cpp \
           shippoclient.cpp \
           settingsdialog.cpp \
           archivedpackageswindow.cpp

HEADERS += mainwindow.h \
           shippoclient.h \
           settingsdialog.h \
           archivedpackageswindow.h

# macOS specific configuration
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
QMAKE_APPLE_DEVICE_ARCHS = arm64

# Qt configuration for Homebrew
QT_PREFIX = $$system(brew --prefix qt)
INCLUDEPATH += $$QT_PREFIX/lib/QtCore.framework/Headers
INCLUDEPATH += $$QT_PREFIX/lib/QtGui.framework/Headers
INCLUDEPATH += $$QT_PREFIX/lib/QtWidgets.framework/Headers
LIBS += -F$$QT_PREFIX/lib

# Silence SDK version warning
CONFIG += sdk_no_version_check

# Add resources
RESOURCES += resources.qrc
