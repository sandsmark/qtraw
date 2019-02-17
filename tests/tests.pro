include(../common-config.pri)

TARGET = qtraw-test

QT += \
    testlib

SOURCES += \
    qtraw-test.cpp

DATA_DIR = $${TOP_SRC_DIR}/tests
DEFINES += \
    DATA_DIR=\\\"$${DATA_DIR}\\\"

check.commands = "QT_PLUGIN_PATH=$${TOP_BUILD_DIR}/src ./qtraw-test"
check.depends = qtraw-test
QMAKE_EXTRA_TARGETS += check
