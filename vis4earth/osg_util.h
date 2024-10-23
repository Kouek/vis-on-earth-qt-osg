#ifndef VIS4EARTH_OSG_H
#define VIS4EARTH_OSG_H

#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>

namespace VIS4Earth {

inline osg::Node *CreateEarth() {
    auto *hints = new osg::TessellationHints;
    hints->setDetailRatio(5.0f);

    auto *sd = new osg::ShapeDrawable(
        new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), osg::WGS_84_RADIUS_POLAR), hints);
    sd->setUseVertexBufferObjects(true);

    auto *geode = new osg::Geode;
    geode->addDrawable(sd);

    auto filename = osgDB::findDataFile(DATA_PATH_PREFIX "land_shallow_topo_2048.jpg");
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, new osg::Texture2D(osgDB::readImageFile(filename)));

    return geode;
}

template <typename Ty> class UniformUpdateCallback : public osg::UniformCallback {
  private:
    Ty dat;

  public:
    UniformUpdateCallback(const Ty &dat) : dat(dat) {}
    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) { uniform->set(dat); }
};

class MVPCallback : public osg::Uniform::Callback {
  public:
    MVPCallback(osg::ref_ptr<osg::Camera> camera) : camera(camera) {}

    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        osg::Matrixf V = camera->getViewMatrix();
        osg::Matrixf P = camera->getProjectionMatrix();
        uniform->set(V * P);
    }

  private:
    osg::ref_ptr<osg::Camera> camera;
};

class EyePositionUpdateCallback : public osg::NodeCallback {
  private:
    osg::ref_ptr<osg::Uniform> eyePosUni;

  public:
    EyePositionUpdateCallback(osg::ref_ptr<osg::Uniform> eyePosUni) : eyePosUni(eyePosUni) {}
    virtual void operator()(osg::Node *node, osg::NodeVisitor *nv) {
        eyePosUni->set(nv->getEyePoint());

        traverse(node, nv);
    }
};

} // namespace VIS4Earth

#endif // !VIS4EARTH_OSG_H
