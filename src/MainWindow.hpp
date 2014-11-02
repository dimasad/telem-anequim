#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Gauge.hpp"
#include "TelemetryStream.hpp"


#include <QComboBox>
#include <QDialog>
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
    Q_OBJECT
    Q_PROPERTY(QString emsPort READ emsPort WRITE setEmsPort
               NOTIFY emsPortChanged)
    Q_PROPERTY(QString efisPort READ efisPort WRITE setEfisPort
               NOTIFY efisPortChanged)

public:
    Settings();
    QString emsPort() const {return m_emsPort;}
    QString efisPort() const {return m_efisPort;}
    void setEmsPort(const QString &newEmsPort);
    void setEfisPort(const QString &newEmsPort);

signals:
    void emsPortChanged(const QString &newEmsPort);
    void efisPortChanged(const QString &newEfisPort);

public slots:
    void sync();

private:
    QSettings m_storedSettings;
    QString m_emsPort, m_efisPort;
};


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(Settings *settings, QWidget *parent=0);

public slots:
    void saveSettings();

protected:
    QComboBox m_emsPortComboBox, m_efisPortComboBox;
    Settings *m_settings;
    
    void setCurrentPort(const QString &portName, QComboBox &comboBox);
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
 public:
    explicit MainWindow(QWidget *parent = 0);
    
 private:
    Settings m_settings;
    GaugeUpdater m_updater;
};

#endif // MAINWINDOW_HPP
