TARGET = test_QtPropertySerializer
TEMPLATE = app
QT += core
QT -= gui
CONFIG += c++11

OBJECTS_DIR = Debug/.obj
MOC_DIR = Debug/.moc
RCC_DIR = Debug/.rcc
UI_DIR = Debug/.ui

INCLUDEPATH += ..

HEADERS += ../QtPropertySerializer.h
SOURCES += ../QtPropertySerializer.cpp

HEADERS += test_QtPropertySerializer.h
SOURCES += test_QtPropertySerializer.cpp
