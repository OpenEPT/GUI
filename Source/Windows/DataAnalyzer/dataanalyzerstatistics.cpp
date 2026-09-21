#include "dataanalyzerstatistics.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <QPainter>
#include <QToolTip>
#include <QMenu>

DataAnalyzerStatisticsWorker::DataAnalyzerStatisticsWorker(QObject *parent)
    : QObject{parent}
{
}

bool DataAnalyzerStatisticsWorker::markerIsStart(QString name, QString* segmentName)
{
    QRegularExpression re("^(.*?)[\\s_\\-]*(start|begin)\\s*$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(name.trimmed());
    if(!match.hasMatch()) return false;
    if(segmentName != NULL) *segmentName = match.captured(1).trimmed();
    return true;
}

bool DataAnalyzerStatisticsWorker::markerIsStop(QString name, QString* segmentName)
{
    QRegularExpression re("^(.*?)[\\s_\\-]*(stop|end)\\s*$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(name.trimmed());
    if(!match.hasMatch()) return false;
    if(segmentName != NULL) *segmentName = match.captured(1).trimmed();
    return true;
}

void DataAnalyzerStatisticsWorker::onComputeStatistics(QVector<double> voltage, QVector<double> voltageKeys, QVector<double> current, QVector<double> currentKeys, QVector<QPair<QString, int>> markers)
{
    QVector<dataanalyzer_segment_stat_t> stats;
    QVector<dataanalyzer_point_marker_t> points;
    QStringList warnings;
    QVector<QPair<QString, int>> openSegments;
    int sampleCount = qMin(qMin(voltage.size(), current.size()), qMin(voltageKeys.size(), currentKeys.size()));

    for(int i = 0; i < markers.size(); i++)
    {
        QString segmentName;
        QString markerName = markers[i].first.trimmed();
        int index = markers[i].second;
        dataanalyzer_point_marker_t point;

        point.name = markerName;
        point.index = index;
        point.time = (index >= 0 && index < sampleCount) ? voltageKeys[index] : 0;
        point.parentIndex = -1;

        if(markerIsStart(markerName, &segmentName))
        {
            bool alreadyOpen = false;
            for(int k = 0; k < openSegments.size(); k++)
            {
                if(openSegments[k].first.compare(segmentName, Qt::CaseInsensitive) == 0)
                {
                    warnings << "Segment \"" + segmentName + "\" started again before it was stopped, previous start ignored";
                    openSegments[k].second = index;
                    alreadyOpen = true;
                    break;
                }
            }
            if(!alreadyOpen)
            {
                openSegments.append(QPair<QString, int>(segmentName, index));
            }
            continue;
        }

        if(markerIsStop(markerName, &segmentName))
        {
            int openIndex = -1;
            for(int k = 0; k < openSegments.size(); k++)
            {
                if(openSegments[k].first.compare(segmentName, Qt::CaseInsensitive) == 0)
                {
                    openIndex = k;
                    break;
                }
            }
            if(openIndex < 0)
            {
                warnings << "Stop marker \"" + markerName + "\" has no matching start";
                points.append(point);
                continue;
            }

            dataanalyzer_segment_stat_t stat;
            stat.parentIndex = -1;
            stat.name = openSegments[openIndex].first;
            stat.startIndex = openSegments[openIndex].second;
            stat.endIndex = index;
            openSegments.removeAt(openIndex);

            if(stat.startIndex < 0 || stat.endIndex >= sampleCount || stat.endIndex <= stat.startIndex)
            {
                warnings << "Segment \"" + stat.name + "\" is outside of loaded data range";
                continue;
            }

            if(computeSegment(voltage, voltageKeys, current, &stat))
            {
                stats.append(stat);
            }
            continue;
        }

        if(index >= 0 && index < sampleCount)
        {
            points.append(point);
        }
    }

    for(int k = 0; k < openSegments.size(); k++)
    {
        dataanalyzer_point_marker_t point;
        warnings << "Start marker \"" + openSegments[k].first + "\" has no matching stop";
        point.name = openSegments[k].first + " Start";
        point.index = openSegments[k].second;
        point.time = (point.index >= 0 && point.index < sampleCount) ? voltageKeys[point.index] : 0;
        point.parentIndex = -1;
        if(point.index >= 0 && point.index < sampleCount) points.append(point);
    }

    for(int i = 1; i < stats.size(); i++)
    {
        dataanalyzer_segment_stat_t tmp = stats[i];
        int k = i - 1;
        while(k >= 0 && (stats[k].startIndex > tmp.startIndex ||
              (stats[k].startIndex == tmp.startIndex && stats[k].endIndex < tmp.endIndex)))
        {
            stats[k + 1] = stats[k];
            k--;
        }
        stats[k + 1] = tmp;
    }

    for(int i = 0; i < stats.size(); i++)
    {
        stats[i].parentIndex = -1;
        for(int k = 0; k < stats.size(); k++)
        {
            if(k == i) continue;
            if(stats[k].startIndex <= stats[i].startIndex && stats[k].endIndex >= stats[i].endIndex &&
               (stats[k].endIndex - stats[k].startIndex) > (stats[i].endIndex - stats[i].startIndex))
            {
                if(stats[i].parentIndex < 0 ||
                   (stats[k].endIndex - stats[k].startIndex) < (stats[stats[i].parentIndex].endIndex - stats[stats[i].parentIndex].startIndex))
                {
                    stats[i].parentIndex = k;
                }
            }
        }
    }

    for(int i = 0; i < points.size(); i++)
    {
        points[i].parentIndex = -1;
        for(int k = 0; k < stats.size(); k++)
        {
            if(stats[k].startIndex <= points[i].index && stats[k].endIndex >= points[i].index)
            {
                if(points[i].parentIndex < 0 ||
                   (stats[k].endIndex - stats[k].startIndex) < (stats[points[i].parentIndex].endIndex - stats[points[i].parentIndex].startIndex))
                {
                    points[i].parentIndex = k;
                }
            }
        }
    }

    dataanalyzer_segment_stat_t total;
    total.parentIndex = -1;
    total.name = "Whole profile";
    total.startIndex = 0;
    total.endIndex = sampleCount - 1;
    if(sampleCount < 2 || !computeSegment(voltage, voltageKeys, current, &total))
    {
        total.consumption = 0;
        total.energy = 0;
        total.duration = 0;
        total.maxCurrent = 0;
        total.minCurrent = 0;
        total.avgCurrent = 0;
        total.maxVoltage = 0;
        total.minVoltage = 0;
        total.avgVoltage = 0;
        total.startTime = 0;
        total.endTime = 0;
    }

    emit sigStatisticsFinished(stats, points, total, warnings);
}

bool DataAnalyzerStatisticsWorker::computeSegment(QVector<double>& voltage, QVector<double>& keys, QVector<double>& current, dataanalyzer_segment_stat_t* stat)
{
    double chargeMAms = 0;
    double energyUJ = 0;
    double currentSum = 0;
    double voltageSum = 0;
    int samples = 0;

    if(stat == NULL) return false;

    stat->maxCurrent = current[stat->startIndex];
    stat->minCurrent = current[stat->startIndex];
    stat->maxVoltage = voltage[stat->startIndex];
    stat->minVoltage = voltage[stat->startIndex];

    for(int i = stat->startIndex; i < stat->endIndex; i++)
    {
        double dt = keys[i + 1] - keys[i];
        if(dt < 0) dt = 0;

        chargeMAms += current[i] * dt;
        energyUJ += voltage[i] * current[i] * dt;
        currentSum += current[i];
        voltageSum += voltage[i];
        samples += 1;

        if(current[i] > stat->maxCurrent) stat->maxCurrent = current[i];
        if(current[i] < stat->minCurrent) stat->minCurrent = current[i];
        if(voltage[i] > stat->maxVoltage) stat->maxVoltage = voltage[i];
        if(voltage[i] < stat->minVoltage) stat->minVoltage = voltage[i];
    }

    if(samples == 0) return false;

    stat->duration = keys[stat->endIndex] - keys[stat->startIndex];
    stat->startTime = keys[stat->startIndex];
    stat->endTime = keys[stat->endIndex];
    stat->consumption = chargeMAms / 3600000.0;
    stat->energy = energyUJ / 1000.0;
    stat->avgCurrent = currentSum / samples;
    stat->avgVoltage = voltageSum / samples;

    return true;
}

#define SHARE_BAR_ROW_HEIGHT    22

DataAnalyzerShareBar::DataAnalyzerShareBar(QWidget *parent)
    : QWidget{parent}
{
    levels = 1;
    setMinimumHeight(SHARE_BAR_ROW_HEIGHT + 6);
    setMaximumHeight(SHARE_BAR_ROW_HEIGHT + 6);
    setMouseTracking(true);
}

void DataAnalyzerShareBar::setShares(QVector<dataanalyzer_share_t> aShares)
{
    shares = aShares;
    layoutShares();
    setMinimumHeight(levels * SHARE_BAR_ROW_HEIGHT + 6);
    setMaximumHeight(levels * SHARE_BAR_ROW_HEIGHT + 6);
    update();
}

void DataAnalyzerShareBar::layoutChildren(int parent, double x0, double x1, double parentShare, int level)
{
    QVector<int> children;
    double x = x0;

    if(level + 1 > levels) levels = level + 1;

    for(int i = 0; i < shares.size(); i++)
    {
        if(shares[i].parentIndex == parent) children.append(i);
    }
    for(int a = 0; a < children.size(); a++)
    {
        for(int b = a + 1; b < children.size(); b++)
        {
            if(shares[children[b]].startIndex < shares[children[a]].startIndex)
            {
                int tmp = children[a]; children[a] = children[b]; children[b] = tmp;
            }
        }
    }
    for(int c = 0; c < children.size(); c++)
    {
        int i = children[c];
        double w = (parentShare > 0) ? (x1 - x0) * shares[i].share / parentShare : 0;
        if(x + w > x1) w = x1 - x;
        if(w < 0) w = 0;
        shares[i].level = level;
        shares[i].x0 = x;
        shares[i].x1 = x + w;
        layoutChildren(i, x, x + w, shares[i].share, level + 1);
        x += w;
    }
}

void DataAnalyzerShareBar::layoutShares()
{
    levels = 1;
    for(int i = 0; i < shares.size(); i++)
    {
        shares[i].level = 0;
        shares[i].x0 = 0;
        shares[i].x1 = 0;
    }
    layoutChildren(-1, 0.0, 1.0, 100.0, 0);
}

void DataAnalyzerShareBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect area = rect().adjusted(0, 2, -1, -3);
    double width = area.width();

    Q_UNUSED(event);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(230, 230, 230));
    painter.drawRect(area);

    for(int i = 0; i < shares.size(); i++)
    {
        QRectF seg(area.left() + shares[i].x0 * width,
                   area.top() + shares[i].level * SHARE_BAR_ROW_HEIGHT,
                   (shares[i].x1 - shares[i].x0) * width,
                   SHARE_BAR_ROW_HEIGHT);
        if(seg.width() <= 0) continue;
        painter.setPen(QColor(255, 255, 255));
        painter.setBrush(shares[i].color);
        painter.drawRect(seg);
        painter.setPen(Qt::black);
        QString label = shares[i].name + " " + QString::number(shares[i].share, 'f', 1) + "%";
        if(painter.fontMetrics().horizontalAdvance(label) + 6 < seg.width())
        {
            painter.drawText(seg, Qt::AlignCenter, label);
        }
        else if(painter.fontMetrics().horizontalAdvance(shares[i].name) + 4 < seg.width())
        {
            painter.drawText(seg, Qt::AlignCenter, shares[i].name);
        }
    }
    painter.setPen(Qt::gray);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area);
}

