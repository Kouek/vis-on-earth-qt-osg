#include <vis4earth/scalar_viser/isopleth.h>

#include <ui_isopleth.h>
#include <vis4earth/components_ui_export.h>

VIS4Earth::IsoplethRenderer::IsoplethRenderer(QWidget *parent)
    : volCmpt(true, true), QtOSGReflectableWidget(ui, parent) {
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&geoCmpt);
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&volCmpt);

    initOSGResource();

    auto updateGeom = [&]() {
        vertSmootheds->clear();
        for (int i = 0; i < 2; ++i)
            if (volCmpt.GetVolumeTimeNumber(i) != 0)
                updateGeometry(i);
    };
    auto genIsosurface = [&, updateGeom]() {
        isoval = ui->horizontalSlider_isoval->value();
        useVolSmoothed = ui->checkBox_useVolSmoothed->isChecked();
        meshSmoothType = static_cast<EMeshSmoothType>(ui->comboBox_meshSmoothType->currentIndex());

        vertIndices.clear();
        verts->clear();
        uvs->clear();
        multiEdges[0].clear();
        multiEdges[1].clear();
        for (int i = 0; i < 2; ++i) {
            if (volCmpt.GetVolumeTimeNumber(i) == 0)
                continue;
            marchingSquare(i);
        }

        updateGeom();
    };
    connect(ui->horizontalSlider_isoval, &QSlider::sliderMoved,
            [&](int val) { ui->label_isoval->setText(QString::number(val)); });
    connect(ui->horizontalSlider_isoval, &QSlider::valueChanged, genIsosurface);
    connect(ui->checkBox_useVolSmoothed, &QCheckBox::stateChanged, genIsosurface);
    connect(&volCmpt, &VolumeComponent::VolumeChanged, genIsosurface);
    connect(ui->comboBox_meshSmoothType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&, updateGeom](int idx) {
                meshSmoothType = static_cast<EMeshSmoothType>(idx);
                updateGeom();
            });

    auto changeTF = [&]() {
        auto stateSet = geode->getOrCreateStateSet();
        stateSet->setTextureAttributeAndModes(0, volCmpt.GetTransferFunction(0),
                                              osg::StateAttribute::ON);
        stateSet->setTextureAttributeAndModes(1, volCmpt.GetTransferFunction(1),
                                              osg::StateAttribute::ON);
    };
    connect(&volCmpt, &VolumeComponent::TransferFunctionChanged, changeTF);
    changeTF();

    debugProperties({this, &volCmpt, &geoCmpt});
}

void VIS4Earth::IsoplethRenderer::initOSGResource() {
    grp = new osg::Group();
    geom = new osg::Geometry();
    geode = new osg::Geode();
    verts = new osg::Vec3Array();
    vertSmootheds = new osg::Vec3Array();
    uvs = new osg::Vec2Array();
    program = new osg::Program();

    auto stateSet = geode->getOrCreateStateSet();
    for (auto obj : std::array<QtOSGReflectableWidget *, 3>{this, &geoCmpt, &volCmpt})
        obj->ForEachProperty([&](const std::string &name, const Property &prop) {
            stateSet->addUniform(prop.GetUniform());
        });
    {
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/isopleth_vert.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/isopleth_frag.glsl");
        program->addShader(vertShader);
        program->addShader(fragShader);
    }
    {
        auto tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex0");
        tfTexUni->set(0);
        stateSet->addUniform(tfTexUni);
        tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex1");
        tfTexUni->set(1);
        stateSet->addUniform(tfTexUni);
    }
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);

    geode->addDrawable(geom);
    grp->addChild(geode);
}

