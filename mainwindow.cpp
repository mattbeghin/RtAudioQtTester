#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include "devicedialog.h"
#include "definitions.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QStandardItemModel * model = new QStandardItemModel(ui->deviceListView);

    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    for (auto api: apis) {
        auto rtAudio = new RtAudio(api);
        m_rtAudioInterfaces.push_back(rtAudio);

        auto deviceCount = rtAudio->getDeviceCount();
        for (unsigned int i=0; i<deviceCount; i++) {
            auto deviceInfo = rtAudio->getDeviceInfo(i);
            auto displayName = deviceInfo.name + " (" + rtAudio->getApiDisplayName(api) + ")";
            auto item = new QStandardItem(QString::fromUtf8(displayName.c_str()));
            item->setData(QVariant::fromValue(deviceInfo),DeviceInfoRole);
            item->setData(api,ApiRole);
            model->appendRow(item);
        }
    }

    ui->deviceListView->setModel(model);
    connect(ui->deviceListView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(onSelectionChanged()));
    ui->deviceListView->setCurrentIndex(model->index(0,0));

    ui->openDeviceButton->setEnabled(ui->deviceListView->currentIndex().isValid());
    connect(ui->openDeviceButton,&QPushButton::clicked,[=](){
        auto selectedIndex = ui->deviceListView->currentIndex();
        auto deviceInfo = selectedIndex.data(DeviceInfoRole).value<RtAudio::DeviceInfo>();
        if (deviceInfo.inputChannels==0 && deviceInfo.outputChannels==0) {
            QMessageBox::information(nullptr,qAppName(),"Error: This audio device is listed but has no input or output channels.",QMessageBox::Ok);
            return;
        }
        auto api = selectedIndex.data(ApiRole).value<RtAudio::Api>();
        auto deviceDialog = new DeviceDialog(api,deviceInfo,this);
        deviceDialog->setAttribute(Qt::WA_DeleteOnClose);
        deviceDialog->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    qDeleteAll(m_rtAudioInterfaces);
    m_rtAudioInterfaces.clear();
}

QString makeNativeFormatString(RtAudioFormat format) {
    QString res;
    for (auto it: s_rtAudioFormatStr) {
        if (format & it.first) {
            res += "  " + QString::fromUtf8(it.second.c_str()) + "\n";
        }
    }
    return res;
}

QString makeSampleRatesString(const std::vector<unsigned int>& sampleRates) {
    QString res;
    for (auto& sampleRate: sampleRates) {
        res += QString("%1 / ").arg(sampleRate);
    }
    res = res.left(res.size()-3);
    return res;
}

void MainWindow::onSelectionChanged()
{
    if (!ui->deviceListView->currentIndex().isValid()) {
        ui->audioDeviceInfoLabel->setText("-");
    } else {
        auto selectedIndex = ui->deviceListView->currentIndex();
        auto deviceInfo = selectedIndex.data(DeviceInfoRole).value<RtAudio::DeviceInfo>();
        auto api = selectedIndex.data(ApiRole).value<RtAudio::Api>();
        ui->audioDeviceInfoLabel->setText(
            QString("%1\n").arg(QString::fromUtf8(deviceInfo.name.c_str())) +
            QString("Audio API: %1\n").arg(QString::fromUtf8(RtAudio::getApiDisplayName(api).c_str())) +
            QString("Input Channels: %1\n").arg(deviceInfo.inputChannels) +
            QString("Output Channels: %1\n").arg(deviceInfo.outputChannels) +
            QString("Duplex Channels: %1\n").arg(deviceInfo.duplexChannels) +
            QString("Default Input: %1\n").arg(deviceInfo.isDefaultInput?"True":"False") +
            QString("Default Output: %1\n").arg(deviceInfo.isDefaultOutput?"True":"False") +
            QString("Native Formats:\n%1").arg(makeNativeFormatString(deviceInfo.nativeFormats)) +
            QString("Sample Rates: %1\n").arg(makeSampleRatesString(deviceInfo.sampleRates)) +
            QString("Preferred Sample Rate: %1").arg(deviceInfo.preferredSampleRate)
        );
    }
}
