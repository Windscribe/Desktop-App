#include "guitest.h"

#include <QApplication>
#include <QKeyEvent>
#include <QTimer>
#include "utils/ws_assert.h"
#include "utils/utils.h"

GuiTest::GuiTest(QWidget *parent) : QObject(parent),
    mainWindow_(parent), curState_(CMD_ID_INITIALIZATION, false, false, false)
{
    cmdsMap_.push_back( qMakePair(CMD_ID_INITIALIZATION, Qt::Key_I) );
    cmdsMap_.push_back( qMakePair(CMD_ID_LOGIN, Qt::Key_L) );
    cmdsMap_.push_back( qMakePair(CMD_ID_LOGGING_IN, Qt::Key_A) );
    cmdsMap_.push_back( qMakePair(CMD_ID_CONNECT, Qt::Key_C) );
    cmdsMap_.push_back( qMakePair(CMD_ID_EMERGENCY, Qt::Key_E) );
    cmdsMap_.push_back( qMakePair(CMD_ID_EXTERNAL_CONFIG, Qt::Key_V) );
    cmdsMap_.push_back( qMakePair(CMD_ID_NOTIFICATIONS, Qt::Key_N) );
    cmdsMap_.push_back( qMakePair(CMD_ID_UPDATE, Qt::Key_B) );
    cmdsMap_.push_back( qMakePair(CMD_ID_UPGRADE, Qt::Key_O) );
    cmdsMap_.push_back( qMakePair(CMD_ID_GENERAL_MESSAGE, Qt::Key_M) );
    cmdsMap_.push_back( qMakePair(CMD_ID_EXIT, Qt::Key_D) );
    cmdsMap_.push_back( qMakePair(CMD_ID_CLOSE_EXIT, Qt::Key_F) );
    cmdsMap_.push_back( qMakePair(CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE, Qt::Key_G) );
    cmdsMap_.push_back( qMakePair(CMD_ID_SHOW_BOTTOM_INFO_SHARING, Qt::Key_P) );
    cmdsMap_.push_back( qMakePair(CMD_ID_EXPAND_PREFERENCES, Qt::Key_Q) );
    cmdsMap_.push_back( qMakePair(CMD_ID_COLLAPSE_PREFERENCES, Qt::Key_W) );
    cmdsMap_.push_back( qMakePair(CMD_ID_EXPAND_LOCATIONS, Qt::Key_Z) );
    cmdsMap_.push_back( qMakePair(CMD_ID_COLLAPSE_LOCATIONS, Qt::Key_X) );
    cmdsMap_.push_back( qMakePair(CMD_ID_SHOW_UPDATE_WIDGET, Qt::Key_U) );
    cmdsMap_.push_back( qMakePair(CMD_ID_HIDE_UPDATE_WIDGET, Qt::Key_Y) );

    // all possible transitions between screens
    insertTransition(TCurState(CMD_ID_INITIALIZATION, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_INITIALIZATION, false, false, false), CMD_ID_LOGGING_IN);

    insertTransition(TCurState(CMD_ID_LOGIN, false, false, false), CMD_ID_EMERGENCY);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, false), CMD_ID_LOGGING_IN);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, false), CMD_ID_EXTERNAL_CONFIG);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, false), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, false), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_LOGIN, false, true, false), CMD_ID_COLLAPSE_PREFERENCES);

    insertTransition(TCurState(CMD_ID_LOGIN, false, false, true), CMD_ID_EMERGENCY);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, true), CMD_ID_LOGGING_IN);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, true), CMD_ID_EXTERNAL_CONFIG);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, true), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_LOGIN, false, false, true), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_LOGIN, false, true, true), CMD_ID_COLLAPSE_PREFERENCES);

    insertTransition(TCurState(CMD_ID_LOGGING_IN, false, false, false), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_LOGGING_IN, false, false, false), CMD_ID_LOGIN);

    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_NOTIFICATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_UPGRADE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_GENERAL_MESSAGE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_EXPAND_LOCATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_NOTIFICATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_UPGRADE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_GENERAL_MESSAGE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_COLLAPSE_LOCATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_NOTIFICATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_UPGRADE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_GENERAL_MESSAGE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_COLLAPSE_LOCATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, true, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_UPDATE);
    //transitions_.push_back( qMakePair(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_GENERAL_MESSAGE) ); ???
    //transitions_.push_back( qMakePair(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_EXIT) ); ???
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_COLLAPSE_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_COLLAPSE_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, false, true, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_NOTIFICATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_UPGRADE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_GENERAL_MESSAGE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_EXIT);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_EXPAND_PREFERENCES);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_EXPAND_LOCATIONS);
    insertTransition(TCurState(CMD_ID_CONNECT, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_EMERGENCY, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_EMERGENCY, false, false, false), CMD_ID_EXIT);

    insertTransition(TCurState(CMD_ID_EXTERNAL_CONFIG, false, false, false), CMD_ID_LOGIN);

    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_UPDATE);
    //transitions_.push_back( qMakePair(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_GENERAL_MESSAGE) ); ???
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_NOTIFICATIONS, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_CONNECT);
    //transitions_.push_back( qMakePair(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_GENERAL_MESSAGE) ); ???
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_UPDATE, false, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPDATE, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_UPDATE, false, true, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPDATE, false, true, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_UPDATE, false, true, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPDATE, false, true, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPDATE, false, true, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_UPDATE, true, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPDATE, true, false, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_UPDATE, true, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPDATE, true, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPDATE, true, false, true), CMD_ID_HIDE_UPDATE_WIDGET);


    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_UPDATE);
    //transitions_.push_back( qMakePair(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_GENERAL_MESSAGE) ); ???
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_LOGIN);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_UPDATE);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_UPGRADE, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, false), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, true), CMD_ID_CONNECT);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_GENERAL_MESSAGE, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);

    insertTransition(TCurState(CMD_ID_EXIT, false, false, false), CMD_ID_CLOSE_EXIT);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, false), CMD_ID_SHOW_UPDATE_WIDGET);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, false), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, false), CMD_ID_UPDATE);

    insertTransition(TCurState(CMD_ID_EXIT, false, false, true), CMD_ID_CLOSE_EXIT);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, true), CMD_ID_HIDE_UPDATE_WIDGET);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, true), CMD_ID_SHOW_BOTTOM_INFO_SHARING);
    insertTransition(TCurState(CMD_ID_EXIT, false, false, true), CMD_ID_UPDATE);
}

