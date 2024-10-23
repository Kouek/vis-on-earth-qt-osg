#ifndef VIS4EARTH_SCALAR_VISER_DVR_H
#define VIS4EARTH_SCALAR_VISER_DVR_H

#include <QtCore/QTimer>

#include <osg/CoordinateSystemnode>
#include <osg/Cullface>
#include <osg/Group>
#include <osg/ShapeDrawable>

#include <vis4earth/geographics_cmpt.h>
#include <vis4earth/osg_util.h>
#include <vis4earth/qt_osg_reflectable.h>
#include <vis4earth/volume_cmpt.h>

namespace Ui {
class DirectVolumeRenderer;
}

namespace VIS4Earth {

class DirectVolumeRenderer : public QtOSGReflectableWidget {
    Q_OBJECT

  public:
    DirectVolumeRenderer(QWidget *parent = nullptr);

    osg::ref_ptr<osg::Group> GetGroup() const { return grp; }

  protected:
    osg::ref_ptr<osg::Group> grp;
#ifdef VIS4EARTH_USE_OLD_RENDERER
    osg::ref_ptr<osg::ShapeDrawable> sphere;
#else
    osg::ref_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::Vec3Array> verts;
    osg::ref_ptr<osg::Uniform> MVP;
#endif
    osg::ref_ptr<osg::Program> program;

    osg::ref_ptr<osg::Uniform> eyePos;
    std::array<osg::ref_ptr<osg::Uniform>, 2> dSamplePoss;
    osg::ref_ptr<osg::Uniform> useMultiVols;

    Ui::DirectVolumeRenderer *ui;
    QTimer timer;
    GeographicsComponent geoCmpt;
    VolumeComponent volCmpt;

    void initOSGResource();

#ifdef VIS4EARTH_USE_OLD_RENDERER
#else
  public:
    void SetCamera(osg::ref_ptr<osg::Camera> camera);

  private:
    void updateGeometry();
#endif
};

} // namespace VIS4Earth

#endif // !VIS4EARTH_SCALAR_VISER_DVR_H
