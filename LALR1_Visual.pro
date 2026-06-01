QT += core gui widgets
CONFIG += c++17
TARGET = LALR1_Visual
TEMPLATE = app

INCLUDEPATH += .

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Grammar.cpp \
    DFA.cpp \
    LR0DFA.cpp \
    LR1DFA.cpp \
    LALR1DFA.cpp \
    LALR1Parser.cpp

HEADERS += \
    mainwindow.h \
    common.h \
    Grammar.h \
    DFA.h \
    LR0DFA.h \
    LR1DFA.h \
    LALR1DFA.h \
    LALR1Parser.h