void GuiTest::addCommand(Qt::Key qtKey)
{
    TEvent e;
    e.isKey = true;
    e.key = qtKey;
    events_.enqueue(e);
}

void GuiTest::addDelay(int ms)
{
    TEvent e;
    e.isKey = false;
    e.timeoutMs = ms;
    events_.enqueue(e);
}

void GuiTest::start()
{
    handleNext();
}

// Initialization -> Login -> LoggingIn -> connect (without delays)
void GuiTest::test1()
{
    addCommand(Qt::Key_I);
    addDelay(0);
    addCommand(Qt::Key_L);
    addDelay(0);
    addCommand(Qt::Key_A);
    addDelay(0);
    addCommand(Qt::Key_C);
}

void GuiTest::test2()
{    
    addCommand(Qt::Key_I);
    addDelay(0);
    addCommand(Qt::Key_L);
    addDelay(0);
    addCommand(Qt::Key_A);
    addDelay(0);
    addCommand(Qt::Key_C);
    addDelay(0);
    addCommand(Qt::Key_H);
    addCommand(Qt::Key_G);
    addDelay(1000);
    addCommand(Qt::Key_Z);
    addDelay(1000);
    addCommand(Qt::Key_X);
    addDelay(10);
    addCommand(Qt::Key_L);
    addDelay(1000);
    addCommand(Qt::Key_A);
    addDelay(1000);
    addCommand(Qt::Key_C);
    addCommand(Qt::Key_Z);
    addDelay(1000);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_G);

    addDelay(1000);
    addCommand(Qt::Key_X);
}