int DataAnalyzerShareBar::shareAt(QPoint pos)
{
    QRect area = rect().adjusted(0, 2, -1, -3);
    double width = area.width();
    int level = (pos.y() - area.top()) / SHARE_BAR_ROW_HEIGHT;
    double rx = (pos.x() - area.left()) / width;

    for(int i = 0; i < shares.size(); i++)
    {
        if(shares[i].level == level && rx >= shares[i].x0 && rx < shares[i].x1) return i;
    }
    return -1;
}

void DataAnalyzerShareBar::mouseMoveEvent(QMouseEvent *event)
{
    int i = shareAt(event->pos());
    QString text;

    if(i >= 0)
    {
        text = shares[i].name + ": " + QString::number(shares[i].share, 'f', 2) + " % of cycle";
        if(shares[i].parentIndex >= 0 && shares[shares[i].parentIndex].share > 0)
        {
            text += ", " + QString::number(shares[i].share / shares[shares[i].parentIndex].share * 100.0, 'f', 2) + " % of " + shares[shares[i].parentIndex].name;
        }
    }
    else
    {
        double marked = 0;
        for(int k = 0; k < shares.size(); k++) if(shares[k].parentIndex < 0) marked += shares[k].share;
        text = "Unmarked part of cycle: " + QString::number(qMax(0.0, 100.0 - marked), 'f', 2) + " %";
    }
    QToolTip::showText(event->globalPos(), text, this);
}

