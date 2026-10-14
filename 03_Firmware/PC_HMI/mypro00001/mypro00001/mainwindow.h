#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QCanBusDevice>
#include <QCanBusFrame>

class QLabel;
class QLineEdit;
class QPushButton;
class DotMatrixView;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnectClicked();
    void onHomeClicked();
    void onManualModeClicked();
    void onAutoModeClicked();
    void onStopClicked();
    void onMoveClicked();
    void onFrameReceived();

private:
    void buildUi();
    void setCanState(const QString &text, bool ok);
    void sendCmd(quint8 cmd);
    void sendMove(int x, int y);
    void handleFrame(const QCanBusFrame &frame);

    QCanBusDevice *m_can = nullptr;
    QLabel *m_canState;
    QLabel *m_posX;
    QLabel *m_posY;
    QLabel *m_sysState;
    QLabel *m_errCode;
    QLabel *m_rxCount;
    QLineEdit *m_edX;
    QLineEdit *m_edY;
    QLineEdit *m_edText;
    DotMatrixView *m_dotView;
    QPushButton *m_btnConnect;
    QPushButton *m_btnHome;
    QPushButton *m_btnManual;
    QPushButton *m_btnAuto;
    QPushButton *m_btnStop;
    QPushButton *m_btnMove;
    quint32 m_rxTotal = 0;
};

#endif