// Expand locations -> Collapse Locations -> Show BottomInfo -> Expand Locations -> Collapse Locations
void GuiTest::test3()
{
    test1();

    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(2000);
    addCommand(Qt::Key_X);
    addDelay(2000);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_J);
    addCommand(Qt::Key_G);
    addDelay(2000);

    addCommand(Qt::Key_Z);
    addDelay(2000);
    addCommand(Qt::Key_X);
    addDelay(2000);
}

// Expand locations -> Show BottomInfo -> Collapse Locations -> Expand locations -> Hide Bottom -> Collapse Locations
void GuiTest::test4()
{
    test1();

    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(2000);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_J);
    addCommand(Qt::Key_G);
    addDelay(2000);

    addCommand(Qt::Key_X);
    addDelay(2000);

    addCommand(Qt::Key_Z);
    addDelay(2000);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_J);
    addCommand(Qt::Key_G);

    addCommand(Qt::Key_X);
    addDelay(2000);
}

// Expand locations -> Show BottomInfo (white locations animation not finished) -> Collapse Locations
void GuiTest::test5()
{
    test1();

    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(10);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_J);
    addCommand(Qt::Key_G);

    addDelay(2000);
    addCommand(Qt::Key_X);
}

// Expand locations -> Show BottomInfo -> Show preferences -> Hide preferences
void GuiTest::test6()
{
    test1();

    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(200);

    addCommand(Qt::Key_H);
    addCommand(Qt::Key_J);
    addCommand(Qt::Key_G);

    addCommand(Qt::Key_Q);
    addDelay(10);
    addCommand(Qt::Key_W);
}

// Show preferences -> goto login window -> Hide preferences
void GuiTest::test7()
{
    test1();
    addDelay(5000);
    addCommand(Qt::Key_Q);
    addDelay(2000);

    addCommand(Qt::Key_L);

    addCommand(Qt::Key_W);
}

// Expand locations -> goto login window (during animation) -> expand preferences -> collapse preferences
void GuiTest::test8()
{
    test1();
    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(15);

    //addCommand(Qt::Key_Q);
    //addDelay(2000);

    addCommand(Qt::Key_L);
    addDelay(100);
    addCommand(Qt::Key_Q);
    addDelay(2000);
    addCommand(Qt::Key_W);
}

// expand locations -> notifications -> connect window
void GuiTest::test9()
{
    test1();
    addDelay(5000);
    addCommand(Qt::Key_Z);
    addDelay(50);
    addCommand(Qt::Key_N);
    addDelay(20);
    addCommand(Qt::Key_C);
}

// show bottom info -> notifications -> connect window
void GuiTest::test10()
{
    test1();
    addDelay(5000);
    addCommand(Qt::Key_G);
    addCommand(Qt::Key_H);
    addDelay(2000);
    addCommand(Qt::Key_N);
    addDelay(20);
    addCommand(Qt::Key_C);
}

// notifications -> show bottom info -> connect window
void GuiTest::test11()
{
    test1();
    addDelay(2000);
    addCommand(Qt::Key_Q);
    addDelay(2000);
    addCommand(Qt::Key_L);

    //addCommand(Qt::Key_M);
    //addCommand(Qt::Key_G);
    //addCommand(Qt::Key_H);
    //addDelay(2000);
    //addCommand(Qt::Key_G);
    //addCommand(Qt::Key_H);
    //addCommand(Qt::Key_L);
    /*addCommand(Qt::Key_N);
    addDelay(3000);


    addCommand(Qt::Key_G);
    addCommand(Qt::Key_H);
    addDelay(2000);

    addCommand(Qt::Key_C);*/
}

// initialization -> login window
void GuiTest::test12()
{
    addCommand(Qt::Key_I);
    addCommand(Qt::Key_L);
}

// expand preferences -> show update window
void GuiTest::test13()
{
     test1();
     addDelay(5000);

     addCommand(Qt::Key_U);

     addCommand(Qt::Key_G);
     addCommand(Qt::Key_H);
     addDelay(2000);


     addCommand(Qt::Key_Q);
     addDelay(2000);
     addCommand(Qt::Key_B);
     addDelay(2000);
     addCommand(Qt::Key_C);
}

void GuiTest::test14()
{
    test12();
}

