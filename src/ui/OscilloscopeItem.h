#ifndef OSCILLOSCOPE_ITEM_H
#define OSCILLOSCOPE_ITEM_H

#include <QQuickPaintedItem>
#include <QVariantList>
#include <QColor>

class OscilloscopeItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList waveform READ waveform WRITE setWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(double traceScale READ traceScale WRITE setTraceScale NOTIFY traceScaleChanged)

public:
    explicit OscilloscopeItem(QQuickItem *parent = nullptr);

    QVariantList waveform() const { return m_waveform; }
    void setWaveform(const QVariantList& data);

    QColor color() const { return m_color; }
    void setColor(const QColor& c);

    double traceScale() const { return m_traceScale; }
    void setTraceScale(double s);

signals:
    void waveformChanged();
    void colorChanged();
    void traceScaleChanged();

protected:
    void paint(QPainter *painter) override;

private:
    QVariantList m_waveform;
    QColor m_color{Qt::cyan};
    double m_traceScale{1.0};
};

#endif // OSCILLOSCOPE_ITEM_H