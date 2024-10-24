#ifndef VIS4EARTH_SCALAR_VISER_HEATMAP_H
#define VIS4EARTH_SCALAR_VISER_HEATMAP_H

#include <osg/CoordinateSystemnode>
#include <osg/Group>
#include <osg/ShapeDrawable>

#include <osg/DispatchCompute>
#include <vis4earth/geographics_cmpt.h>
#include <vis4earth/osg_util.h>
#include <vis4earth/qt_osg_reflectable.h>
#include <vis4earth/volume_cmpt.h>

namespace Ui {
class HeatmapRenderer;
}

namespace VIS4Earth {

class HeatmapRenderer : public QtOSGReflectableWidget {
    friend class Heatmap2DDrawCallback;

    Q_OBJECT

  public:
    enum class ETextureFilterMode { Nearest, Linear };

    HeatmapRenderer(QWidget *parent = nullptr);

    osg::ref_ptr<osg::Group> GetGroup() const { return grp; }

  protected:
    osg::ref_ptr<osg::Group> grp;
    osg::ref_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::Program> program;
    osg::ref_ptr<osg::Vec3Array> verts;
    osg::ref_ptr<osg::Texture2D> volSliceTex;
    osg::ref_ptr<osg::Image> volSliceImg;

    osg::ref_ptr<osg::Program> program2D;
    osg::ref_ptr<osg::DispatchCompute> dispatchNode;
    osg::ref_ptr<osg::Texture2D> heatmapTex;

    osg::ref_ptr<osg::Uniform> volHeight;

    Ui::HeatmapRenderer *ui;
    GeographicsComponent geoCmpt;
    VolumeComponent volCmpt;

    void initOSGResource();
    void displayHeatmap2D();
    void updateGeometry();
    void updateHeatmap2D();

  signals:
    void Heatmap2DUpdated();
};

class Heatmap2DDrawCallback : public osg::Drawable::DrawCallback {
  public:
    Heatmap2DDrawCallback(HeatmapRenderer *renderer) : renderer(renderer) {}

    void drawImplementation(osg::RenderInfo &renderInfo, const osg::Drawable *drawable) const override;

  private:
    HeatmapRenderer *renderer;
};

} // namespace VIS4Earth

#endif
