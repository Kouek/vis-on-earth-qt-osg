#ifndef VIS4EARTH_CALLBACK_H
#define VIS4EARTH_CALLBACK_H

#include <osg/Camera>
#include <osg/Uniform>

namespace VIS4Earth {

    class MCallback : public osg::NodeCallback {
    private:
        osg::ref_ptr<osg::Uniform> M;

    public:
        MCallback(osg::ref_ptr<osg::Uniform> M) : M(M) {}
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
            auto mats = node->getWorldMatrices();
            osg::Matrixd l2w = mats[0];
            for (uint32_t i = 1; i < mats.size(); ++i)
                l2w *= mats[i];
            M->set(osg::Matrixf(l2w));

            traverse(node, nv);
        }
    };

    class VPCallback : public osg::UniformCallback {
    private:
        osg::ref_ptr<osg::Camera> camera;

    public:
        VPCallback(osg::ref_ptr<osg::Camera> camera) : camera(camera) {}
        virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {
            uniform->set(osg::Matrixf(camera->getProjectionMatrix() * camera->getViewMatrix()));
        }
    };

    template <typename Ty> class UniformUpdateCallback : public osg::UniformCallback {
    private:
        Ty dat;

    public:
        UniformUpdateCallback(const Ty& dat) : dat(dat) {}
        virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) { uniform->set(dat); }
    };

} // namespace VIS4Earth

#endif // !VIS4EARTH_CALLBACK_H
