TEMPLATE = app
TARGET = lighthugger
QT += core gui widgets network multimedia qml quick quickcontrols2 dbus
CONFIG += c++17

HEADERS += src/somafm.h src/player.h src/mpris.h src/netfactory.h
SOURCES += src/main.cpp src/somafm.cpp src/player.cpp src/mpris.cpp
RESOURCES += resources.qrc

isEmpty(PREFIX): PREFIX = $$(HOME)/.local
target.path = $$PREFIX/bin
desktop.files = data/io.github.mrdfuse.lighthugger.desktop
desktop.path = $$PREFIX/share/applications
icon.files = data/io.github.mrdfuse.lighthugger.svg
icon.path = $$PREFIX/share/icons/hicolor/scalable/apps
INSTALLS += target desktop icon
