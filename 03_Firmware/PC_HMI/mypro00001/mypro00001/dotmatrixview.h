#ifndef DOTMATRIXVIEW_H
#define DOTMATRIXVIEW_H

#include <QWidget>
#include <QImage>
#include <QFont>

class DotMatrixView : public QWidget
{
    Q_OBJECT
public:
    explicit DotMatrixView(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setDotSize(int size);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateDotMatrix();

    QString m_text;
    QImage  m_dotImage;
    int     m_dotSize = 12;
};

#endif
