QT += core gui qml quick quickcontrols2
CONFIG += c++17
TARGET = tbar
TEMPLATE = app
SOURCES += src/main.cpp src/backend.cpp src/desktop.cpp
HEADERS += src/backend.h src/desktop.h
RESOURCES += src/resources.qrc
