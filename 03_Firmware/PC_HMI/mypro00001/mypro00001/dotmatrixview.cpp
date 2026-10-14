#include "dotmatrixview.h"
#include <QPainter>
#include <QFontMetrics>

#define FONT_COL 32
#define FONT_ROW 32

DotMatrixView::DotMatrixView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(300);
}

void DotMatrixView::setDotSize(int size)
{
    m_dotSize = qMax(4, size);
    update();
}

void DotMatrixView::setText(const QString &text)
{
    m_text = text;
    updateDotMatrix();
    update();
}

void DotMatrixView::updateDotMatrix()
{
    if (m_text.isEmpty()) {
        m_dotImage = QImage();
        return;
    }

    QChar ch = m_text.at(0);

    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPixelSize(64);
    font.setBold(true);

    QFontMetrics fm(font);
    int w = fm.horizontalAdvance(QString(ch));
    int h = fm.height();
    if (w <= 0 || h <= 0) {
        m_dotImage = QImage();
        return;
    }

    QImage src(w, h, QImage::Format_ARGB32);
    src.fill(Qt::transparent);

    QPainter painter(&src);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    painter.drawText(0, fm.ascent(), QString(ch));
    painter.end();

    m_dotImage = src.scaled(FONT_COL, FONT_ROW,
                            Qt::IgnoreAspectRatio,
                            Qt::FastTransformation);
}

void DotMatrixView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 24));

    if (m_dotImage.isNull()) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "输入文字预览 32x32 点阵");
        return;
    }

    int cols = m_dotImage.width();
    int rows = m_dotImage.height();
    int step = qMin(width() / cols, height() / rows);
    if (step < 1) step = 1;

    int ox = (width() - step * cols) / 2;
    int oy = (height() - step * rows) / 2;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            bool on = m_dotImage.pixelColor(x, y).alpha() > 0;
            QRect r(ox + x * step, oy + y * step, step, step);
            if (on) {
                p.fillRect(r, QColor(255, 80, 80));
            } else {
                p.fillRect(r, QColor(60, 60, 60));
            }
            p.setPen(QColor(90, 90, 90));
            p.drawRect(r);
        }
    }
}
