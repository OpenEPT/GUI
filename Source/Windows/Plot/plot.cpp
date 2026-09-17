#include <QtOpenGL>
#include "plot.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgGenerator>
#include <QDateTime>

#define IMAGE_PNG_SCALE     4.0
#define IMAGE_PNG_DPI       300

#define BUTTONS_SIZE 24

Plot::Plot(int mw, int mh, bool aEnableTracking, QWidget *parent)
    : QWidget{parent}
{
    /* Set parent */
    this->setParent(parent);

    /* Create plot*/
    plot = new QCustomPlot();
    plot->setMinimumSize(mw, mh);
    plot->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    plot->setAntialiasedElements(QCP::aeAll);

    plot->addGraph(); // blue line
    plot->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    plot->setInteraction(QCP::iSelectPlottables, true);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);

    //plot->setBackground(QBrush(QColor("#7a7d7c")));

    zoomIn      = new QPushButton();
    zoomOut     = new QPushButton();
    zoomExpand  = new QPushButton();
    zoomArea    = new QPushButton();
    moveGraph   = new QPushButton();
    lockAxis    = new QPushButton();
    saveImage   = new QPushButton();
    trackGraph  = new QPushButton();

    QPixmap zoomInPng(":/images/NewSet/zoom_in.png");
    QIcon zoomInIcon(zoomInPng);
    zoomIn->setIcon(zoomInIcon);
    zoomIn->setIconSize(QSize(14,14));
    zoomIn->setToolTip("Zoom in");
    zoomIn->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);


    QPixmap zoomOutPng(":/images/NewSet/zoom_out.png");
    QIcon zoomOutIcon(zoomOutPng);
    zoomOut->setIcon(zoomOutIcon);
    zoomOut->setIconSize(QSize(14,14));
    zoomOut->setToolTip("Zoom out");
    zoomOut->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);

    QPixmap zoomExpandPng(":/images/NewSet/expand.png");
    QIcon zoomExpandIcon(zoomExpandPng);
    zoomExpand->setIcon(zoomExpandIcon);
    zoomExpand->setIconSize(QSize(14,14));
    zoomExpand->setToolTip("Fit to full data");
    zoomExpand->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);

    QPixmap zoomAreaPng(":/images/NewSet/zoom_area.png");
    QIcon zoomAreaIcon(zoomAreaPng);
    zoomArea->setIcon(zoomAreaIcon);
    zoomArea->setIconSize(QSize(14,14));
    zoomArea->setToolTip("Zoom area");
    zoomArea->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);

    QPixmap moveGraphPng(":/images/NewSet/moveGraph.png");
    QIcon moveGraphIcon(moveGraphPng);
    moveGraph->setIcon(moveGraphIcon);
    moveGraph->setIconSize(QSize(14,14));
    moveGraph->setToolTip("Move graph");
    moveGraph->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);

    QPixmap lockAxisPng(":/images/NewSet/lock.png");
    QIcon lockAxisIcon(lockAxisPng);
    lockAxis->setIcon(lockAxisIcon);
    lockAxis->setIconSize(QSize(14,14));
    lockAxis->setToolTip("Lock X axis with other locked plots");
    lockAxis->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);
    lockAxis->setCheckable(true);

    QPixmap saveImagePng(":/images/NewSet/save.png");
    QIcon saveImageIcon(saveImagePng);
    saveImage->setIcon(saveImageIcon);
    saveImage->setIconSize(QSize(14,14));
    saveImage->setToolTip("Save image (PNG / SVG)");
    saveImage->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);

    QPixmap trackGraphPng(":/images/NewSet/tracking_graph.png");
    QIcon trackGraphIcon(trackGraphPng);
    trackGraph->setIcon(trackGraphIcon);
    trackGraph->setIconSize(QSize(14,14));
    trackGraph->setToolTip("Enable graph tracking");
    trackGraph->setFixedSize(BUTTONS_SIZE, BUTTONS_SIZE);


    QVBoxLayout *buttonsLayout = new QVBoxLayout();
    buttonsLayout->addWidget(zoomIn);
    buttonsLayout->addWidget(zoomOut);
    buttonsLayout->addWidget(zoomExpand);
    buttonsLayout->addWidget(zoomArea);
    buttonsLayout->addWidget(moveGraph);
    buttonsLayout->addWidget(lockAxis);
    buttonsLayout->addWidget(saveImage);
    buttonsLayout->addWidget(trackGraph);
    buttonsLayout->setAlignment(Qt::AlignCenter);
    buttonsLayout->setSpacing(2);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);


    QHBoxLayout *plotLayout = new QHBoxLayout(this);
    plotLayout->addLayout(buttonsLayout);
    plotLayout->addWidget(plot);
    plotLayout->setAlignment(Qt::AlignLeft);
    plotLayout->setSpacing(3);

    plot->plotLayout()->insertRow(0);
    title = new QCPTextElement(plot, "NN", QFont("Helvetica", 16));
    plot->plotLayout()->addElement(0, 0, title);

    enableTracking      = aEnableTracking;
    replotActive        = true;
    scatterGraphAdded   = false;
    axisLocked          = false;
    xRangeSyncInProgress = false;

    scatterFont = new QFont("Times", 14);
    scatterFont->setBold(true);

    connect(zoomIn, SIGNAL(pressed()), this, SLOT(onZoomIn()));
    connect(zoomOut, SIGNAL(pressed()), this, SLOT(onZoomOut()));
    connect(zoomExpand, SIGNAL(pressed()), this, SLOT(onZoomExpand()));
    connect(zoomArea, SIGNAL(pressed()), this, SLOT(onZoomArea()));
    connect(moveGraph, SIGNAL(pressed()), this, SLOT(onMoveGraph()));
    connect(lockAxis, SIGNAL(clicked()), this, SLOT(onLockAxis()));
    connect(saveImage, SIGNAL(clicked()), this, SLOT(onSaveImage()));
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onXRangeChanged(QCPRange)));
    connect(trackGraph, SIGNAL(pressed()), this, SLOT(onTrackGraph()));
    setButtonStyle();
}