DataAnalyzerStatisticsItem::DataAnalyzerStatisticsItem(int segmentIndex)
    : QTreeWidgetItem()
{
    setData(0, Qt::UserRole + 1, segmentIndex);
    setData(0, Qt::UserRole + 2, -1);
}

bool DataAnalyzerStatisticsItem::operator<(const QTreeWidgetItem &other) const
{
    int column = treeWidget() ? treeWidget()->sortColumn() : 0;
    QVariant mine = data(column, Qt::UserRole);
    QVariant theirs = other.data(column, Qt::UserRole);
    if(column != 0 && mine.isValid() && theirs.isValid()) return mine.toDouble() < theirs.toDouble();
    return text(column).toLower() < other.text(column).toLower();
}

#define STAT_COL_NAME           0
#define STAT_COL_FIRST          1
#define STAT_COL_LAST           2
#define STAT_COL_CONSUMPTION    3
#define STAT_COL_ENERGY         4
#define STAT_COL_DURATION       5
#define STAT_COL_SHARE          6
#define STAT_COL_PARENT_SHARE   7
#define STAT_COL_BATTERY        8
#define STAT_COL_MAX_CURRENT    9
#define STAT_COL_MIN_CURRENT    10
#define STAT_COL_AVG_CURRENT    11
#define STAT_COL_MAX_VOLTAGE    12
#define STAT_COL_MIN_VOLTAGE    13
#define STAT_COL_AVG_VOLTAGE    14
#define STAT_COL_COUNT          15

#define STAT_ROLE_SEGMENT       (Qt::UserRole + 1)
#define STAT_ROLE_POINT         (Qt::UserRole + 2)

DataAnalyzerStatisticsWnd::DataAnalyzerStatisticsWnd(QWidget *parent)
    : QWidget{parent}
{
    QStringList header;

    tableUpdating = false;

    setWindowFlags(Qt::Window);
    setWindowTitle("Consumption statistics");
    resize(1300, 520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    header << "Segment" << "First action [s]" << "Last action [s]" << "Consumption [mAh]" << "Energy [mJ]" << "Duration [s]" << "Share of cycle [%]" << "Share of parent [%]" << "Battery [mAh]"
           << "Max Current [mA]" << "Min Current [mA]" << "Avg Current [mA]" << "Max Voltage [V]" << "Min Voltage [V]" << "Avg Voltage [V]";
    table = new QTreeWidget(this);
    table->setColumnCount(header.size());
    table->setHeaderLabels(header);
    table->headerItem()->setToolTip(STAT_COL_NAME, "Segments nested in time are shown as sub-items, single markers are listed inside the segment where they occurred; double click to zoom plots");
    table->headerItem()->setToolTip(STAT_COL_FIRST, "Time of segment start (or marker time) from the beginning of the recording");
    table->headerItem()->setToolTip(STAT_COL_LAST, "Time of segment stop from the beginning of the recording");
    table->headerItem()->setToolTip(STAT_COL_CONSUMPTION, "Editable: charge consumed by segment in one cycle (includes sub-segments)");
    table->headerItem()->setToolTip(STAT_COL_DURATION, "Editable: segment duration; consumption is scaled with average current");
    table->headerItem()->setToolTip(STAT_COL_SHARE, "Segment consumption / whole cycle consumption");
    table->headerItem()->setToolTip(STAT_COL_PARENT_SHARE, "Segment consumption / parent segment consumption");
    table->headerItem()->setToolTip(STAT_COL_BATTERY, "Part of battery capacity spent on this segment over the whole operating time");
    table->header()->setSectionResizeMode(QHeaderView::Interactive);
    table->header()->setStretchLastSection(false);
    table->header()->setSectionsMovable(true);
    table->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    table->header()->setToolTip("Drag column edges to resize, right click to show or hide columns");
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table->setAllColumnsShowFocus(true);
    mainLayout->addWidget(table, 3);

    unassignedLabel = new QLabel("Unassigned markers (outside of any segment)", this);
    unassignedLabel->setStyleSheet("font-weight: bold;");
    unassignedTable = new QTreeWidget(this);
    unassignedTable->setColumnCount(3);
    unassignedTable->setHeaderLabels(QStringList() << "Marker" << "Time [s]" << "Sample index");
    unassignedTable->setRootIsDecorated(false);
    unassignedTable->setAlternatingRowColors(true);
    unassignedTable->setSortingEnabled(true);
    unassignedTable->header()->setSectionResizeMode(QHeaderView::Interactive);
    unassignedTable->header()->setStretchLastSection(false);
    unassignedTable->setMaximumHeight(160);
    mainLayout->addWidget(unassignedLabel);
    mainLayout->addWidget(unassignedTable, 1);
    unassignedLabel->hide();
    unassignedTable->hide();

    shareBar = new DataAnalyzerShareBar(this);
    mainLayout->addWidget(shareBar);

    wholeInfoLabel = new QLabel(this);
    markedInfoLabel = new QLabel(this);
    unmarkedInfoLabel = new QLabel(this);
    mainLayout->addWidget(wholeInfoLabel);
    mainLayout->addWidget(markedInfoLabel);
    mainLayout->addWidget(unmarkedInfoLabel);

    QHBoxLayout *infoRow = new QHBoxLayout();
    QLabel *cycleReferenceLabel = new QLabel("Cycle reference:", this);
    cycleReferenceCombo = new QComboBox(this);
    cycleReferenceCombo->setMinimumWidth(160);
    infoRow->addWidget(cycleReferenceLabel);
    infoRow->addWidget(cycleReferenceCombo);
    infoRow->addSpacing(20);
    cycleDurationLabel = new QLabel(this);
    cycleConsumptionLabel = new QLabel(this);
    averageCurrentLabel = new QLabel(this);
    infoRow->addWidget(cycleDurationLabel);
    infoRow->addSpacing(20);
    infoRow->addWidget(cycleConsumptionLabel);
    infoRow->addSpacing(20);
    infoRow->addWidget(averageCurrentLabel);
    infoRow->addStretch();
    mainLayout->addLayout(infoRow);

    QHBoxLayout *batteryRow = new QHBoxLayout();
    QLabel *capacityLabel = new QLabel("Battery capacity [mAh]:", this);
    capacityLabel->setFixedWidth(300);
    batteryRow->addWidget(capacityLabel);
    batteryCapacityEdit = new QLineEdit(this);
    batteryCapacityEdit->setFixedWidth(120);
    batteryCapacityEdit->setPlaceholderText("e.g. 2000");
    batteryRow->addWidget(batteryCapacityEdit);
    batteryRow->addSpacing(20);
    operatingTimeLabel = new QLabel(this);
    operatingTimeLabel->setStyleSheet("font-weight: bold;");
    cyclesLabel = new QLabel(this);
    batteryRow->addWidget(operatingTimeLabel);
    batteryRow->addSpacing(20);
    batteryRow->addWidget(cyclesLabel);
    batteryRow->addStretch();
    mainLayout->addLayout(batteryRow);

    QHBoxLayout *targetRow = new QHBoxLayout();
    QLabel *targetLabel = new QLabel("Target operating time [hh:mm:ss]:", this);
    targetLabel->setFixedWidth(300);
    targetRow->addWidget(targetLabel);
    targetTimeEdit = new QLineEdit(this);
    targetTimeEdit->setFixedWidth(120);
    targetTimeEdit->setPlaceholderText("e.g. 72:00:00");
    targetRow->addWidget(targetTimeEdit);
    targetRow->addSpacing(20);
    requiredCapacityLabel = new QLabel(this);
    requiredCapacityLabel->setStyleSheet("font-weight: bold;");
    targetRow->addWidget(requiredCapacityLabel);
    targetRow->addStretch();
    mainLayout->addLayout(targetRow);

    QHBoxLayout *buttonRow = new QHBoxLayout();
    expandButton = new QPushButton("Expand all", this);
    collapseButton = new QPushButton("Collapse all", this);
    buttonRow->addWidget(expandButton);
    buttonRow->addWidget(collapseButton);
    buttonRow->addStretch();
    resetButton = new QPushButton("Reset edits", this);
    exportButton = new QPushButton("Export CSV", this);
    buttonRow->addWidget(resetButton);
    buttonRow->addWidget(exportButton);
    mainLayout->addLayout(buttonRow);

    connect(expandButton, SIGNAL(clicked()), table, SLOT(expandAll()));
    connect(collapseButton, SIGNAL(clicked()), table, SLOT(collapseAll()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(onExportCsv()));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(onResetEdits()));
    connect(batteryCapacityEdit, SIGNAL(textChanged(QString)), this, SLOT(onBatteryCapacityChanged()));
    connect(targetTimeEdit, SIGNAL(textChanged(QString)), this, SLOT(onTargetTimeChanged()));
    connect(cycleReferenceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onCycleReferenceChanged()));
    connect(table, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*,int)));
    connect(table, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));
    connect(table->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onHeaderContextMenu(QPoint)));
    connect(unassignedTable, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onUnassignedDoubleClicked(QTreeWidgetItem*,int)));
}

