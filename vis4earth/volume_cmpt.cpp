#include <vis4earth/volume_cmpt.h>

#include <ui_volume_cmpt.h>

VIS4Earth::VolumeComponent::VolumeComponent(bool keepCPUData, bool keepVolSmoothed, QWidget *parent)
    : keepCPUData(keepCPUData), keepVolSmoothed(keepVolSmoothed),
      QtOSGReflectableWidget(ui, parent) {
    {
        auto gridLayout = reinterpret_cast<QGridLayout *>(ui->groupBox_tf->layout());

        auto insertRow = reinterpret_cast<QGridLayout *>(ui->groupBox_tf->layout())->rowCount();
        auto insertSapn = reinterpret_cast<QGridLayout *>(ui->groupBox_tf->layout())->columnCount();
        for (int i = 0; i < 2; ++i) {
            tfEditors[i] = new TransferFunctionEditor();
            gridLayout->addWidget(tfEditors[i], insertRow, 0, insertSapn, insertSapn);
        }
    }

    connect(ui->pushButton_loadRAWVolume, &QPushButton::clicked, this,
            &VolumeComponent::loadRAWVolume);
    connect(ui->pushButton_loadTF, &QPushButton::clicked, this, &VolumeComponent::loadTF);

    auto changeVolID = [&](int idx) {
        tfEditors[idx]->raise();

        if (GetVolumeTimeNumber(idx) == 0)
            return;

        updateVoxelPerVolume();
    };
    connect(ui->comboBox_currVolID, QOverload<int>::of(&QComboBox::currentIndexChanged),
            changeVolID);
    changeVolID(ui->comboBox_currVolID->currentIndex());

    connect(ui->comboBox_smoothType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&](int) { smoothVolume(); });
    connect(ui->comboBox_smoothDim, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&](int) { smoothVolume(); });

    for (int i = 0; i < 2; ++i)
        connect(tfEditors[i], &TransferFunctionEditor::TransferFunctionChanged, this,
                &VolumeComponent::sampleTF);
    sampleTF();
}

void VIS4Earth::VolumeComponent::loadRAWVolume() {
    auto filePaths = QFileDialog::getOpenFileNames(this, tr("Open RAW Volume File"), "./",
                                                   tr("RAW Volume (*.bin;*.raw)"));
    if (filePaths.isEmpty())
        return;

    std::array<uint32_t, 3> voxPerVol = {ui->spinBox_voxPerVolX->value(),
                                         ui->spinBox_voxPerVolY->value(),
                                         ui->spinBox_voxPerVolZ->value()};

    auto volID = ui->comboBox_currVolID->currentIndex();
    auto &vols = multiTimeVaryingVols[volID];
    auto &volCPUs = multiTimeVaryingVolCPUs[volID];

    vols.clear();
    vols.reserve(filePaths.size());
    if (keepCPUData) {
        volCPUs.clear();
        volCPUs.reserve(filePaths.size());
    }
    for (const auto &filePath : filePaths) {
        auto volDat = RAWVolumeData::LoadFromFile(RAWVolumeData::FromFileParameters{
            voxPerVol, ESupportedVoxelType::UInt8, filePath.toStdString()});
        if (!volDat.ok) {
            QMessageBox::warning(this, tr("Error"), tr(volDat.result.errMsg.c_str()));
            continue;
        }

        vols.emplace_back(volDat.result.dat.ToOSGTexture());
        if (keepCPUData)
            volCPUs.emplace_back(volDat.result.dat);
    }

    smoothVolume();
    updateVoxelPerVolume();
}

void VIS4Earth::VolumeComponent::smoothVolume() {
    auto smooth = [&](uint32_t volID) {
        auto &vols = multiTimeVaryingVols[volID];
        auto &volCPUs = multiTimeVaryingVolCPUs[volID];
        auto &volSmootheds = multiTimeVaryingVolSmootheds[volID];
        auto &volCPUSmootheds = multiTimeVaryingVolCPUSmootheds[volID];

        if (keepVolSmoothed) {
            volSmootheds.clear();
            volSmootheds.reserve(vols.size());
        }
        if (keepCPUData && keepVolSmoothed) {
            volCPUSmootheds.clear();
            volCPUSmootheds.reserve(vols.size());
        }

        for (uint32_t timeID = 0; timeID < vols.size(); ++timeID) {
            if (keepVolSmoothed) {
                auto volDatSmoothed = volCPUs[timeID].GetSmoothed(
                    RAWVolumeData::SmoothParameters{static_cast<RAWVolumeData::ESmoothType>(
                                                        ui->comboBox_smoothType->currentIndex()),
                                                    static_cast<RAWVolumeData::ESmoothDimension>(
                                                        ui->comboBox_smoothDim->currentIndex())});
                volSmootheds.emplace_back(volDatSmoothed.ToOSGTexture());

                if (keepCPUData)
                    volCPUSmootheds.emplace_back(volDatSmoothed);
            }
        }
    };

    for (uint32_t vi = 0; vi < 2; ++vi) {
        auto tNum = GetVolumeTimeNumber(vi);
        if (tNum == 0)
            continue;

        smooth(vi);
    }

    emit VolumeChanged();
}

void VIS4Earth::VolumeComponent::loadTF() {
    auto filePath = QFileDialog::getOpenFileName(this, tr("Open TXT File"), "./",
                                                 tr("Transfer Function (*.txt)"));
    if (filePath.isEmpty())
        return;

    auto tfDat = TransferFunctionData::LoadFromFile(TransferFunctionData::FromFileParameters{
        TransferFunctionData::EFilterType::Linear, filePath.toStdString()});
    if (!tfDat.ok) {
        QMessageBox::warning(this, tr("Error"), tr(tfDat.result.errMsg.c_str()));
        return;
    }

    tfEditors[ui->comboBox_currVolID->currentIndex()]->SetTransferFunctionData(tfDat.result.dat);
}

void VIS4Earth::VolumeComponent::saveTF() { assert(false); }

void VIS4Earth::VolumeComponent::sampleTF() {
    auto sample = [&](uint32_t volID) {
        multiTFs[volID] = tfEditors[volID]->GetTransferFunctionData().ToOSGTexture();
        multiTFPreInts[volID] =
            tfEditors[volID]->GetTransferFunctionData().ToPreIntegratedOSGTexture();
    };

    for (uint32_t vi = 0; vi < 2; ++vi)
        sample(vi);

    emit TransferFunctionChanged();
}

void VIS4Earth::VolumeComponent::updateVoxelPerVolume() {
    int volIdx = ui->comboBox_currVolID->currentIndex();

    std::array<int, 3> voxPerVol;
    if (keepCPUData) {
        auto &vol = multiTimeVaryingVolCPUs[volIdx][0];
        voxPerVol[0] = vol.GetVoxelPerVolume()[0];
        voxPerVol[1] = vol.GetVoxelPerVolume()[1];
        voxPerVol[2] = vol.GetVoxelPerVolume()[2];
    } else {
        auto vol = multiTimeVaryingVols[volIdx][0];
        voxPerVol[0] = vol->getImage()->s();
        voxPerVol[1] = vol->getImage()->t();
        voxPerVol[2] = vol->getImage()->r();
    }
    ui->label_voxPerVolX->setText(QString("%0").arg(voxPerVol[0]));
    ui->label_voxPerVolY->setText(QString("%0").arg(voxPerVol[1]));
    ui->label_voxPerVolZ->setText(QString("%0").arg(voxPerVol[2]));
}