void GuiTest::generateRandomTest()
{
    addCommand(Qt::Key_I);
    addDelay(2000);

    curState_.curWindow = CMD_ID_INITIALIZATION;
    curState_.isLocationsExpanded = false;
    curState_.isPreferencesExpanded = false;
    curState_.isUpdateWidgetVisible = false;

    for (int i = 0; i < 100; ++i)
    {

        QVector<CMD_ID> transitions = getTransitions(curState_);
        int n = Utils::generateIntegerRandom(0, transitions.count() - 1);
        CMD_ID cmdId = transitions[n];

        addCommand(cmdIdToQtKey(cmdId));
        prevState_ = curState_;
        if (cmdId == CMD_ID_SHOW_UPDATE_WIDGET)
        {
            curState_.isUpdateWidgetVisible = true;
        }
        else if (cmdId == CMD_ID_HIDE_UPDATE_WIDGET)
        {
            curState_.isPreferencesExpanded = false;
        }
        else if (cmdId == CMD_ID_EXPAND_PREFERENCES)
        {
            curState_.isPreferencesExpanded = true;
        }
        else if (cmdId == CMD_ID_COLLAPSE_PREFERENCES)
        {
            curState_.isPreferencesExpanded = false;
        }
        else if (cmdId == CMD_ID_EXPAND_LOCATIONS)
        {
            curState_.isLocationsExpanded = true;
        }
        else if (cmdId == CMD_ID_COLLAPSE_LOCATIONS)
        {
            curState_.isLocationsExpanded = false;
        }
        else if (cmdId == CMD_ID_EXIT)
        {
            stateBeforeCmdExit_ = curState_.curWindow;
            curState_.curWindow = cmdId;
        }
        else if (cmdId == CMD_ID_CLOSE_EXIT)
        {
            curState_.curWindow = stateBeforeCmdExit_;
        }
        else if (cmdId == CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE || cmdId == CMD_ID_SHOW_BOTTOM_INFO_SHARING)
        {

        }
        else
        {
            if (cmdId == CMD_ID_LOGIN || cmdId == CMD_ID_LOGGING_IN || cmdId == CMD_ID_EMERGENCY ||
                cmdId == CMD_ID_EXTERNAL_CONFIG)
            {
                curState_.isLocationsExpanded = false;
                curState_.isUpdateWidgetVisible = false;
            }
            curState_.curWindow = cmdId;
        }

        addDelay(2000);
    }
}

void GuiTest::handleNext()
{
    if (!events_.isEmpty())
    {
        TEvent e = events_.dequeue();
        if (e.isKey)
        {
            QKeyEvent event(QEvent::KeyPress, e.key, Qt::ControlModifier);
            QApplication::sendEvent(mainWindow_, &event);
            handleNext();
        }
        else
        {
            QTimer::singleShot(e.timeoutMs, this, SLOT(handleNext()));
        }
    }
}

QVector<GuiTest::CMD_ID> GuiTest::getTransitions(const GuiTest::TCurState &s)
{
    auto it = transitions_.find(s);
    if (it == transitions_.end())
    {
        WS_ASSERT(false);
    }
    return *it;
}

void GuiTest::insertTransition(const GuiTest::TCurState &st, GuiTest::CMD_ID cmdId)
{
    auto it = transitions_.find(st);
    if (it != transitions_.end())
    {
        it.value().push_back(cmdId);
    }
    else
    {
        transitions_[st] = QVector<CMD_ID>() << cmdId;
    }
}

GuiTest::CMD_ID GuiTest::qtKeyToCmdId(Qt::Key k)
{
    for (auto it = cmdsMap_.begin(); it != cmdsMap_.end(); ++it)
    {
        if (it->second == k)
        {
            return it->first;
        }
    }
    WS_ASSERT(false);
    return CMD_ID_INITIALIZATION;
}

Qt::Key GuiTest::cmdIdToQtKey(GuiTest::CMD_ID id)
{
    for (auto it = cmdsMap_.begin(); it != cmdsMap_.end(); ++it)
    {
        if (it->first == id)
        {
            return it->second;
        }
    }
    WS_ASSERT(false);
    return Qt::Key_Z;
}