void DataAnalyzerStatisticsWnd::onHeaderContextMenu(QPoint pos)
{
    QMenu menu(this);
    QAction *showAll = menu.addAction("Show all columns");
    QAction *fit = menu.addAction("Fit columns to contents");
    menu.addSeparator();
    QAction *expandAllRows = menu.addAction("Expand all rows");
    QAction *collapseAllRows = menu.addAction("Collapse all rows");
    menu.addSeparator();
    QVector<QAction*> actions(STAT_COL_COUNT, NULL);

    for(int i = 1; i < STAT_COL_COUNT; i++)
    {
        actions[i] = menu.addAction(table->headerItem()->text(i));
        actions[i]->setCheckable(true);
        actions[i]->setChecked(!table->isColumnHidden(i));
    }

    QAction *chosen = menu.exec(table->header()->mapToGlobal(pos));
    if(chosen == NULL) return;
    if(chosen == showAll)
    {
        for(int i = 0; i < STAT_COL_COUNT; i++) table->setColumnHidden(i, false);
        return;
    }
    if(chosen == fit)
    {
        for(int i = 0; i < STAT_COL_COUNT; i++) table->resizeColumnToContents(i);
        return;
    }
    if(chosen == expandAllRows)
    {
        table->expandAll();
        return;
    }
    if(chosen == collapseAllRows)
    {
        table->collapseAll();
        return;
    }
    for(int i = 1; i < STAT_COL_COUNT; i++)
    {
        if(actions[i] == chosen)
        {
            table->setColumnHidden(i, !chosen->isChecked());
            break;
        }
    }
}

QString DataAnalyzerStatisticsWnd::formatConsumption(double mAh)
{
    double absValue = qAbs(mAh);
    if(absValue >= 1000.0) return QString::number(mAh / 1000.0, 'f', 3) + " Ah";
    if(absValue >= 1.0) return QString::number(mAh, 'f', 3) + " mAh";
    if(absValue >= 0.001) return QString::number(mAh * 1000.0, 'f', 3) + " uAh";
    return QString::number(mAh * 1000000.0, 'f', 3) + " nAh";
}

QString DataAnalyzerStatisticsWnd::formatEnergy(double mJ)
{
    double absValue = qAbs(mJ);
    if(absValue >= 1000.0) return QString::number(mJ / 1000.0, 'f', 3) + " J";
    if(absValue >= 1.0) return QString::number(mJ, 'f', 3) + " mJ";
    if(absValue >= 0.001) return QString::number(mJ * 1000.0, 'f', 3) + " uJ";
    return QString::number(mJ * 1000000.0, 'f', 3) + " nJ";
}

