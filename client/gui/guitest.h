#ifndef GUITEST_H
#define GUITEST_H

#include <QWidget>
#include <QQueue>
#include <QMap>

class GuiTest : public QObject
{
    Q_OBJECT
public:
    explicit GuiTest(QWidget *parent);

    void addCommand(Qt::Key qtKey);
    void addDelay(int ms);

    void start();

    void test1();
    void test2();
    void test3();
    void test4();
    void test5();
    void test6();
    void test7();
    void test8();
    void test9();
    void test10();
    void test11();
    void test12();
    void test13();
    void test14();

    void generateRandomTest();

    enum CMD_ID {    CMD_ID_INITIALIZATION, CMD_ID_LOGIN, CMD_ID_LOGGING_IN,
                     CMD_ID_CONNECT, CMD_ID_EMERGENCY, CMD_ID_EXTERNAL_CONFIG, CMD_ID_NOTIFICATIONS,
                     CMD_ID_UPDATE, CMD_ID_UPGRADE, CMD_ID_GENERAL_MESSAGE, CMD_ID_EXIT, CMD_ID_CLOSE_EXIT,
                     CMD_ID_SHOW_BOTTOM_INFO_UPGRADEMODE, CMD_ID_SHOW_BOTTOM_INFO_SHARING , CMD_ID_EXPAND_PREFERENCES, CMD_ID_COLLAPSE_PREFERENCES,
                     CMD_ID_EXPAND_LOCATIONS, CMD_ID_COLLAPSE_LOCATIONS, CMD_ID_SHOW_UPDATE_WIDGET, CMD_ID_HIDE_UPDATE_WIDGET };

    struct TCurState {
        CMD_ID curWindow;
        bool isLocationsExpanded;
        bool isPreferencesExpanded;
        bool isUpdateWidgetVisible;

        TCurState(CMD_ID w, bool locations, bool preferences, bool update) :
            curWindow(w), isLocationsExpanded(locations), isPreferencesExpanded(preferences), isUpdateWidgetVisible(update)
        {
        }
        TCurState() {}

        bool operator == (const TCurState &c) const
        {
            return c.curWindow == curWindow && c.isLocationsExpanded == isLocationsExpanded &&
                   c.isPreferencesExpanded == isPreferencesExpanded && c.isUpdateWidgetVisible == isUpdateWidgetVisible;
        }

        bool operator < (const TCurState &c) const
        {
            quint32 this_ = (unsigned char)(curWindow) | ((unsigned char)isLocationsExpanded << 8) | ((unsigned char)isPreferencesExpanded << 16) | ((unsigned char)isUpdateWidgetVisible << 24);
            quint32 other = (unsigned char)(c.curWindow) | ((unsigned char)c.isLocationsExpanded << 8) | ((unsigned char)c.isPreferencesExpanded << 16) | ((unsigned char)c.isUpdateWidgetVisible << 24);
            return this_ < other;
        }
    };

private slots:
    void handleNext();

private:
    QWidget *mainWindow_;

    struct TEvent
    {
        bool isKey;
        Qt::Key key;
        int timeoutMs;
    };

    QQueue<TEvent> events_;

    TCurState curState_;
    TCurState prevState_;

    QMap<TCurState, QVector<CMD_ID>> transitions_;

    QVector<CMD_ID> getTransitions(const TCurState &s);

    QVector< QPair< CMD_ID, Qt::Key> > cmdsMap_;

    CMD_ID stateBeforeCmdExit_;

    void insertTransition(const TCurState &st, CMD_ID cmdId);
    CMD_ID qtKeyToCmdId(Qt::Key k);
    Qt::Key cmdIdToQtKey(CMD_ID id);
};

/*inline uint qHash(const GuiTest::TCurState &s, uint seed)
{
    return qHash("key.getHashString()", seed);
}*/

#endif // GUITEST_H