void VIS4Earth::IsoplethRenderer::marchingSquare(uint32_t volID) {
    using T = uint8_t;
    std::array<uint32_t, 3> voxPerVol = {volCmpt.GetUI()->label_voxPerVolX->text().toInt(),
                                         volCmpt.GetUI()->label_voxPerVolY->text().toInt(),
                                         volCmpt.GetUI()->label_voxPerVolZ->text().toInt()};
    auto voxPerVolYxX = static_cast<size_t>(voxPerVol[1]) * voxPerVol[0];

    auto sample = [&](const osg::Vec3i &pos) -> T {
        if (useVolSmoothed)
            return volCmpt.GetVolumeCPUSmoothed(volID, 0).Sample<T>(pos.x(), pos.y(), pos.z());
        return volCmpt.GetVolumeCPU(volID, 0).Sample<T>(pos.x(), pos.y(), pos.z());
    };

    struct HashEdge {
        size_t operator()(const std::array<int, 3> &edgeID) const {
            size_t hash = edgeID[0];
            hash = (hash << 32) | edgeID[1];
            hash = (hash << 2) | edgeID[2];
            return std::hash<size_t>()(hash);
        };
    };
    std::unordered_map<GLuint, GLuint> edge2vertIDs;

    auto addLineSeg = [&](const osg::Vec3i &startPos, const std::array<T, 4> &scalars,
                          const osg::Vec4f &omegas, uint8_t mask) {
        for (uint8_t i = 0; i < 4; ++i) {
            if (((mask >> i) & 0b1) == 0)
                continue;

            // Edge indexed by Start Voxel Position
            // +----------+
            // | /*\      |
            // |  e1      |
            // |  * e0 *> |
            // +----------+
            // *:   startPos
            // *>:  startPos + (1,0)
            // /*\: startPos + (0,1)
            // ID(e0): [ ID(*) : 63bit | 0 : 1bit ]
            // ID(e1): [ ID(*) : 63bit | 1 : 1bit ]
            auto edgeID = static_cast<GLuint>(startPos.y() + (i == 2 ? 1 : 0)) * voxPerVol[0] +
                          startPos.x() + (i == 1 ? 1 : 0);
            edgeID = (edgeID << 1) + (i == 1 || i == 3 ? 1 : 0);
            auto itr = edge2vertIDs.find(edgeID);
            if (itr != edge2vertIDs.end()) {
                vertIndices.push_back(itr->second);
                continue;
            }

            osg::Vec3 pos(startPos.x() + (i == 0 || i == 2 ? omegas[i]
                                          : i == 1         ? 1.f
                                                           : 0.f),
                          startPos.y() + (i == 1 || i == 3 ? omegas[i]
                                          : i == 2         ? 1.f
                                                           : 0.f),
                          startPos.z());
            for (uint8_t i = 0; i < 3; ++i)
                pos[i] /= voxPerVol[i];

            float scalar;
            switch (i) {
            case 0:
                scalar = omegas[0] * scalars[0] + (1.f - omegas[0]) * scalars[1];
                break;
            case 1:
                scalar = omegas[1] * scalars[1] + (1.f - omegas[1]) * scalars[2];
                break;
            case 2:
                scalar = omegas[2] * scalars[3] + (1.f - omegas[2]) * scalars[2];
                break;
            default:
                scalar = omegas[3] * scalars[0] + (1.f - omegas[3]) * scalars[3];
                break;
            }

            vertIndices.push_back(verts->size());
            verts->push_back(pos);
            uvs->push_back(osg::Vec2(volID, scalar / 255.f));
            edge2vertIDs.emplace(edgeID, vertIndices.back());
        }

        multiEdges[volID].emplace(std::array<GLuint, 2>{vertIndices[vertIndices.size() - 1],
                                                        vertIndices[vertIndices.size() - 2]});
        multiEdges[volID].emplace(std::array<GLuint, 2>{vertIndices[vertIndices.size() - 2],
                                                        vertIndices[vertIndices.size() - 1]});
    };

    osg::Vec3i startPos;
    for (startPos.z() = 0; startPos.z() < voxPerVol[2]; ++startPos.z()) {
        edge2vertIDs.clear(); // hash map only stores vertices of the same height

        for (startPos.y() = 0; startPos.y() < voxPerVol[1] - 1; ++startPos.y())
            for (startPos.x() = 0; startPos.x() < voxPerVol[0] - 1; ++startPos.x()) {
                // Voxels in CCW order form a grid
                // +------------+
                // |  3 <--- 2  |
                // |  |     /|\ |
                // | \|/     |  |
                // |  0 ---> 1  |
                // +------------+
                uint8_t cornerState = 0;
                std::array<T, 4> scalars = {
                    sample(startPos), sample(startPos + osg::Vec3i(1, 0, 0)),
                    sample(startPos + osg::Vec3i(1, 1, 0)), sample(startPos + osg::Vec3i(0, 1, 0))};
                for (uint8_t i = 0; i < 4; ++i)
                    if (scalars[i] >= isoval)
                        cornerState |= 1 << i;

                osg::Vec4 omegas(1.f * scalars[0] / (scalars[1] + scalars[0]),
                                 1.f * scalars[1] / (scalars[2] + scalars[1]),
                                 1.f * scalars[3] / (scalars[3] + scalars[2]),
                                 1.f * scalars[0] / (scalars[0] + scalars[3]));

                switch (cornerState) {
                case 0b0001:
                case 0b1110:
                    addLineSeg(startPos, scalars, omegas, 0b1001);
                    break;
                case 0b0010:
                case 0b1101:
                    addLineSeg(startPos, scalars, omegas, 0b0011);
                    break;
                case 0b0011:
                case 0b1100:
                    addLineSeg(startPos, scalars, omegas, 0b1010);
                    break;
                case 0b0100:
                case 0b1011:
                    addLineSeg(startPos, scalars, omegas, 0b0110);
                    break;
                case 0b0101:
                    addLineSeg(startPos, scalars, omegas, 0b0011);
                    addLineSeg(startPos, scalars, omegas, 0b1100);
                    break;
                case 0b1010:
                    addLineSeg(startPos, scalars, omegas, 0b0110);
                    addLineSeg(startPos, scalars, omegas, 0b1001);
                    break;
                case 0b0110:
                case 0b1001:
                    addLineSeg(startPos, scalars, omegas, 0b0101);
                    break;
                case 0b0111:
                case 0b1000:
                    addLineSeg(startPos, scalars, omegas, 0b1100);
                    break;
                }
            }
    }
}

