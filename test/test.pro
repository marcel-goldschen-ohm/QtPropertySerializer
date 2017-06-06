TARGET = test
TEMPLATE = app
QT += core
QT -= gui
CONFIG += c++11

OBJECTS_DIR = Debug/.obj
MOC_DIR = Debug/.moc
RCC_DIR = Debug/.rcc
UI_DIR = Debug/.ui

INCLUDEPATH += ..
HEADERS += test.h ../QtObjectPropertySerializer.h
SOURCES += test.cpp ../QtObjectPropertySerializer.cpp
