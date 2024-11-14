#ifndef VIS4EARTH_VOLUME_CMPT_H
#define VIS4EARTH_VOLUME_CMPT_H

#include <vector>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>

#include <vis4earth/data/tf_data.h>
#include <vis4earth/data/vol_data.h>
#include <vis4earth/math.h>
#include <vis4earth/qt_osg_reflectable.h>
#include <vis4earth/tf_editor.h>

namespace Ui {
class VolumeComponent;
}

namespace VIS4Earth {

class VolumeComponent : public QtOSGReflectableWidget {
    Q_OBJECT

  public:
    VolumeComponent(bool keepCPUData = false, bool keepVolSmoothed = false,
                    QWidget *parent = nullptr);

    const Ui::VolumeComponent *GetUI() const { return ui; }

    ESupportedVoxelType GetVoxelType() const { return ESupportedVoxelType::UInt8; }

    uint32_t GetVolumeTimeNumber(uint32_t volID) const {
        if (volID > 1)
            return 0;
        return multiTimeVaryingVols[volID].size();
    }

    osg::ref_ptr<osg::Texture3D> GetVolume(uint32_t volID, uint32_t timeID) const {
        if (volID > 1 || timeID >= multiTimeVaryingVols[volID].size())
            return nullptr;
        return multiTimeVaryingVols[volID][timeID];
    }
    osg::ref_ptr<osg::Texture3D> GetVolumeSmoothed(uint32_t volID, uint32_t timeID) const {
        if (volID > 1 || timeID >= multiTimeVaryingVolSmootheds[volID].size())
            return nullptr;
        return multiTimeVaryingVolSmootheds[volID][timeID];
    }
    const RAWVolumeData &GetVolumeCPU(uint32_t volID, uint32_t timeID) const {
        return multiTimeVaryingVolCPUs[volID][timeID];
    }
    const RAWVolumeData &GetVolumeCPUSmoothed(uint32_t volID, uint32_t timeID) const {;
        return multiTimeVaryingVolCPUSmootheds[volID][timeID];
    }

    osg::ref_ptr<osg::Texture1D> GetTransferFunction(uint32_t volID) const {
        if (volID > 1)
            return nullptr;
        return multiTFs[volID];
    }
    const TransferFunctionData &GetTransferFunctionCPU(uint32_t volID) const {
        if (volID > 1)
            return {};
        return tfEditors[volID]->GetTransferFunctionData();
    }
    osg::ref_ptr<osg::Texture2D> GetPreIntegratedTransferFunction(uint32_t volID) const {
        if (volID > 1)
            return nullptr;
        return multiTFPreInts[volID];
    }

  Q_SIGNALS:
    void VolumeChanged();
    void TransferFunctionChanged();

  private:
    bool keepCPUData;
    bool keepVolSmoothed;

    Ui::VolumeComponent *ui;
    std::array<TransferFunctionEditor *, 2> tfEditors;
    std::array<osg::ref_ptr<osg::Texture1D>, 2> multiTFs;
    std::array<osg::ref_ptr<osg::Texture2D>, 2> multiTFPreInts;
    std::array<std::vector<osg::ref_ptr<osg::Texture3D>>, 2> multiTimeVaryingVols;
    std::array<std::vector<osg::ref_ptr<osg::Texture3D>>, 2> multiTimeVaryingVolSmootheds;
    std::array<std::vector<RAWVolumeData>, 2> multiTimeVaryingVolCPUs;
    std::array<std::vector<RAWVolumeData>, 2> multiTimeVaryingVolCPUSmootheds;

    void loadRAWVolume();

    void smoothVolume();

    void loadTF();

    void saveTF();

    void sampleTF();

    void updateVoxelPerVolume();
};

} // namespace VIS4Earth

#endif // !VIS4EARTH_VOLUME_CMPT_H