void VIS4Earth::IsoplethRenderer::updateGeometry(uint32_t volID) {
    if (vertIndices.empty())
        return;

    auto sphericalSplineSmooth = [&]() {
        for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
            auto p0 = (*verts)[vIdx];
            auto itr = multiEdges[volID].lower_bound(std::array<GLuint, 2>{vIdx, 0});
            assert(itr != multiEdges[volID].end() && (*itr)[0] == vIdx);
            auto vi1 = (*itr)[1];
            auto p1 = (*verts)[vi1];

            ++itr;
            if (itr == multiEdges[volID].end() || (*itr)[0] != vIdx) {
                vertSmootheds->push_back(p0);
                continue;
            }
            auto vi2 = (*itr)[1];
            auto p2 = (*verts)[vi2];

            auto A = p0.x() * (p1.y() - p2.y()) - p0.y() * (p1.x() - p2.x()) + p1.x() * p2.y() -
                     p2.x() * p1.y();
            if (A <= std::numeric_limits<float>::epsilon()) {
                vertSmootheds->push_back(p0);
                continue;
            }

            auto B = (std::pow(p0.x(), 2) + std::pow(p0.y(), 2)) * (p2.y() - p1.y()) +
                     (std::pow(p1.x(), 2) + std::pow(p1.y(), 2)) * (p0.y() - p2.y()) +
                     (std::pow(p2.x(), 2) + std::pow(p2.y(), 2)) * (p1.y() - p0.y());
            auto C = (std::pow(p0.x(), 2) + std::pow(p0.y(), 2)) * (p1.x() - p2.x()) +
                     (std::pow(p1.x(), 2) + std::pow(p1.y(), 2)) * (p2.x() - p0.x()) +
                     (std::pow(p2.x(), 2) + std::pow(p2.y(), 2)) * (p0.x() - p1.x());
            osg::Vec3 c(-.5f * B / A, -.5f * C / A, p0.z());

            auto e1 = p1 - c;
            auto e2 = p2 - c;
            auto r = e1.length();
            e1 = e1 + e2;
            e1.normalize();

            vertSmootheds->push_back(c + e1 * r);
        }
    };
    auto squareBezierSmooth = [&]() {
        for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
            auto p0 = (*verts)[vIdx];
            auto itr = multiEdges[volID].lower_bound(std::array<GLuint, 2>{vIdx, 0});
            assert(itr != multiEdges[volID].end() && (*itr)[0] == vIdx);
            auto vi1 = (*itr)[1];
            auto p1 = (*verts)[vi1];

            ++itr;
            if (itr == multiEdges[volID].end() || (*itr)[0] != vIdx) {
                vertSmootheds->push_back(p0);
                continue;
            }
            auto vi2 = (*itr)[1];
            auto p2 = (*verts)[vi2];

            vertSmootheds->push_back(p1 * .25f + p0 * .5f + p2 * .25f);
        }
    };

    vertSmootheds->clear();
    switch (meshSmoothType) {
    case EMeshSmoothType::SphericalSpline:
        sphericalSplineSmooth();
        break;
    case EMeshSmoothType::SqaureBezier:
        squareBezierSmooth();
        break;
    }

    switch (meshSmoothType) {
    case EMeshSmoothType::None:
        geom->setVertexArray(verts);
        break;
    case EMeshSmoothType::SphericalSpline:
    case EMeshSmoothType::SqaureBezier:
        geom->setVertexArray(vertSmootheds);
        break;
    }
    geom->setTexCoordArray(0, uvs);

    geom->setInitialBound([]() -> osg::BoundingBox {
        osg::Vec3 max(osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR);
        return osg::BoundingBox(-max, max);
    }()); // 必须，否则不显示
    geom->getPrimitiveSetList().clear();
    geom->addPrimitiveSet(
        new osg::DrawElementsUInt(GL_LINES, vertIndices.size(), vertIndices.data()));
}
