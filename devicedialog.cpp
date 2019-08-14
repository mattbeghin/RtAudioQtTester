#include <cmath>
#include "devicedialog.h"
#include "ui_devicedialog.h"
#include <QDebug>

DeviceDialog::DeviceDialog(RtAudio::Api api, RtAudio::DeviceInfo deviceInfo, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceDialog),
    m_rtAudio(api),
    m_deviceInfo(deviceInfo)
{
    ui->setupUi(this);

    auto displayName = deviceInfo.name + " (" + m_rtAudio.getApiDisplayName(api) + ")";
    setWindowTitle(QString::fromUtf8(displayName.c_str()));

    ui->inputChannelsSpinBox->setMinimum(0);
    ui->inputChannelsSpinBox->setMaximum(static_cast<int>(m_deviceInfo.inputChannels));
    ui->inputChannelsSpinBox->setValue(static_cast<int>(m_deviceInfo.inputChannels));
    ui->outputChannelsSpinBox->setMinimum(0);
    ui->outputChannelsSpinBox->setMaximum(static_cast<int>(m_deviceInfo.outputChannels));
    ui->outputChannelsSpinBox->setValue(static_cast<int>(m_deviceInfo.outputChannels));
    ui->sampleRateComboBox->clear();
    for (auto sampleRate: deviceInfo.sampleRates) {
        ui->sampleRateComboBox->addItem(QString::number(sampleRate));
    }
    ui->sampleRateComboBox->setCurrentText(QString::number(deviceInfo.preferredSampleRate));

    ui->sampleFormatBox->clear();
    for (auto it: s_rtAudioFormatStr) {
        if (deviceInfo.nativeFormats & it.first) {
            ui->sampleFormatBox->addItem(QString::fromUtf8(it.second.c_str()));
        }
    }
    ui->sampleFormatBox->setCurrentText(QString::fromUtf8(s_rtAudioFormatStr.at(RTAUDIO_FLOAT32).c_str()));

    // Setting 0 is an idea, but I will first report a possible crash on macOS
    // when setting 0 and opening microphone input channel at 96000Hz
    // Address sanitizer detects a buffer overrun (macOS 10.14.3 at least)
    ui->bufferSizeSpinBox->setValue(32);

    onDeviceSettingsChanged();
    connect(ui->sampleRateComboBox,SIGNAL(currentTextChanged(QString)),this,SLOT(onDeviceSettingsChanged()));
    connect(ui->bufferSizeSpinBox,SIGNAL(valueChanged(int)),this,SLOT(onDeviceSettingsChanged()));
    connect(ui->inputChannelsSpinBox,SIGNAL(valueChanged(int)),this,SLOT(onDeviceSettingsChanged()));
    connect(ui->outputChannelsSpinBox,SIGNAL(valueChanged(int)),this,SLOT(onDeviceSettingsChanged()));
    connect(ui->activateDeviceCheckBox,SIGNAL(toggled(bool)),this,SLOT(onDeviceSettingsChanged()));

    connect(ui->activateDeviceCheckBox,SIGNAL(toggled(bool)),this,SLOT(onActivateDeviceCheckBoxToggled(bool)));

    connect(&m_runTimer,SIGNAL(timeout()),this,SLOT(onRunTimer()));

    connect(ui->outFrequencySpinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),[this](double value){
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        m_runtimeData.sinFrequency = value;
    });
    connect(ui->outAmplitudeSpinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),[this](double value){
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        m_runtimeData.sinAmplitude = value;
    });

    ui->inputMonitoringGroupBox->setVisible(m_deviceInfo.inputChannels > 0);
    ui->outputTestingGroupBox->setVisible(m_deviceInfo.outputChannels > 0);
}

DeviceDialog::~DeviceDialog()
{
    ui->activateDeviceCheckBox->setChecked(false);
    delete ui;
}

void DeviceDialog::onDeviceSettingsChanged()
{
    if (ui->bufferSizeSpinBox->value() == 0) {
        ui->bufferSizeDetailLabel->setText(QString("samples (0 = lowest possible)"));
    } else {
        auto sampleRate = ui->sampleRateComboBox->currentText().toInt();
        if (sampleRate == 0) {
            ui->bufferSizeDetailLabel->setText(QString("samples (INVALID)"));
        } else {
            ui->bufferSizeDetailLabel->setText(QString("samples (%1 ms)").arg(1000.0 * ui->bufferSizeSpinBox->value() / sampleRate));
        }
    }
    ui->activateDeviceCheckBox->setEnabled(ui->inputChannelsSpinBox->value()>0 || ui->outputChannelsSpinBox->value()>0);

    ui->settingsWidget->setEnabled(!ui->activateDeviceCheckBox->isChecked());
}

int cRtAudioCallback(void* outputBuffer, void* inputBuffer,
                     unsigned int nFrames, double streamTime,
                     RtAudioStreamStatus status, void* userData)
{
    return static_cast<DeviceDialog*>(userData)->rtAudioCallback(outputBuffer, inputBuffer, nFrames, streamTime, status);
}

