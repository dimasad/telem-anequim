#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Gauge.hpp"
#include "TelemetryStream.hpp"


#include <QMainWindow>
#include <QMap>


class GaugeUpdater : public QObject
{
    Q_OBJECT
    
public slots:
    void update(const TelemetryVariable &var);

public:
    void link(const QString &label, Gauge *gauge);
    
private:
    QMap<QString, Gauge*> m_gauges;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
 public:
    explicit MainWindow(QWidget *parent = 0);
    
 private:
    GaugeUpdater m_updater;
};

#endif // MAINWINDOW_HPP
