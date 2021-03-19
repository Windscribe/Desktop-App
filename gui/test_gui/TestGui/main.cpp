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
    RUN_ALL_TESTS();

    return app.exec();
}
