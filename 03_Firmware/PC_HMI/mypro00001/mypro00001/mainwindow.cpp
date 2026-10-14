#include "mainwindow.h"
#include "protocol.h"
#include "dotmatrixview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCanBus>
#include <QDebug>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("XY-Vision-Picker 上位机");
    resize(420, 480);
    buildUi();
}

void MainWindow::buildUi()
{
    QVBoxLayout *root = new QVBoxLayout(this);

    QGroupBox *grpCan = new QGroupBox("CAN 连接", this);
    QHBoxLayout *layCan = new QHBoxLayout(grpCan);
    m_btnConnect = new QPushButton("连接设备", grpCan);
    m_canState = new QLabel("未连接", grpCan);
    layCan->addWidget(new QLabel("通道: usb0 @ 500k", grpCan));
    layCan->addWidget(m_canState);
    layCan->addStretch();
    layCan->addWidget(m_btnConnect);
    root->addWidget(grpCan);

    QGroupBox *grpCmd = new QGroupBox("命令", this);
    QHBoxLayout *layCmd = new QHBoxLayout(grpCmd);
    m_btnHome = new QPushButton("回零", grpCmd);
    m_btnManual = new QPushButton("手动模式", grpCmd);
    m_btnAuto = new QPushButton("自动模式", grpCmd);
    m_btnStop = new QPushButton("停止", grpCmd);
    layCmd->addWidget(m_btnHome);
    layCmd->addWidget(m_btnManual);
    layCmd->addWidget(m_btnAuto);
    layCmd->addWidget(m_btnStop);
    root->addWidget(grpCmd);

    QGroupBox *grpMove = new QGroupBox("手动移动 (mm)", this);
    QHBoxLayout *layMove = new QHBoxLayout(grpMove);
    m_edX = new QLineEdit("100", grpMove);
    m_edY = new QLineEdit("50", grpMove);
    m_btnMove = new QPushButton("移动", grpMove);
    layMove->addWidget(new QLabel("X:", grpMove));
    layMove->addWidget(m_edX);
    layMove->addWidget(new QLabel("Y:", grpMove));
    layMove->addWidget(m_edY);
    layMove->addWidget(m_btnMove);
    root->addWidget(grpMove);

    QGroupBox *grpStatus = new QGroupBox("实时状态", this);
    QGridLayout *laySt = new QGridLayout(grpStatus);
    m_posX = new QLabel("0.000", grpStatus);
    m_posY = new QLabel("0.000", grpStatus);
    m_sysState = new QLabel("IDLE", grpStatus);
    m_errCode = new QLabel("0x0000", grpStatus);
    m_rxCount = new QLabel("0", grpStatus);
    laySt->addWidget(new QLabel("位置 X (mm):", grpStatus), 0, 0);
    laySt->addWidget(m_posX, 0, 1);
    laySt->addWidget(new QLabel("位置 Y (mm):", grpStatus), 1, 0);
    laySt->addWidget(m_posY, 1, 1);
    laySt->addWidget(new QLabel("系统状态:", grpStatus), 2, 0);
    laySt->addWidget(m_sysState, 2, 1);
    laySt->addWidget(new QLabel("错误码:", grpStatus), 3, 0);
    laySt->addWidget(m_errCode, 3, 1);
    laySt->addWidget(new QLabel("收帧计数:", grpStatus), 4, 0);
    laySt->addWidget(m_rxCount, 4, 1);
    root->addWidget(grpStatus);

    QGroupBox *grpDot = new QGroupBox("文字点阵预览 (32x32)", this);
    QVBoxLayout *layDot = new QVBoxLayout(grpDot);
    m_edText = new QLineEdit(grpDot);
    m_edText->setPlaceholderText("输入字母或汉字，如 A、中");
    m_dotView = new DotMatrixView(grpDot);
    m_dotView->setDotSize(12);
    layDot->addWidget(m_edText);
    layDot->addWidget(m_dotView);
    root->addWidget(grpDot);
    root->addStretch();

    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_btnHome, &QPushButton::clicked, this, &MainWindow::onHomeClicked);
    connect(m_btnManual, &QPushButton::clicked, this, &MainWindow::onManualModeClicked);
    connect(m_btnAuto, &QPushButton::clicked, this, &MainWindow::onAutoModeClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_btnMove, &QPushButton::clicked, this, &MainWindow::onMoveClicked);
    connect(m_edText, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_dotView->setText(text);
    });
}

void MainWindow::setCanState(const QString &text, bool ok)
{
    m_canState->setText(text);
    m_canState->setStyleSheet(ok ? "color:#1a7f37;" : "color:#b3261e;");
}

