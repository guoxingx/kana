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

#ifndef CHART_H
#define CHART_H

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QTimer>

QT_CHARTS_USE_NAMESPACE

//![1]
class Chart: public QChart
{
    Q_OBJECT
public:
    Chart(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
    virtual ~Chart();

    // 更新轴
    void update_xlist(float *xlist);
    void update_ylist(int *ylist);
    void init(float *xlist, int count);

    // 修改x轴和y轴范围
    void set_x_range(qreal min, qreal max);
    void set_y_range(int min, int max);

    // 为了控制器（主界面）使用同一份数据，但是又懒得改代码，因此公开这些变量
    float _xlist[4096];
    int _ylist[4096];

private:
    // 数据集
    QLineSeries *_series;
    int _count;
    bool _updated;

    // y轴数据要频繁更新，因此用memcpy来更新内存
    // 然后用定时器更新图表
    // 先用最大的4096初始化，在光谱仪启动后调用 init() 来 memset() 实际内存
//    float _xlist[4096];
//    int _ylist[4096];
    int _nbytes_spectrums;

    qreal m_xmin;
    qreal m_xmax;
    qreal m_ymin;
    qreal m_ymax;
};
//![1]

#endif /* CHART_H */
