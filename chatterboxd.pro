TEMPLATE =app
TARGET =chatterboxd
DEPENDPATH +=src
INCLUDEPATH +=src

CONFIG +=console

QT +=network

OBJECTS_DIR = src/obj
MOC_DIR = src/obj

HEADERS += src/chatterboxserver.h
SOURCES += src/chatterboxserver.cpp src/main.cpp

