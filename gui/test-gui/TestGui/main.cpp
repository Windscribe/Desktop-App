#include <iostream>

#include <QApplication>
#include <QMainWindow>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow mainWindow;
    mainWindow.show();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

    // urn app.exec();
}