static std::mutex gErrorStringMutex;
static std::string gLastError;

void setGErrorString(const std::string& errorText)
{
    std::lock_guard<std::mutex> guard(gErrorStringMutex);
    gLastError = errorText;
}

std::string getGErrorString()
{
    std::lock_guard<std::mutex> guard(gErrorStringMutex);
    auto res = gLastError;
    gLastError.clear();
    return res;
}

void rtAudioErrorCallback(RtAudioError::Type /*type*/,
                          const std::string& errorText)
{
    qDebug() << "RtAudio error: " << errorText.c_str();
    setGErrorString(errorText);
    // TODO: would be nice to transmit the error to the UI log area but we're missing user ptr
    // RtAudio audio patch needed
    //ui->logTextEdit->appendPlainText("Error:" + std2q(errorText) + "\n");
}

void DeviceDialog::onActivateDeviceCheckBoxToggled(bool activate)
{
    if (activate) {
        setGErrorString("");
        unsigned int deviceId=0;
        bool found=false;
        unsigned int deviceCount = m_rtAudio.getDeviceCount();
        for (deviceId=0; deviceId<deviceCount; deviceId++) {
            auto devInfo = m_rtAudio.getDeviceInfo(deviceId);
            if (devInfo.name==m_deviceInfo.name && devInfo.inputChannels==m_deviceInfo.inputChannels && devInfo.outputChannels==m_deviceInfo.outputChannels) {
                found = true;
                break;
            }
        }
        if (!found) {
            ui->logTextEdit->appendPlainText("Error starting audio: device not connected\n");
            ui->activateDeviceCheckBox->setChecked(false);
            return;
        }

        m_runtimeData.inChannelCount = static_cast<unsigned int>(ui->inputChannelsSpinBox->value());
        m_runtimeData.outChannelCount = static_cast<unsigned int>(ui->outputChannelsSpinBox->value());
        m_runtimeData.sampleRate = ui->sampleRateComboBox->currentText().toUInt();
        m_runtimeData.inChannelsAmplitude = std::vector<float>(m_runtimeData.inChannelCount,0.0f);
        m_runtimeData.inChannelsAverageAmplitude = std::vector<float>(m_runtimeData.inChannelCount,0.0f);
        m_runtimeData.inChannelsPeakAmplitude = std::vector<float>(m_runtimeData.inChannelCount,0.0f);
        m_runtimeData.bufferSize = static_cast<unsigned int>(ui->bufferSizeSpinBox->value());
        m_runtimeData.lastErrorStatus = 0;

        m_runtimeData.sinFrequency = ui->outFrequencySpinBox->value();
        m_runtimeData.sinAmplitude = ui->outAmplitudeSpinBox->value();

        RtAudio::StreamParameters inputStreamParams;
        inputStreamParams.deviceId = deviceId;
        inputStreamParams.firstChannel = 0;
        inputStreamParams.nChannels = static_cast<unsigned int>(m_runtimeData.inChannelCount);

        RtAudio::StreamParameters outputStreamParams;
        outputStreamParams.deviceId = deviceId;
        outputStreamParams.firstChannel = 0;
        outputStreamParams.nChannels = static_cast<unsigned int>(m_runtimeData.outChannelCount);

        ui->logTextEdit->appendPlainText(QString("Starting audio with %1 inputs & %2 outputs").arg(inputStreamParams.nChannels).arg(outputStreamParams.nChannels));
        updateRuntimeWidgets();

        m_rtAudio.openStream(outputStreamParams.nChannels>0?&outputStreamParams:nullptr,
                             inputStreamParams.nChannels>0?&inputStreamParams:nullptr,
                             RTAUDIO_FLOAT32,
                             m_runtimeData.sampleRate,
                             &m_runtimeData.bufferSize, &cRtAudioCallback, this, nullptr, &rtAudioErrorCallback);

        auto errorStr = getGErrorString();
        if (!errorStr.empty()) {
            ui->logTextEdit->appendPlainText("Error:" + QString::fromStdString(errorStr) + "\n");
        }

        if (m_rtAudio.isStreamOpen()) {
            m_rtAudio.startStream();

            auto errorStr = getGErrorString();
            if (!errorStr.empty()) {
                ui->logTextEdit->appendPlainText("Error:" + QString::fromStdString(errorStr) + "\n");
            }

            if (m_rtAudio.isStreamRunning()) {
                ui->logTextEdit->appendPlainText(QString(" -> Real buffer Size: %1").arg(m_runtimeData.bufferSize));
                m_runTimer.start(10);
            }
        }
    } else {
        ui->logTextEdit->appendPlainText("Stopping\n");

        m_runTimer.stop();

        if (m_rtAudio.isStreamRunning()) {
            m_rtAudio.stopStream();
        }
        if (m_rtAudio.isStreamOpen()) {
            m_rtAudio.closeStream();
        }

        updateRuntimeWidgets();
    }
}

