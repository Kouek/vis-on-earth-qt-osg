#ifndef VIS4EARTH_SCALAR_VISER_HEATMAP_H
#define VIS4EARTH_SCALAR_VISER_HEATMAP_H

#include <osg/CoordinateSystemnode>
#include <osg/Group>
#include <osg/ShapeDrawable>

#include <vis4earth/geographics_cmpt.h>
#include <vis4earth/osg_util.h>
#include <vis4earth/qt_osg_reflectable.h>
#include <vis4earth/volume_cmpt.h>

namespace Ui {
class HeatmapRenderer;
}

namespace VIS4Earth {

class HeatmapRenderer : public QtOSGReflectableWidget {
    Q_OBJECT

  public:
    enum class ETextureFilterMode { Nearest, Linear };
    enum class EHeightMapMode { None, Surface, Pillar };

    HeatmapRenderer(QWidget *parent = nullptr);

    osg::ref_ptr<osg::Group> GetGroup() const { return grp; }

  protected:
    osg::ref_ptr<osg::Group> grp;
    osg::ref_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::Program> program;
    osg::ref_ptr<osg::Vec3Array> verts;
    osg::ref_ptr<osg::Vec2Array> uvs;
    osg::ref_ptr<osg::Texture2D> volSliceTex;
    osg::ref_ptr<osg::Image> volSliceImg;

    osg::ref_ptr<osg::Uniform> volHeight;
    osg::ref_ptr<osg::Uniform> heightMapMode;

    Ui::HeatmapRenderer *ui;
    GeographicsComponent geoCmpt;
    VolumeComponent volCmpt;
    QImage heatmap2D;

    void initOSGResource();
    void updateHeatmap2D();
    void updateGeometry();
};

} // namespace VIS4Earth

#endif