void MainWindow::onConnectClicked()
{
    if (m_can) {
        m_can->disconnectDevice();
        delete m_can;
        m_can = nullptr;
        setCanState("未连接", false);
        m_btnConnect->setText("连接设备");
        return;
    }

    QString err;
    m_can = QCanBus::instance()->createDevice("peakcan", "usb0", &err);
    if (!m_can) {
        setCanState("创建失败: " + err, false);
        return;
    }

    m_can->setConfigurationParameter(QCanBusDevice::BitRateKey, 500000);
    m_can->setConfigurationParameter(QCanBusDevice::LoopbackKey, false);
    m_can->setConfigurationParameter(QCanBusDevice::ReceiveOwnKey, false);

    QList<QCanBusDevice::Filter> filters;
    QCanBusDevice::Filter f1;
    f1.frameId = CAN_ID_POS;
    f1.frameIdMask = 0x7FF;
    f1.type = QCanBusFrame::DataFrame;
    filters.append(f1);
    QCanBusDevice::Filter f2;
    f2.frameId = CAN_ID_ERR;
    f2.frameIdMask = 0x7FF;
    f2.type = QCanBusFrame::DataFrame;
    filters.append(f2);
    m_can->setConfigurationParameter(QCanBusDevice::RawFilterKey, QVariant::fromValue(filters));

    connect(m_can, &QCanBusDevice::framesReceived, this, &MainWindow::onFrameReceived);
    connect(m_can, &QCanBusDevice::errorOccurred, this, [this](QCanBusDevice::CanBusError) {
        setCanState("错误: " + m_can->errorString(), false);
    });

    if (!m_can->connectDevice()) {
        setCanState("连接失败: " + m_can->errorString(), false);
        delete m_can;
        m_can = nullptr;
        return;
    }

    setCanState("已连接", true);
    m_btnConnect->setText("断开");
}

void MainWindow::sendCmd(quint8 cmd)
{
    if (!m_can || m_can->state() != QCanBusDevice::ConnectedState)
        return;

    QCanBusFrame f;
    f.setFrameId(CAN_ID_CMD);
    f.setPayload(QByteArray(1, char(cmd)));
    m_can->writeFrame(f);
}

void MainWindow::sendMove(int x, int y)
{
    if (!m_can || m_can->state() != QCanBusDevice::ConnectedState)
        return;

    QByteArray p;
    p.append(char(CMD_MOVE));
    p.append(char(x & 0xFF));
    p.append(char((x >> 8) & 0xFF));
    p.append(char(y & 0xFF));
    p.append(char((y >> 8) & 0xFF));

    QCanBusFrame f;
    f.setFrameId(CAN_ID_CMD);
    f.setPayload(p);
    m_can->writeFrame(f);
}

void MainWindow::onHomeClicked()     { sendCmd(CMD_HOME); }
void MainWindow::onManualModeClicked(){ sendCmd(CMD_MANUAL); }
void MainWindow::onAutoModeClicked() { sendCmd(CMD_AUTO); }
void MainWindow::onStopClicked()     { sendCmd(CMD_STOP); }

void MainWindow::onMoveClicked()
{
    bool okX, okY;
    int x = m_edX->text().toInt(&okX);
    int y = m_edY->text().toInt(&okY);
    if (!okX || !okY)
        return;
    if (x < -32768 || x > 32767 || y < -32768 || y > 32767)
        return;
    sendMove(x, y);
}

void MainWindow::onFrameReceived()
{
    if (!m_can)
        return;

    while (m_can->framesAvailable()) {
        QCanBusFrame f = m_can->readFrame();
        if (f.isValid() && !f.hasExtendedFrameFormat())
            handleFrame(f);
    }
}

void MainWindow::handleFrame(const QCanBusFrame &frame)
{
    const QByteArray d = frame.payload();
    m_rxTotal++;
    m_rxCount->setText(QString::number(m_rxTotal));

    if (frame.frameId() == CAN_ID_POS && d.size() >= 5) {
        qint16 x = qint16(quint16(quint8(d.at(0))) | (quint16(quint8(d.at(1))) << 8));
        qint16 y = qint16(quint16(quint8(d.at(2))) | (quint16(quint8(d.at(3))) << 8));
        m_posX->setText(QString::number(x / 100.0, 'f', 1));   /* mm×10 */
        m_posY->setText(QString::number(y / 100.0, 'f', 1));

        switch (quint8(d.at(4))) {
        case ST_IDLE:   m_sysState->setText("IDLE");   break;
        case ST_HOME:   m_sysState->setText("回零中"); break;
        case ST_MANUAL: m_sysState->setText("手动");   break;
        case ST_AUTO:   m_sysState->setText("自动");   break;
        case ST_GRIP:   m_sysState->setText("抓取中"); break;
        case ST_DONE:   m_sysState->setText("完成");   break;
        case ST_ERROR:  m_sysState->setText("错误");   break;
        default:        m_sysState->setText("未知");   break;
        }
    } else if (frame.frameId() == CAN_ID_ERR && d.size() >= 2) {
        quint16 err = quint16(quint8(d.at(0))) | (quint16(quint8(d.at(1))) << 8);
        m_errCode->setText(QString("0x%1").arg(err, 4, 16, QLatin1Char('0')));
    }
}