QString DataAnalyzerStatisticsWnd::formatClock(double ms)
{
    qint64 totalSeconds = (qint64)(ms / 1000.0 + 0.5);
    qint64 hours = totalSeconds / 3600;
    qint64 minutes = (totalSeconds % 3600) / 60;
    qint64 seconds = totalSeconds % 60;
    QString clock = QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    if(hours >= 24)
    {
        clock += " (" + QString::number(ms / 86400000.0, 'f', 2) + " days)";
    }
    return clock;
}

bool DataAnalyzerStatisticsWnd::parseClock(QString text, double* ms)
{
    QStringList parts = text.trimmed().split(':');
    double hours = 0;
    double minutes = 0;
    double seconds = 0;
    bool ok = false;

    if(ms == NULL) return false;
    if(parts.size() < 1 || parts.size() > 3) return false;
    if(parts.size() == 3)
    {
        hours = parts[0].toDouble(&ok); if(!ok) return false;
        minutes = parts[1].toDouble(&ok); if(!ok) return false;
        seconds = parts[2].toDouble(&ok); if(!ok) return false;
    }
    else if(parts.size() == 2)
    {
        hours = parts[0].toDouble(&ok); if(!ok) return false;
        minutes = parts[1].toDouble(&ok); if(!ok) return false;
    }
    else
    {
        hours = parts[0].toDouble(&ok); if(!ok) return false;
    }
    *ms = (hours * 3600.0 + minutes * 60.0 + seconds) * 1000.0;
    return *ms > 0;
}

QColor DataAnalyzerStatisticsWnd::segmentColor(int index)
{
    static const QColor palette[] = {
        QColor(102, 194, 165), QColor(252, 141,  98), QColor(141, 160, 203), QColor(231, 138, 195),
        QColor(166, 216,  84), QColor(255, 217,  47), QColor(229, 196, 148), QColor(179, 179, 179),
        QColor( 27, 158, 119), QColor(217,  95,   2), QColor(117, 112, 179), QColor(231,  41, 138),
        QColor(102, 166,  30), QColor(230, 171,   2), QColor(166, 118,  29), QColor(102, 102, 102)
    };
    return palette[index % 16];
}

QString DataAnalyzerStatisticsWnd::formatDuration(double ms)
{
    if(ms >= 86400000.0) return QString::number(ms / 86400000.0, 'f', 2) + " days";
    if(ms >= 3600000.0) return QString::number(ms / 3600000.0, 'f', 2) + " h";
    if(ms >= 60000.0) return QString::number(ms / 60000.0, 'f', 2) + " min";
    if(ms >= 1000.0) return QString::number(ms / 1000.0, 'f', 3) + " s";
    return QString::number(ms, 'f', 3) + " ms";
}

void DataAnalyzerStatisticsWnd::setStatistics(QString aProfileName, QVector<dataanalyzer_segment_stat_t> stats, QVector<dataanalyzer_point_marker_t> aPoints, dataanalyzer_segment_stat_t aTotal, QStringList warnings)
{
    profileName = aProfileName;
    statistics = stats;
    edited = stats;
    points = aPoints;
    total = aTotal;
    setWindowTitle("Consumption statistics - " + profileName);

    cycleReferenceCombo->blockSignals(true);
    cycleReferenceCombo->clear();
    cycleReferenceCombo->addItem("Whole recording", -1);
    cycleReferenceCombo->addItem("Marked segments only", -2);
    for(int i = 0; i < stats.size(); i++)
    {
        if(stats[i].parentIndex < 0) cycleReferenceCombo->addItem(stats[i].name, i);
    }
    cycleReferenceCombo->setCurrentIndex(0);
    cycleReferenceCombo->blockSignals(false);

    fillTable();
    fillUnassigned();
    updateBatteryEstimate();
    for(int i = 0; i < STAT_COL_COUNT; i++) table->resizeColumnToContents(i);
    for(int i = 0; i < 3; i++) unassignedTable->resizeColumnToContents(i);

    if(!warnings.isEmpty())
    {
        QMessageBox::information(this, "Consumption statistics", warnings.join("\n"));
    }
}

void DataAnalyzerStatisticsWnd::setCell(QTreeWidgetItem *item, int column, double value, QString text, bool editable)
{
    item->setText(column, text);
    item->setData(column, Qt::UserRole, value);
    item->setTextAlignment(column, column == STAT_COL_NAME ? (Qt::AlignLeft | Qt::AlignVCenter) : (Qt::AlignRight | Qt::AlignVCenter));
    if(editable)
    {
        item->setBackground(column, QColor(255, 250, 205));
    }
}

void DataAnalyzerStatisticsWnd::refreshItemValues(int i)
{
    QTreeWidgetItem *item = items[i];
    setCell(item, STAT_COL_FIRST, edited[i].startTime / 1000.0, QString::number(edited[i].startTime / 1000.0, 'f', 3), false);
    setCell(item, STAT_COL_LAST, edited[i].endTime / 1000.0, QString::number(edited[i].endTime / 1000.0, 'f', 3), false);
    setCell(item, STAT_COL_CONSUMPTION, edited[i].consumption, QString::number(edited[i].consumption, 'f', 6), true);
    setCell(item, STAT_COL_ENERGY, edited[i].energy, QString::number(edited[i].energy, 'f', 3), false);
    setCell(item, STAT_COL_DURATION, edited[i].duration / 1000.0, QString::number(edited[i].duration / 1000.0, 'f', 3), true);
    setCell(item, STAT_COL_MAX_CURRENT, edited[i].maxCurrent, QString::number(edited[i].maxCurrent, 'f', 3), false);
    setCell(item, STAT_COL_MIN_CURRENT, edited[i].minCurrent, QString::number(edited[i].minCurrent, 'f', 3), false);
    setCell(item, STAT_COL_AVG_CURRENT, edited[i].avgCurrent, QString::number(edited[i].avgCurrent, 'f', 3), false);
    setCell(item, STAT_COL_MAX_VOLTAGE, edited[i].maxVoltage, QString::number(edited[i].maxVoltage, 'f', 4), false);
    setCell(item, STAT_COL_MIN_VOLTAGE, edited[i].minVoltage, QString::number(edited[i].minVoltage, 'f', 4), false);
    setCell(item, STAT_COL_AVG_VOLTAGE, edited[i].avgVoltage, QString::number(edited[i].avgVoltage, 'f', 4), false);
}

