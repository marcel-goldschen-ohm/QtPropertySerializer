TARGET = QtPropertySerializerTest
TEMPLATE = app
QT += core
QT -= gui
CONFIG += c++11

OBJECTS_DIR = Debug/.obj
MOC_DIR = Debug/.moc
RCC_DIR = Debug/.rcc
UI_DIR = Debug/.ui

INCLUDEPATH += ..

HEADERS += QtPropertySerializerTest.h
SOURCES += QtPropertySerializerTest.cpp

HEADERS += ../QtPropertySerializer.h
SOURCES += ../QtPropertySerializer.cpp
