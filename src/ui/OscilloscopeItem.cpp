#include "OscilloscopeItem.h"
#include <QPainter>
#include <QPen>
#include <QDebug>

OscilloscopeItem::OscilloscopeItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
}

void OscilloscopeItem::setWaveform(const QVariantList& data) {
    if (m_waveform != data) {
        m_waveform = data;
        emit waveformChanged();
        update(); // Request a repaint
    }
}

void OscilloscopeItem::setColor(const QColor& c) {
    if (m_color != c) {
        m_color = c;
        emit colorChanged();
        update();
    }
}

void OscilloscopeItem::setTraceScale(double s) {
    if (m_traceScale != s) {
        m_traceScale = s;
        emit traceScaleChanged();
        update();
    }
}

void OscilloscopeItem::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen(m_color);
    pen.setWidth(2);
    painter->setPen(pen);

    if (m_waveform.isEmpty()) {
        return;
    }

    QVector<QPointF> points;
    points.reserve(m_waveform.size());
    double xStep = width() / (m_waveform.size() - 1.0);
    double centerY = height() / 2.0;

    for (int i = 0; i < m_waveform.size(); ++i) {
        // Center the waveform and apply vertical scaling
        double val = m_waveform.at(i).toDouble() * m_traceScale;
        double y = centerY - (val * centerY); 
        points.append(QPointF(i * xStep, y));
    }
    painter->drawPolyline(points.data(), points.size());
}