QTreeWidgetItem* DataAnalyzerStatisticsWnd::createPointItem(int pointIndex)
{
    DataAnalyzerStatisticsItem *item = new DataAnalyzerStatisticsItem(-1);
    QPixmap swatch(14, 14);
    QPainter painter(&swatch);
    QFont font = item->font(STAT_COL_NAME);

    swatch.fill(Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(90, 90, 90));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(4, 4, 6, 6);
    painter.end();

    font.setItalic(true);
    item->setData(0, STAT_ROLE_POINT, pointIndex);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setIcon(STAT_COL_NAME, QIcon(swatch));
    item->setFont(STAT_COL_NAME, font);
    item->setForeground(STAT_COL_NAME, QColor(90, 90, 90));
    item->setToolTip(STAT_COL_NAME, "Single marker (no Start/Stop pair), sample index " + QString::number(points[pointIndex].index));
    setCell(item, STAT_COL_NAME, 0, points[pointIndex].name, false);
    setCell(item, STAT_COL_FIRST, points[pointIndex].time / 1000.0, QString::number(points[pointIndex].time / 1000.0, 'f', 3), false);
    setCell(item, STAT_COL_LAST, points[pointIndex].time / 1000.0, "", false);
    return item;
}

void DataAnalyzerStatisticsWnd::fillUnassigned()
{
    int count = 0;

    unassignedTable->setSortingEnabled(false);
    unassignedTable->clear();
    for(int i = 0; i < points.size(); i++)
    {
        if(points[i].parentIndex >= 0) continue;
        DataAnalyzerStatisticsItem *item = new DataAnalyzerStatisticsItem(-1);
        item->setData(0, STAT_ROLE_POINT, i);
        item->setText(0, points[i].name);
        item->setText(1, QString::number(points[i].time / 1000.0, 'f', 3));
        item->setData(1, Qt::UserRole, points[i].time);
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        item->setText(2, QString::number(points[i].index));
        item->setData(2, Qt::UserRole, (double)points[i].index);
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        unassignedTable->addTopLevelItem(item);
        count++;
    }
    unassignedTable->setSortingEnabled(true);
    unassignedTable->sortByColumn(1, Qt::AscendingOrder);
    unassignedLabel->setText("Unassigned markers (outside of any segment): " + QString::number(count));
    unassignedLabel->setVisible(count > 0);
    unassignedTable->setVisible(count > 0);
}

void DataAnalyzerStatisticsWnd::fillTable()
{
    tableUpdating = true;
    table->setSortingEnabled(false);
    table->clear();
    items.clear();
    items.resize(edited.size());

    for(int i = 0; i < edited.size(); i++)
    {
        items[i] = new DataAnalyzerStatisticsItem(i);
        items[i]->setFlags(items[i]->flags() | Qt::ItemIsEditable);
        QPixmap swatch(14, 14);
        swatch.fill(segmentColor(i));
        items[i]->setIcon(STAT_COL_NAME, QIcon(swatch));
        setCell(items[i], STAT_COL_NAME, i, edited[i].name, false);
        setCell(items[i], STAT_COL_SHARE, 0, "", false);
        setCell(items[i], STAT_COL_PARENT_SHARE, 0, "", false);
        setCell(items[i], STAT_COL_BATTERY, 0, "", false);
        refreshItemValues(i);
    }
    for(int i = 0; i < edited.size(); i++)
    {
        if(edited[i].parentIndex >= 0 && edited[i].parentIndex < items.size())
        {
            items[edited[i].parentIndex]->addChild(items[i]);
        }
        else
        {
            table->addTopLevelItem(items[i]);
        }
    }
    for(int i = 0; i < points.size(); i++)
    {
        if(points[i].parentIndex >= 0 && points[i].parentIndex < items.size())
        {
            items[points[i].parentIndex]->addChild(createPointItem(i));
        }
    }
    table->expandAll();
    table->setSortingEnabled(true);
    table->sortByColumn(STAT_COL_FIRST, Qt::AscendingOrder);
    tableUpdating = false;
}

void DataAnalyzerStatisticsWnd::selectPoint(int i)
{
    int window;
    int start;
    int end;

    if(i < 0 || i >= points.size()) return;
    window = qMax(50, total.endIndex / 200);
    start = qMax(0, points[i].index - window);
    end = qMin(total.endIndex, points[i].index + window);
    if(end <= start) return;
    emit sigSegmentSelected(points[i].name, start, end);
}

void DataAnalyzerStatisticsWnd::onUnassignedDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if(item == NULL) return;
    selectPoint(item->data(0, STAT_ROLE_POINT).toInt());
}

int DataAnalyzerStatisticsWnd::cycleReferenceIndex()
{
    int i = cycleReferenceCombo->currentData().toInt();
    if(i == -2) return -2;
    if(i < 0 || i >= edited.size()) return -1;
    return i;
}

void DataAnalyzerStatisticsWnd::markedTotals(double* consumption, double* energy, double* duration)
{
    *consumption = 0;
    *energy = 0;
    *duration = 0;
    for(int i = 0; i < edited.size(); i++)
    {
        if(edited[i].parentIndex < 0)
        {
            *consumption += edited[i].consumption;
            *energy += edited[i].energy;
            *duration += edited[i].duration;
        }
    }
}

double DataAnalyzerStatisticsWnd::editedTotalConsumption()
{
    int ref = cycleReferenceIndex();
    double consumption, energy, duration;

    markedTotals(&consumption, &energy, &duration);
    if(ref >= 0) return edited[ref].consumption;
    if(ref == -2) return consumption;

    double value = total.consumption;
    for(int i = 0; i < edited.size(); i++)
    {
        if(edited[i].parentIndex < 0) value += edited[i].consumption - statistics[i].consumption;
    }
    return value;
}

double DataAnalyzerStatisticsWnd::editedTotalEnergy()
{
    int ref = cycleReferenceIndex();
    double consumption, energy, duration;

    markedTotals(&consumption, &energy, &duration);
    if(ref >= 0) return edited[ref].energy;
    if(ref == -2) return energy;

    double value = total.energy;
    for(int i = 0; i < edited.size(); i++)
    {
        if(edited[i].parentIndex < 0) value += edited[i].energy - statistics[i].energy;
    }
    return value;
}

