TEMPLATE = app
TARGET = PackageTracker

QT += core gui widgets network
CONFIG += ssl

# Add Shippo package
INCLUDEPATH += $$system(dotnet list package | grep Shippo | cut -d' ' -f1)

SOURCES += main.cpp \
           mainwindow.cpp \
           fedexclient.cpp \
           upsclient.cpp \
           settingsdialog.cpp

HEADERS += mainwindow.h \
           fedexclient.h \
           upsclient.h \
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
