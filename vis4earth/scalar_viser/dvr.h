#ifndef VIS4EARTH_SCALAR_VISER_DVR_H
#define VIS4EARTH_SCALAR_VISER_DVR_H

#include <QtCore/QTimer>

#include <osg/BindImageTexture>
#include <osg/CoordinateSystemnode>
#include <osg/Cullface>
#include <osg/Group>
#include <osg/ShapeDrawable>

#include <vis4earth/geographics_cmpt.h>
#include <vis4earth/osg_util.h>
#include <vis4earth/qt_osg_reflectable.h>
#include <vis4earth/volume_cmpt.h>

#include <random>

namespace Ui {
class DirectVolumeRenderer;
}

namespace VIS4Earth {

class DirectVolumeRenderer : public QtOSGReflectableWidget {
    Q_OBJECT

  public:
    DirectVolumeRenderer(QWidget *parent = nullptr);
    ~DirectVolumeRenderer() { delete ssaoNoise; }

    osg::ref_ptr<osg::Group> GetGroup() const { return grp; }

  protected:
    osg::ref_ptr<osg::Group> grp;
    osg::ref_ptr<osg::ShapeDrawable> sphere;
    osg::ref_ptr<osg::Program> program;
    osg::ref_ptr<osg::ShapeDrawable> ssaoSphere;
    osg::ref_ptr<osg::Program> ssaoProgram;

    osg::ref_ptr<osg::Uniform> eyePos;
    std::array<osg::ref_ptr<osg::Uniform>, 2> dSamplePoss;
    osg::ref_ptr<osg::Uniform> useMultiVols;
    osg::ref_ptr<osg::Texture2D> colorTex, normalTex, depthTex;

    Ui::DirectVolumeRenderer *ui;
    QTimer timer;
    GeographicsComponent geoCmpt;
    VolumeComponent volCmpt;
    float *ssaoNoise;

    void initOSGResource();
};

} // namespace VIS4Earth

#endif // !VIS4EARTH_SCALAR_VISER_DVR_H