double DataAnalyzerStatisticsWnd::editedCycleDuration()
{
    int ref = cycleReferenceIndex();
    double consumption, energy, duration;

    markedTotals(&consumption, &energy, &duration);
    if(ref >= 0) return edited[ref].duration;
    if(ref == -2) return duration;

    double value = total.duration;
    for(int i = 0; i < edited.size(); i++)
    {
        if(edited[i].parentIndex < 0) value += edited[i].duration - statistics[i].duration;
    }
    return value;
}

QString DataAnalyzerStatisticsWnd::formatInfo(QString title, double duration, double consumption, double energy)
{
    double avg = (duration > 0) ? consumption * 3600000.0 / duration : 0;
    return title + ": " + formatDuration(duration) + ",  " + formatConsumption(consumption) + " / " + formatEnergy(energy) +
           ",  avg " + QString::number(avg, 'f', 3) + " mA";
}

void DataAnalyzerStatisticsWnd::updateBatteryEstimate()
{
    double capacity = batteryCapacityEdit->text().trimmed().toDouble();
    double cycleConsumption = editedTotalConsumption();
    double cycleDuration = editedCycleDuration();
    double cycleEnergy = editedTotalEnergy();
    double cycles = 0;
    double operatingMs = 0;
    double averageCurrent = 0;
    double targetMs = 0;
    QVector<dataanalyzer_share_t> shares;

    if(cycleDuration > 0)
    {
        averageCurrent = cycleConsumption * 3600000.0 / cycleDuration;
    }
    cycleDurationLabel->setText("Cycle: " + formatDuration(cycleDuration));
    cycleConsumptionLabel->setText("Per cycle: " + formatConsumption(cycleConsumption) + " / " + formatEnergy(cycleEnergy));
    averageCurrentLabel->setText("Avg current: " + QString::number(averageCurrent, 'f', 3) + " mA");

    {
        double markedConsumption, markedEnergy, markedDuration;
        double wholeConsumption = total.consumption;
        double wholeEnergy = total.energy;
        double wholeDuration = total.duration;
        markedTotals(&markedConsumption, &markedEnergy, &markedDuration);
        for(int i = 0; i < edited.size(); i++)
        {
            if(edited[i].parentIndex < 0)
            {
                wholeConsumption += edited[i].consumption - statistics[i].consumption;
                wholeEnergy += edited[i].energy - statistics[i].energy;
                wholeDuration += edited[i].duration - statistics[i].duration;
            }
        }
        wholeInfoLabel->setText(formatInfo("Whole recording", wholeDuration, wholeConsumption, wholeEnergy));
        markedInfoLabel->setText(formatInfo("Marked segments", markedDuration, markedConsumption, markedEnergy));
        unmarkedInfoLabel->setText(formatInfo("Unmarked part", qMax(0.0, wholeDuration - markedDuration), qMax(0.0, wholeConsumption - markedConsumption), qMax(0.0, wholeEnergy - markedEnergy)));
    }

    if(capacity > 0 && cycleConsumption > 0)
    {
        cycles = capacity / cycleConsumption;
        operatingMs = cycles * cycleDuration;
        cyclesLabel->setText("Cycles: " + QString::number(cycles, 'f', 1));
        operatingTimeLabel->setText("Operating time: " + formatClock(operatingMs));
    }
    else
    {
        cyclesLabel->setText("Cycles: -");
        operatingTimeLabel->setText("Operating time: -");
    }

    tableUpdating = true;
    for(int i = 0; i < edited.size(); i++)
    {
        double share = (cycleConsumption > 0) ? edited[i].consumption / cycleConsumption * 100.0 : 0;
        double battery = capacity * share / 100.0;
        double parentShare = 0;
        if(edited[i].parentIndex >= 0 && edited[edited[i].parentIndex].consumption > 0)
        {
            parentShare = edited[i].consumption / edited[edited[i].parentIndex].consumption * 100.0;
        }
        setCell(items[i], STAT_COL_SHARE, share, QString::number(share, 'f', 2), false);
        setCell(items[i], STAT_COL_PARENT_SHARE, parentShare, edited[i].parentIndex >= 0 ? QString::number(parentShare, 'f', 2) : "", false);
        setCell(items[i], STAT_COL_BATTERY, battery, capacity > 0 ? QString::number(battery, 'f', 3) : "", false);
    }
    tableUpdating = false;

    {
        int ref = cycleReferenceIndex();
        QVector<int> map(edited.size(), -1);
        for(int i = 0; i < edited.size(); i++)
        {
            bool inside = false;
            int p = i;
            while(p >= 0)
            {
                if(p == ref) { inside = true; break; }
                p = edited[p].parentIndex;
            }
            if(ref < 0) inside = true;
            if(i == ref) inside = false;
            if(!inside) continue;
            dataanalyzer_share_t sh;
            sh.name = edited[i].name;
            sh.share = (cycleConsumption > 0) ? edited[i].consumption / cycleConsumption * 100.0 : 0;
            sh.color = segmentColor(i);
            sh.startIndex = edited[i].startIndex;
            sh.parentIndex = -1;
            sh.level = 0;
            sh.x0 = 0;
            sh.x1 = 0;
            map[i] = shares.size();
            shares.append(sh);
        }
        for(int i = 0; i < edited.size(); i++)
        {
            if(map[i] < 0) continue;
            int p = edited[i].parentIndex;
            shares[map[i]].parentIndex = (p >= 0 && p != ref && map[p] >= 0) ? map[p] : -1;
        }
    }
    shareBar->setShares(shares);

    if(parseClock(targetTimeEdit->text(), &targetMs) && cycleDuration > 0)
    {
        double requiredCapacity = cycleConsumption * targetMs / cycleDuration;
        requiredCapacityLabel->setText("Required battery: " + QString::number(requiredCapacity, 'f', 1) + " mAh (" + QString::number(targetMs / cycleDuration, 'f', 1) + " cycles)");
    }
    else
    {
        requiredCapacityLabel->setText("Required battery: -");
    }
}

void DataAnalyzerStatisticsWnd::onTargetTimeChanged()
{
    updateBatteryEstimate();
}

void DataAnalyzerStatisticsWnd::onCycleReferenceChanged()
{
    updateBatteryEstimate();
}

