TEMPLATE = app
TARGET = PackageTracker

QT += core gui widgets network
CONFIG += ssl

SOURCES += main.cpp \
           mainwindow.cpp \
           fedexclient.cpp \
           upsclient.cpp \
           settingsdialog.cpp

HEADERS += mainwindow.h \
           fedexclient.h \
           upsclient.h \
           settingsdialog.h

# Specify Homebrew's Qt installation path
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
QMAKE_APPLE_DEVICE_ARCHS = arm64
QMAKE_LFLAGS += -F/opt/homebrew/lib
INCLUDEPATH += /opt/homebrew/include
LIBS += -F/opt/homebrew/lib -framework QtCore -framework QtGui -framework QtWidgets

# Silence SDK version warning
CONFIG += sdk_no_version_check

# Add resources
RESOURCES += resources.qrc
