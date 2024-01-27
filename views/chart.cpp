/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chart.h"

// 坐标轴动态刻度，数据超过这个值就会更新坐标
const int XTICK = 10;
const int YTICK = 100;

// 构造函数后面 直接跟上 变量初始化
Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags):
    QChart(QChart::ChartTypeCartesian, parent, wFlags),
    _series(0),
    _count(0),
    _updated(false),
    _nbytes_spectrums(0),
    m_xmin(1050),
    m_xmax(1230),
    m_ymin(100),
    m_ymax(1000)
{
    // 初始化折线
    _series = new QLineSeries(this);
    // setUseOpenGL(true) 降低卡顿，大约10倍的性能提升
    // 但是在5万个点的时候依旧会明显卡顿，后续考虑将QtChart换成QCustomPlot
    _series->setUseOpenGL(true);
    addSeries(_series);

    // 初始化默认坐标
    createDefaultAxes();
    axisX()->setRange(m_xmin, m_xmax);
    axisY()->setRange(m_ymin, m_ymax);
}

Chart::~Chart()
{
//    delete _xlist;
//    delete _ylist;
}

void Chart::init(float *xlist, int count) {
    qDebug("char init");
    _count = count;
    memset(_xlist, 0, sizeof(_xlist));
    memset(_ylist, 0, sizeof(_xlist));

    _nbytes_spectrums = sizeof(int) * count;

    memcpy(_xlist, xlist, sizeof(float) * count);
    axisX()->setRange(_xlist[0], _xlist[_count - 1]);
}

void Chart::update_xlist(float *xlist) {
    memcpy(_xlist, xlist, _nbytes_spectrums);
    axisX()->setRange(_xlist[0], _xlist[_count - 1]);
}

void Chart::update_ylist(int *ylist) {
    _updated = true;
    memcpy(_ylist, ylist, _nbytes_spectrums);

    if (_count == 0 || !_updated)
        return;
    _updated = false;

    // 用QPointF更新数据，并根据最值更新坐标
    QList<QPointF> data;
    qreal ymin=0, ymax=0;
    for (int i = 0; i < _count; i++) {
        qreal y(_ylist[i]);
        if (ymin < y || ymin == 0)
            ymin = y;
        if (ymax > y)
            ymax = y;
        data.append(QPointF(_xlist[i], y));
    }
    _series->replace(data);

    // 只扩大不缩小
    if (ymin < m_ymin)
        m_ymin = qreal(((int(ymin) / YTICK)) * YTICK);
    if (ymax > m_ymax)
        m_ymax = qreal(((int(ymax) / YTICK) + 1) * YTICK);

    axisY()->setRange(m_ymin, m_ymax);
}
// 修改x轴范围
void Chart::set_x_range(qreal min, qreal max) {
    axisX()->setRange(min, max);
}

// 修改y轴范围
void Chart::set_y_range(int min, int max) {
    m_ymin = min;
    m_ymax = max;
    axisY()->setRange(min, max);
}
