#ifndef DEVICEDIALOG_H
#define DEVICEDIALOG_H

#include <mutex>
#include <QDialog>
#include <QTimer>
#include "RtAudio.h"
#include "definitions.h"

namespace Ui {
class DeviceDialog;
}

class DeviceDialog : public QDialog
{
    Q_OBJECT

public:
    DeviceDialog(RtAudio::Api api, RtAudio::DeviceInfo deviceInfo, QWidget *parent = nullptr);
    ~DeviceDialog();

private slots:
    void onDeviceSettingsChanged();
    void onActivateDeviceCheckBoxToggled(bool);
    void onRunTimer();

private:
    Ui::DeviceDialog *ui;
    RtAudio m_rtAudio;
    RtAudio::DeviceInfo m_deviceInfo;
    QTimer m_runTimer;

    struct RuntimeData {
        // Setup
        unsigned int inChannelCount=0;
        unsigned int outChannelCount=0;
        unsigned int sampleRate=0;
        unsigned int bufferSize=0;
        // UI => callback
        double sinFrequency;
        double sinAmplitude;

        // Callback => UI
        std::vector<float> inChannelsAmplitude;
        std::vector<float> inChannelsAverageAmplitude;
        std::vector<float> inChannelsPeakAmplitude;
        unsigned int lastErrorStatus=0;
    };
    std::mutex m_runtimeDataMutex;
    RuntimeData m_runtimeData;

    friend int cRtAudioCallback(void* outputBuffer, void* inputBuffer,
                                unsigned int nFrames, double streamTime,
                                RtAudioStreamStatus status, void* userData);
    int rtAudioCallback(void* outputBuffer, void* inputBuffer,
                         unsigned int nFrames, double streamTime,
                         RtAudioStreamStatus status);
    void updateRuntimeWidgets();
};

#endif // DEVICEDIALOG_H