void        Plot::scatterAddGraph()
{

    scatterGraphAdded = true;
    plot->addGraph();

    plot->graph(1)->setLineStyle(QCPGraph::lsNone);  // No line
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::red, 10));  // Red circle marker, size 10

}

void Plot::scatterAddData(QVector<double> data, QVector<double> keys)
{
    plot->graph(1)->setData(keys, data);

    for(int i = 0; i < data.size(); i++)
    {
        QCPItemText *textLabel = new QCPItemText(plot);

        // Set text label position above each point
        textLabel->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        textLabel->position->setType(QCPItemPosition::ptPlotCoords);  // Position in plot coordinates
        textLabel->position->setCoords(keys[i], data[i] + 0.5);  // Set position slightly above the point

        // Set text style and content
        textLabel->setText(QString::number(i));  // Set the text (label)
        textLabel->setFont(QFont("Times", 14));  // Set font and size
        textLabel->setColor(Qt::red);  // Set text color
    }

    plot->replot();
}

void Plot::scatterAddAllDataWithName(QVector<QPair<QString, int>> data)
{
    for(int i = 0; i < data.size(); i++)
    {
        if(data[i].second >=  xData.size()) break;
        plot->graph(1)->addData(xData[data[i].second], yData[data[i].second]);
        QCPItemText *textLabel = new QCPItemText(plot);

        // Set text label position above each point
        textLabel->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        textLabel->position->setType(QCPItemPosition::ptPlotCoords);  // Position in plot coordinates
        textLabel->position->setCoords(xData[data[i].second], yData[data[i].second]);  // Set position slightly above the point

        // Set text style and content
        textLabel->setText(data[i].first);  // Set the text (label)
        textLabel->setFont(*scatterFont);  // Set font and size
        textLabel->setColor(Qt::black);  // Set text color


        textLabel->setClipToAxisRect(false);
        textData.push_back(textLabel);
    }

    plot->replot();
}

