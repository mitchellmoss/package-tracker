TEMPLATE = app
TARGET = PackageTracker

QT += core gui widgets

SOURCES += main.cpp \
           mainwindow.cpp

HEADERS += mainwindow.h

# Specify Homebrew's Qt installation path
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
QMAKE_APPLE_DEVICE_ARCHS = arm64
QMAKE_LFLAGS += -F/opt/homebrew/lib
INCLUDEPATH += /opt/homebrew/include
LIBS += -F/opt/homebrew/lib -framework QtCore -framework QtGui -framework QtWidgets

# Silence SDK version warning
CONFIG += sdk_no_version_check