void DataAnalyzerStatisticsWnd::applyDelta(int index, double consumptionDelta, double energyDelta, double durationDelta)
{
    int parent = edited[index].parentIndex;
    while(parent >= 0 && parent < edited.size())
    {
        edited[parent].consumption += consumptionDelta;
        edited[parent].energy += energyDelta;
        edited[parent].duration += durationDelta;
        if(edited[parent].consumption < 0) edited[parent].consumption = 0;
        if(edited[parent].energy < 0) edited[parent].energy = 0;
        if(edited[parent].duration < 1) edited[parent].duration = 1;
        edited[parent].avgCurrent = edited[parent].consumption * 3600000.0 / edited[parent].duration;
        refreshItemValues(parent);
        parent = edited[parent].parentIndex;
    }
}

void DataAnalyzerStatisticsWnd::onItemChanged(QTreeWidgetItem *item, int column)
{
    bool ok = false;
    double value;
    double oldConsumption;
    double oldEnergy;
    double oldDuration;
    int i;

    if(tableUpdating) return;
    if(item == NULL) return;
    if(column != STAT_COL_CONSUMPTION && column != STAT_COL_DURATION) return;

    i = item->data(0, STAT_ROLE_SEGMENT).toInt();
    if(i < 0 || i >= edited.size()) return;
    value = item->text(column).trimmed().toDouble(&ok);

    tableUpdating = true;
    oldConsumption = edited[i].consumption;
    oldEnergy = edited[i].energy;
    oldDuration = edited[i].duration;
    if(ok && value >= 0)
    {
        if(column == STAT_COL_CONSUMPTION)
        {
            edited[i].consumption = value;
            edited[i].energy = (statistics[i].consumption > 0) ? statistics[i].energy * edited[i].consumption / statistics[i].consumption : 0;
            edited[i].avgCurrent = (edited[i].duration > 0) ? edited[i].consumption * 3600000.0 / edited[i].duration : 0;
        }
        else
        {
            edited[i].duration = value * 1000.0;
            edited[i].consumption = edited[i].avgCurrent * edited[i].duration / 3600000.0;
            edited[i].energy = (statistics[i].consumption > 0) ? statistics[i].energy * edited[i].consumption / statistics[i].consumption : 0;
        }
    }
    refreshItemValues(i);
    applyDelta(i, edited[i].consumption - oldConsumption, edited[i].energy - oldEnergy, edited[i].duration - oldDuration);
    tableUpdating = false;

    updateBatteryEstimate();
}

void DataAnalyzerStatisticsWnd::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    int i;

    if(item == NULL) return;
    if(column == STAT_COL_CONSUMPTION || column == STAT_COL_DURATION) return;
    i = item->data(0, STAT_ROLE_SEGMENT).toInt();
    if(i < 0)
    {
        selectPoint(item->data(0, STAT_ROLE_POINT).toInt());
        return;
    }
    if(i >= statistics.size()) return;
    emit sigSegmentSelected(statistics[i].name, statistics[i].startIndex, statistics[i].endIndex);
}

void DataAnalyzerStatisticsWnd::onBatteryCapacityChanged()
{
    updateBatteryEstimate();
}

void DataAnalyzerStatisticsWnd::onResetEdits()
{
    edited = statistics;
    fillTable();
    updateBatteryEstimate();
}

void DataAnalyzerStatisticsWnd::onExportCsv()
{
    QString suggested = "oept_" + profileName.section('/', -1).toLower().replace(' ', '_') + "_statistics_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
    QString path = QFileDialog::getSaveFileName(this, "Export statistics", suggested, "CSV (*.csv)");
    if(path.isEmpty()) return;
    if(!path.endsWith(".csv", Qt::CaseInsensitive)) path += ".csv";

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox::warning(this, "Export statistics", "Unable to write file:\n" + path);
        return;
    }

    QTextStream out(&file);
    double capacity = batteryCapacityEdit->text().trimmed().toDouble();
    double cycleConsumption = editedTotalConsumption();

    out << "Segment,First action [ms],Last action [ms],Consumption [mAh],Energy [mJ],Duration [ms],Share of cycle [%],Battery [mAh],Max Current [mA],Min Current [mA],Avg Current [mA],Max Voltage [V],Min Voltage [V],Avg Voltage [V]\n";
    for(int i = 0; i < edited.size(); i++)
    {
        double share = (cycleConsumption > 0) ? edited[i].consumption / cycleConsumption * 100.0 : 0;
        QString path = edited[i].name;
        int parent = edited[i].parentIndex;
        while(parent >= 0 && parent < edited.size())
        {
            path = edited[parent].name + " > " + path;
            parent = edited[parent].parentIndex;
        }
        out << path << ","
            << QString::number(edited[i].startTime, 'g', 12) << ","
            << QString::number(edited[i].endTime, 'g', 12) << ","
            << QString::number(edited[i].consumption, 'g', 9) << ","
            << QString::number(edited[i].energy, 'g', 9) << ","
            << QString::number(edited[i].duration, 'g', 9) << ","
            << QString::number(share, 'g', 9) << ","
            << QString::number(capacity * share / 100.0, 'g', 9) << ","
            << QString::number(edited[i].maxCurrent, 'g', 9) << ","
            << QString::number(edited[i].minCurrent, 'g', 9) << ","
            << QString::number(edited[i].avgCurrent, 'g', 9) << ","
            << QString::number(edited[i].maxVoltage, 'g', 9) << ","
            << QString::number(edited[i].minVoltage, 'g', 9) << ","
            << QString::number(edited[i].avgVoltage, 'g', 9) << "\n";
    }
    if(!points.isEmpty())
    {
        out << "\nMarker,Time [ms],Sample index,Segment\n";
        for(int i = 0; i < points.size(); i++)
        {
            QString path;
            int parent = points[i].parentIndex;
            while(parent >= 0 && parent < edited.size())
            {
                path = path.isEmpty() ? edited[parent].name : edited[parent].name + " > " + path;
                parent = edited[parent].parentIndex;
            }
            out << points[i].name << "," << QString::number(points[i].time, 'g', 12) << "," << points[i].index << "," << (path.isEmpty() ? "unassigned" : path) << "\n";
        }
    }
    out << "\nWhole profile," << QString::number(cycleConsumption, 'g', 9) << "," << QString::number(total.energy, 'g', 9) << "," << QString::number(editedCycleDuration(), 'g', 9) << "\n";
    out << "Battery capacity [mAh]," << QString::number(capacity, 'g', 9) << "\n";
    out << operatingTimeLabel->text() << "\n";
    file.close();
}