QCPItemText* Plot::createMarkerLabel(double x, double y, QString name)
{
    QCPItemText *textLabel = new QCPItemText(plot);
    textLabel->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    textLabel->position->setType(QCPItemPosition::ptPlotCoords);
    textLabel->position->setCoords(x, y);
    textLabel->setText(name);
    textLabel->setFont(*scatterFont);
    textLabel->setColor(Qt::red);
    textLabel->setClipToAxisRect(false);
    textData.push_back(textLabel);
    return textLabel;
}

void Plot::rescaleYWithMarkers()
{
    QCPRange range;
    double padding;

    plot->yAxis->rescale(true);
    if(textData.isEmpty()) return;
    range = plot->yAxis->range();
    padding = range.size() * 0.15;
    if(padding <= 0) padding = 1.0;
    plot->yAxis->setRange(range.lower, range.upper + padding);
}

void Plot::trimMarkers(double minKey)
{
    if(!scatterGraphAdded) return;
    plot->graph(1)->data()->removeBefore(minKey);
    for(int i = 0; i < textData.size(); i++)
    {
        textData[i]->setVisible(textData[i]->position->key() >= minKey);
    }
}

void Plot::showAllMarkers()
{
    if(!scatterGraphAdded) return;
    plot->graph(1)->data()->clear();
    for(int i = 0; i < textData.size(); i++)
    {
        textData[i]->setVisible(true);
        plot->graph(1)->addData(textData[i]->position->key(), textData[i]->position->value());
    }
}

void Plot::scatterAddDataWithName(double value, double keys, QString name)
{
    if(keys < 0 || keys >= xData.size() || keys >= yData.size())
    {
        qDebug() << "Corresponding data not arrived";
        epDataKey.append(keys);
        epDataName.append(name);
        return;
    }
    plot->graph(1)->addData(xData[keys], yData[keys]);
    createMarkerLabel(xData[keys], yData[keys], name);

    rescaleYWithMarkers();
    plot->replot();
}

