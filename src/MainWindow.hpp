#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Gauge.hpp"
#include "TelemetryStream.hpp"


#include <QMainWindow>
#include <QMap>
#include <QSettings>


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


class Settings : public QObject
{
    Q_OBJECT;
    Q_PROPERTY(QString emsPort MEMBER m_emsPort WRITE setEmsPort
               NOTIFY emsPortChanged)
    Q_PROPERTY(QString efisPort MEMBER m_efisPort WRITE setEfisPort
               NOTIFY efisPortChanged)

public:
    Settings();
    void setEmsPort(const QString &newEmsPort);
    void setEfisPort(const QString &newEmsPort);

signals:
    void emsPortChanged(const QString &newEmsPort);
    void efisPortChanged(const QString &newEfisPort);

private:
    QSettings m_storedSettings;
    QString m_emsPort, m_efisPort;
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