void DeviceDialog::updateRuntimeWidgets()
{
    RuntimeData runtimeData;
    {
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        runtimeData = m_runtimeData;
    }
    ui->inputMonitoringGroupBox->setVisible(runtimeData.inChannelCount > 0);
    ui->outputTestingGroupBox->setVisible(runtimeData.outChannelCount > 0);
}

int DeviceDialog::rtAudioCallback(void* outputBuffer, void* inputBuffer,
                     unsigned int nFrames, double streamTime,
                     RtAudioStreamStatus status)
{
    RuntimeData runtimeData;
    {
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        runtimeData = m_runtimeData;
    }

    // Sinusoides in each output channel
    float* outPtr = static_cast<float*>(outputBuffer);
    for (size_t frame=0; frame<nFrames; frame++) {
        auto sinValue = sin(streamTime*runtimeData.sinFrequency);
        for (unsigned int outChannel=0; outChannel<runtimeData.outChannelCount; outChannel++) {
            *outPtr++ = static_cast<float>(sinValue * runtimeData.sinAmplitude);
        }
        streamTime += 1.0/runtimeData.sampleRate;
    }

    // Compute input channels amplitude
    std::fill(runtimeData.inChannelsAmplitude.begin(), runtimeData.inChannelsAmplitude.end(), 0.0f);
    float* inPtr = static_cast<float*>(inputBuffer);
    for (size_t frame=0; frame<nFrames; frame++) {
        for (size_t inChannel=0; inChannel<runtimeData.inChannelCount; inChannel++) {
            runtimeData.inChannelsAmplitude[inChannel] += std::abs(*inPtr++);
        }
    }
    for (size_t inChannel=0; inChannel<runtimeData.inChannelCount; inChannel++) {
        runtimeData.inChannelsAmplitude[inChannel] /= nFrames; // Divide once after to avoid dividing nFrames time
    }
    // Compute input channels average & peak amplitude
    float averageAmplitudeFactor = nFrames*0.0001f;
    for (size_t inChannel=0; inChannel<runtimeData.inChannelCount; inChannel++) {
        // Average
        runtimeData.inChannelsAverageAmplitude[inChannel] *= 1-averageAmplitudeFactor;
        runtimeData.inChannelsAverageAmplitude[inChannel] += averageAmplitudeFactor * runtimeData.inChannelsAmplitude[inChannel];
        // Peak
        runtimeData.inChannelsPeakAmplitude[inChannel] = std::max(runtimeData.inChannelsPeakAmplitude[inChannel],
                                                                  runtimeData.inChannelsAmplitude[inChannel]);
    }

    // Transmit error status to UI thread
    if (status != 0) {
        runtimeData.lastErrorStatus = status;
    }

    // Update runtime data
    {
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        m_runtimeData = runtimeData;
    }

    return 0;
}

void DeviceDialog::onRunTimer()
{
    auto errorStr = getGErrorString();
    if (!errorStr.empty()) {
        ui->logTextEdit->appendPlainText("Error:" + QString::fromStdString(errorStr) + "\n");
    }

    RuntimeData runtimeData;
    {
        std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
        runtimeData = m_runtimeData;
    }

    // Log input channels amplitude
    if (runtimeData.inChannelCount > 0) {
        QString inChannelsAmplitudeStr;
        QString inChannelsAverageAmplitudeStr;
        QString inChannelsPeakAmplitudeStr;
        for (size_t i=0; i<runtimeData.inChannelCount; i++) {
            inChannelsAmplitudeStr += QString("In %1: %2;").arg(i).arg(static_cast<double>(runtimeData.inChannelsAmplitude[i]),8,'f',6,'0');
            inChannelsAverageAmplitudeStr += QString("In %1: %2;").arg(i).arg(static_cast<double>(runtimeData.inChannelsAverageAmplitude[i]),8,'f',6,'0');
            inChannelsPeakAmplitudeStr += QString("In %1: %2;").arg(i).arg(static_cast<double>(runtimeData.inChannelsPeakAmplitude[i]),8,'f',6,'0');
        }
        ui->inputAmplitudeLabel->setText("Amplitude per channel:         " + inChannelsAmplitudeStr + "\n" +
                                         "Average amplitude per channel: " + inChannelsAverageAmplitudeStr + "\n" +
                                         "Peak amplitude per channel:    " + inChannelsPeakAmplitudeStr + "\n");
    }

    // Log error status
    if (runtimeData.lastErrorStatus != 0) {
        if (runtimeData.lastErrorStatus == RTAUDIO_INPUT_OVERFLOW) {
            ui->logTextEdit->appendPlainText(QString("Device error reported: Input data was discarded because of an overflow condition at the driver.\n"));
        }
        if (runtimeData.lastErrorStatus == RTAUDIO_OUTPUT_UNDERFLOW) {
            ui->logTextEdit->appendPlainText(QString("Device error reported: The output buffer ran low, likely causing a gap in the output sound.\n"));
        }
        {
            std::lock_guard<std::mutex> guard(m_runtimeDataMutex);
            runtimeData.lastErrorStatus = 0;
        }
    }
}