void Plot::scatterReplotDataWithName()
{
    int key;
    double value;
    int i = 0;
    while(i < epDataKey.size())
    {
        if(epDataKey[i] < 0 || epDataKey[i] >= xData.size() || epDataKey[i] >= yData.size())
        {
            i++;
            continue;
        }
        key = epDataKey[i];
        value = yData[key];
        plot->graph(1)->addData(xData[key], yData[key]);
        createMarkerLabel(xData[key], value, epDataName[i]);

        rescaleYWithMarkers();
        plot->replot();
        epDataKey.removeAt(i);
        epDataName.removeAt(i);
    }
}
void        Plot::setData(QVector<double> data, QVector<double> keys)
{
    xData = keys;
    yData = data;
    if(replotActive)
    {
        plot->graph(0)->setData(xData, yData, true);
        plot->rescaleAxes(true);
        plot->replot();
    }
}
void        Plot::appendData(QVector<double> data, QVector<double> keys)
{
    if(keys.isEmpty() || keys.size() != data.size()) return;
    xData.append(keys);
    yData.append(data);
    plotXData.append(keys);
    plotYData.append(data);
    if((plotXData.last() > 10000) && (plotXData.size() > data.size()))
    {
        plotXData.remove(0,data.size());
        plotYData.remove(0,data.size());
        if(replotActive)
        {
            trimMarkers(plotXData.first());
        }
    }
    if(replotActive)
    {
        plot->graph(0)->setData(plotXData, plotYData, true);
        if(plotXData.first() < plotXData.last())
        {
            plot->xAxis->setRange(plotXData.first(), plotXData.last());
        }
        rescaleYWithMarkers();
        plot->replot();
        scatterReplotDataWithName();
    }

}
void        Plot::setYRange(double min, double max)
{
    plot->yAxis->setRange(min, max);
    plot->replot();
}
void        Plot::setYLabel(QString label)
{
    plot->yAxis->setLabel(label);
    plot->replot();
}
void        Plot::setXRange(double min, double max)
{
    plot->xAxis->setRange(min, max);
    plot->replot();
}
void        Plot::setXLabel(QString label)
{
    plot->xAxis->setLabel(label);
    plot->replot();
}
void        Plot::setTitle(QString aTitle)
{
    title->setText(aTitle);
    plot->replot();
}
void        Plot::clear()
{
    plot->graph(0)->data()->clear();
    if(scatterGraphAdded)
    {
        plot->graph(1)->data()->clear();
        for(int i =0; i < textData.size(); i++)
        {
            plot->removeItem(textData[i]);
        }
        textData.clear();
    }
    xData.clear();
    yData.clear();
    plot->replot();
    epDataKey.clear();
    epDataName.clear();
    plotXData.clear();
    plotYData.clear();
    plot->xAxis->setRange(0, 1000);
    plot->replot();

}
void        Plot::onZoomIn()
{
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->xAxis->scaleRange(.85, plot->xAxis->range().center());
    plot->yAxis->scaleRange(.85, plot->yAxis->range().center());
    plot->replot();
    setButtonStyle();
}
void        Plot::onZoomOut()
{
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->xAxis->scaleRange(1.25, plot->xAxis->range().center());
    plot->yAxis->scaleRange(1.25, plot->yAxis->range().center());
    plot->replot();
    setButtonStyle();
}
void        Plot::onZoomExpand()
{
    plot->graph(0)->setData(xData, yData, true);
    showAllMarkers();
    plot->setInteraction(QCP::iRangeDrag, false);
    plot->setInteraction(QCP::iRangeZoom, false);
    plot->rescaleAxes(true);
    rescaleYWithMarkers();
    plot->setSelectionRectMode(QCP::srmZoom);
    plot->replot();
    setButtonStyle();
}
void        Plot::onZoomArea()
{
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    plot->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
    plot->setSelectionRectMode(QCP::srmZoom);
    plot->replot();
    setButtonStyle();
}
void        Plot::onMoveGraph()
{
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, false);
    plot->setInteraction(QCP::iSelectPlottables, true);
    plot->setSelectionRectMode(QCP::srmNone);
    plot->replot();
    setButtonStyle();
}
void       Plot::onTrackGraph()
{
    enableTracking = enableTracking == false? true : false;
    replotActive = enableTracking;
    if(!enableTracking)
    {
        showAllMarkers();
        plot->replot();
    }
    setButtonStyle();
}

void Plot::setButtonStyle()
{
    if(enableTracking)
    {
        trackGraph->setStyleSheet("background-color:  rgb(255,197,172);");
        zoomIn->setEnabled(false);
        zoomOut->setEnabled(false);
        moveGraph->setEnabled(false);
        zoomExpand->setEnabled(false);
        zoomArea->setEnabled(false);
        saveImage->setEnabled(false);
        lockAxis->setEnabled(false);
    }
    else
    {
        trackGraph->setStyleSheet("background-color:  rgb(255,255,255);");
        zoomIn->setEnabled(true);
        zoomOut->setEnabled(true);
        moveGraph->setEnabled(true);
        zoomExpand->setEnabled(true);
        zoomArea->setEnabled(true);
        saveImage->setEnabled(true);
        lockAxis->setEnabled(true);
    }
}

void Plot::onLockAxis()
{
    axisLocked = lockAxis->isChecked();
    if(axisLocked)
    {
        lockAxis->setStyleSheet("background-color:  rgb(255,197,172);");
        emit sigXRangeChanged(plot->xAxis->range());
    }
    else
    {
        lockAxis->setStyleSheet("background-color:  rgb(255,255,255);");
    }
    emit sigAxisLockChanged(axisLocked);
}

