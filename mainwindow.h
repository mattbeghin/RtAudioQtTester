#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "RtAudio.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum RtAudioRoles {
        DeviceInfoRole = Qt::UserRole+0,
        ApiRole = Qt::UserRole+1
    };

private slots:
    void onSelectionChanged();

private:
    Ui::MainWindow *ui;
    QVector<RtAudio*> m_rtAudioInterfaces;
};

Q_DECLARE_METATYPE(RtAudio::DeviceInfo)
Q_DECLARE_METATYPE(RtAudio::Api)

#endif // MAINWINDOW_H
