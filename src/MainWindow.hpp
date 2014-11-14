#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Gauge.hpp"
#include "TelemetryStream.hpp"


#include <QPushButton>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QSettings>

#include <functional>


/* TODO:
 * - Prevent settings dialog from allowing user to select both streams on
 * the same port.
 */


class GaugeUpdater : public QObject
{
    Q_OBJECT
    
public slots:
    void update(const TelemetryVariable &var);

public:
    typedef std::function<void(double)> updater;
    void link(const QString &label, updater updater);
    
private:
    QMap<QString, updater> m_updaters;
};


class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString emsPort READ emsPort WRITE setEmsPort
               NOTIFY emsPortChanged)
    Q_PROPERTY(QString efisPort READ efisPort WRITE setEfisPort
               NOTIFY efisPortChanged)
    Q_PROPERTY(QString logFolder READ logFolder WRITE setLogFolder
               NOTIFY logFolderChanged)

public:
    Settings();
    QString emsPort() const {return m_emsPort;}
    QString efisPort() const {return m_efisPort;}
    QString logFolder() const {return m_logFolder;}
    void setEmsPort(const QString &newEmsPort);
    void setEfisPort(const QString &newEmsPort);
    void setLogFolder(const QString &newLogFolder);

signals:
    void emsPortChanged(const QString &newEmsPort);
    void efisPortChanged(const QString &newEfisPort);
    void logFolderChanged(const QString &newLogFolder);

public slots:
    void sync();

private:
    QSettings m_storedSettings;
    QString m_emsPort, m_efisPort, m_logFolder;
};


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(Settings *settings, QWidget *parent=0);

public slots:
    void chooseLogFolder();
    void saveSettings();

protected:
    QComboBox *m_emsPortComboBox, *m_efisPortComboBox;
    QPushButton *m_logFolderButton;
    Settings *m_settings;
    
    void setCurrentPort(const QString &portName, QComboBox *comboBox);
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);

public slots:
    void efisOnline();
    void efisOffline();
    void emsOnline();
    void emsOffline();
    void showSettingsDialog();
    void updateLogFolder(const QString &logFolder);

private:
    Settings m_settings;
    GaugeUpdater m_updater;
    EfisStream *m_efisStream;
    EmsStream *m_emsStream;
    QLabel *m_efisStatusLabel, *m_emsStatusLabel;
    QTimer *m_efisStatusTimer, *m_emsStatusTimer;
};

#endif // MAINWINDOW_HPP