bool Plot::isAxisLocked()
{
    return axisLocked;
}

void Plot::onXRangeChanged(const QCPRange &range)
{
    if(!axisLocked) return;
    if(xRangeSyncInProgress) return;
    emit sigXRangeChanged(range);
}

void Plot::zoomToKeyRange(double min, double max)
{
    double margin = (max - min) * 0.05;

    if(max <= min) return;
    if(enableTracking) return;

    plot->graph(0)->setData(xData, yData, true);
    if(scatterGraphAdded) showAllMarkers();
    xRangeSyncInProgress = true;
    plot->xAxis->setRange(min - margin, max + margin);
    xRangeSyncInProgress = false;
    plot->graph(0)->rescaleValueAxis(false, true);
    rescaleYWithMarkers();
    if(!textData.isEmpty())
    {
        plot->graph(0)->rescaleValueAxis(false, true);
        QCPRange range = plot->yAxis->range();
        plot->yAxis->setRange(range.lower, range.upper + range.size() * 0.15);
    }
    plot->replot();
    if(axisLocked)
    {
        emit sigXRangeChanged(plot->xAxis->range());
    }
}

void Plot::setXRangeSynced(QCPRange range)
{
    if(!axisLocked) return;
    if(plot->xAxis->range() == range) return;
    xRangeSyncInProgress = true;
    plot->xAxis->setRange(range);
    plot->replot();
    xRangeSyncInProgress = false;
}

void Plot::onSaveImage()
{
    QString pngFilter = "PNG image (*.png)";
    QString svgFilter = "SVG image (*.svg)";
    QString selectedFilter = pngFilter;
    QString suggestedName;
    QString path;
    bool ok;

    suggestedName = "oept_" + (title->text().isEmpty() ? "plot" : title->text().toLower().replace(' ', '_')) + "_" +
                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    path = QFileDialog::getSaveFileName(this, "Save plot image", suggestedName + ".png", pngFilter + ";;" + svgFilter, &selectedFilter);
    if(path.isEmpty()) return;

    if(path.endsWith(".svg", Qt::CaseInsensitive))
    {
        ok = saveImageAsSvg(path);
    }
    else if(path.endsWith(".png", Qt::CaseInsensitive))
    {
        ok = saveImageAsPng(path);
    }
    else if(selectedFilter == svgFilter)
    {
        ok = saveImageAsSvg(path + ".svg");
    }
    else
    {
        ok = saveImageAsPng(path + ".png");
    }

    if(!ok)
    {
        QMessageBox::warning(this, "Save plot image", "Unable to save image to:\n" + path);
    }
}

QString Plot::getTitle()
{
    return title->text();
}

bool Plot::saveImageToFile(QString path)
{
    if(path.endsWith(".svg", Qt::CaseInsensitive)) return saveImageAsSvg(path);
    return saveImageAsPng(path);
}

bool Plot::saveImageAsPng(QString path)
{
    return plot->savePng(path, 0, 0, IMAGE_PNG_SCALE, -1, IMAGE_PNG_DPI);
}

bool Plot::saveImageAsSvg(QString path)
{
    QSvgGenerator generator;
    int width = plot->width();
    int height = plot->height();

    generator.setFileName(path);
    generator.setSize(QSize(width, height));
    generator.setViewBox(QRect(0, 0, width, height));
    generator.setTitle(title->text());
    generator.setDescription("OpenEPT plot export");

    QCPPainter painter;
    if(!painter.begin(&generator)) return false;
    painter.setMode(QCPPainter::pmVectorized);
    painter.setMode(QCPPainter::pmNoCaching);
    painter.setMode(QCPPainter::pmNonCosmetic);
    plot->toPainter(&painter, width, height);
    painter.end();

    return true;
}
