#include <vis4earth/scalar_viser/isosurface.h>

#include <ui_isosurface.h>
#include <vis4earth/components_ui_export.h>

VIS4Earth::IsosurfaceRenderer::IsosurfaceRenderer(QWidget *parent)
    : volCmpt(true, true), QtOSGReflectableWidget(ui, parent) {
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&geoCmpt);
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&volCmpt);

    for (auto name : {"lightPosX", "lightPosY", "lightPosZ"}) {
        // 为光源位置进行坐标转换
        auto &prop = properties.at(name);
        prop->SetConvertor(
            [&, name = std::string(name)](Reflectable::Type val) -> Reflectable::Type {
                assert(val.type == Reflectable::ESupportedType::Float);

                float lon = ui->doubleSpinBox_lightPosX_float_VIS4EarthReflectable->value();
                float lat = ui->doubleSpinBox_lightPosY_float_VIS4EarthReflectable->value();
                float h = ui->doubleSpinBox_lightPosZ_float_VIS4EarthReflectable->value();
                auto xyz = Math::BLHToEarth(Math::DegToRad(lon), Math::DegToRad(lat),
                                            static_cast<float>(osg::WGS_84_RADIUS_POLAR) + h);

                if (std::strcmp(name.c_str(), "lightPosX") == 0)
                    return Reflectable::Type(xyz[0]);
                else if (std::strcmp(name.c_str(), "lightPosY") == 0)
                    return Reflectable::Type(xyz[1]);
                return Reflectable::Type(xyz[2]);
            });
    }

    initOSGResource();

    auto updateGeom = [&]() {
        vertSmootheds->clear();
        normSmootheds->clear();
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
        norms->clear();
        uvs->clear();
        multiEdges[0].clear();
        multiEdges[1].clear();
        for (int i = 0; i < 2; ++i) {
            if (volCmpt.GetVolumeTimeNumber(i) == 0)
                continue;
            marchingCube(i);
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

void VIS4Earth::IsosurfaceRenderer::initOSGResource() {
    grp = new osg::Group();
    geom = new osg::Geometry();
    geode = new osg::Geode();
    verts = new osg::Vec3Array();
    vertSmootheds = new osg::Vec3Array();
    norms = new osg::Vec3Array();
    normSmootheds = new osg::Vec3Array();
    uvs = new osg::Vec2Array();
    program = new osg::Program();

    auto stateSet = geode->getOrCreateStateSet();
    eyePos = new osg::Uniform("eyePos", osg::Vec3());
    stateSet->addUniform(eyePos);
    stateSet->addUniform(geoCmpt.GetRotateMatrix());
    for (auto obj : std::array<QtOSGReflectableWidget *, 3>{this, &geoCmpt, &volCmpt})
        obj->ForEachProperty([&](const std::string &name, const Property &prop) {
            stateSet->addUniform(prop.GetUniform());
        });
    {
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/isosurface_vert.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/isosurface_frag.glsl");
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

    geode->setCullCallback(new EyePositionUpdateCallback(eyePos));

    geode->addDrawable(geom);
    grp->addChild(geode);
}

void VIS4Earth::IsosurfaceRenderer::marchingCube(uint32_t volID) {
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
    std::array<std::unordered_map<std::array<int, 3>, GLuint, HashEdge>, 2> edge2vertIDs;

    osg::Vec3i startPos;
    for (startPos.z() = 0; startPos.z() < voxPerVol[2] - 1; ++startPos.z()) {
        if (startPos.z() != 0) {
            edge2vertIDs[0] = std::move(edge2vertIDs[1]);
            edge2vertIDs[1].clear(); // hash map only stores vertices of 2 consecutive heights
        }

        for (startPos.y() = 0; startPos.y() < voxPerVol[1] - 1; ++startPos.y())
            for (startPos.x() = 0; startPos.x() < voxPerVol[0] - 1; ++startPos.x()) {
                // Voxels in CCW order form a grid
                // +-----------------+
                // |       3 <--- 2  |
                // |       |     /|\ |
                // |      \|/     |  |
                // |       0 ---> 1  |
                // |      /          |
                // |  7 <--- 6       |
                // |  | /   /|\      |
                // | \|/_    |       |
                // |  4 ---> 5       |
                // +-----------------+
                uint8_t cornerState = 0;
                std::array<T, 8> scalars;
                for (int i = 0; i < 8; ++i) {
                    scalars[i] = sample(startPos);
                    if (scalars[i] >= isoval)
                        cornerState |= 1 << i;

                    startPos.x() += i == 0 || i == 4 ? 1 : i == 2 || i == 6 ? -1 : 0;
                    startPos.y() += i == 1 || i == 5 ? 1 : i == 3 || i == 7 ? -1 : 0;
                    startPos.z() += i == 3 ? 1 : i == 7 ? -1 : 0;
                }
                std::array<float, 12> omegas = {1.f * scalars[0] / (scalars[1] + scalars[0]),
                                                1.f * scalars[1] / (scalars[2] + scalars[1]),
                                                1.f * scalars[3] / (scalars[3] + scalars[2]),
                                                1.f * scalars[0] / (scalars[0] + scalars[3]),
                                                1.f * scalars[4] / (scalars[5] + scalars[4]),
                                                1.f * scalars[5] / (scalars[6] + scalars[5]),
                                                1.f * scalars[7] / (scalars[7] + scalars[6]),
                                                1.f * scalars[4] / (scalars[4] + scalars[7]),
                                                1.f * scalars[0] / (scalars[0] + scalars[4]),
                                                1.f * scalars[1] / (scalars[1] + scalars[5]),
                                                1.f * scalars[2] / (scalars[2] + scalars[6]),
                                                1.f * scalars[3] / (scalars[3] + scalars[7])};

                // Edge indexed by Start Voxel Position
                // +----------+
                // | /*\  *|  |
                // |  |  /    |
                // | e1 e2    |
                // |  * e0 *> |
                // +----------+
                // *:   startPos
                // *>:  startPos + (1,0,0)
                // /*\: startPos + (0,1,0)
                // *|:  startPos + (0,0,1)
                // ID(e0) = (startPos.xy, 00)
                // ID(e1) = (startPos.xy, 01)
                // ID(e2) = (startPos.xy, 10)
                for (uint32_t i = 0; i < VertNumTable[cornerState]; i += 3) {
                    for (int32_t ii = 0; ii < 3; ++ii) {
                        auto ei = TriangleTable[cornerState][i + ii];
                        std::array<int, 3> edgeID = {
                            startPos.x() + (ei == 1 || ei == 5 || ei == 9 || ei == 10 ? 1 : 0),
                            startPos.y() + (ei == 2 || ei == 6 || ei == 10 || ei == 11 ? 1 : 0),
                            ei >= 8                                    ? 2
                            : ei == 1 || ei == 3 || ei == 5 || ei == 7 ? 1
                                                                       : 0};
                        auto edge2vertIDIdx = ei >= 4 && ei < 8 ? 1 : 0;
                        auto itr = edge2vertIDs[edge2vertIDIdx].find(edgeID);
                        if (itr != edge2vertIDs[edge2vertIDIdx].end()) {
                            vertIndices.emplace_back(itr->second);
                            continue;
                        }

                        osg::Vec3 pos(
                            startPos.x() + (ei == 0 || ei == 2 || ei == 4 || ei == 6    ? omegas[ei]
                                            : ei == 1 || ei == 5 || ei == 9 || ei == 10 ? 1.f
                                                                                        : 0.f),
                            startPos.y() + (ei == 1 || ei == 3 || ei == 5 || ei == 7 ? omegas[ei]
                                            : ei == 2 || ei == 6 || ei == 10 || ei == 11 ? 1.f
                                                                                         : 0.f),
                            startPos.z() + (ei >= 8   ? omegas[ei]
                                            : ei >= 4 ? 1.f
                                                      : 0.f));
                        for (uint8_t i = 0; i < 3; ++i)
                            pos[i] /= voxPerVol[i];

                        float scalar;
                        switch (ei) {
                        case 0:
                            scalar = omegas[0] * scalars[0] + (1.f - omegas[0]) * scalars[1];
                            break;
                        case 1:
                            scalar = omegas[1] * scalars[1] + (1.f - omegas[1]) * scalars[2];
                            break;
                        case 2:
                            scalar = omegas[2] * scalars[3] + (1.f - omegas[2]) * scalars[2];
                            break;
                        case 3:
                            scalar = omegas[3] * scalars[0] + (1.f - omegas[3]) * scalars[3];
                            break;
                        case 4:
                            scalar = omegas[4] * scalars[4] + (1.f - omegas[4]) * scalars[5];
                            break;
                        case 5:
                            scalar = omegas[5] * scalars[5] + (1.f - omegas[5]) * scalars[6];
                            break;
                        case 6:
                            scalar = omegas[6] * scalars[7] + (1.f - omegas[6]) * scalars[6];
                            break;
                        case 7:
                            scalar = omegas[7] * scalars[4] + (1.f - omegas[7]) * scalars[7];
                            break;
                        default:
                            scalar =
                                omegas[ei] * scalars[ei - 8] + (1.f - omegas[ei]) * scalars[ei - 4];
                        }

                        vertIndices.emplace_back(verts->size());
                        verts->push_back(pos);
                        norms->push_back(osg::Vec3(0.f, 0.f, 0.f));
                        uvs->push_back(osg::Vec2(volID, scalar / 255.f));
                        edge2vertIDs[edge2vertIDIdx].emplace(edgeID, vertIndices.back());
                    }

                    std::array<GLuint, 3> triVertIdxs = {vertIndices[vertIndices.size() - 3],
                                                         vertIndices[vertIndices.size() - 2],
                                                         vertIndices[vertIndices.size() - 1]};
                    osg::Vec3 norm;
                    {
                        auto e0 = (*verts)[triVertIdxs[1]] - (*verts)[triVertIdxs[0]];
                        auto e1 = (*verts)[triVertIdxs[2]] - (*verts)[triVertIdxs[0]];
                        norm = e1 ^ e0;
                        norm.normalize();
                    }

                    (*norms)[triVertIdxs[0]] += norm;
                    (*norms)[triVertIdxs[1]] += norm;
                    (*norms)[triVertIdxs[2]] += norm;

                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[0], triVertIdxs[1]});
                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[1], triVertIdxs[0]});
                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[1], triVertIdxs[2]});
                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[2], triVertIdxs[1]});
                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[2], triVertIdxs[0]});
                    multiEdges[volID].emplace(
                        std::array<GLuint, 2>{triVertIdxs[0], triVertIdxs[2]});
                }
            }
    }

    for (auto &norm : *norms)
        norm.normalize();
}

void VIS4Earth::IsosurfaceRenderer::updateGeometry(uint32_t volID) {
    if (vertIndices.empty())
        return;

    auto laplacianSmooth = [&]() {
        for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
            auto itr = multiEdges[volID].lower_bound(std::array<GLuint, 2>{vIdx, 0});
            assert(itr != multiEdges[volID].end() && (*itr)[0] == vIdx);

            auto vertSmoothed = (*verts)[vIdx];
            auto normSmoothed = (*norms)[vIdx];
            int lnkNum = 1;
            while (itr != multiEdges[volID].end() && (*itr)[0] == vIdx) {
                vertSmoothed += (*verts)[(*itr)[1]];
                normSmoothed += (*norms)[(*itr)[1]];
                ++itr;
                ++lnkNum;
            }
            vertSmoothed /= lnkNum;
            normSmoothed.normalize();

            vertSmootheds->push_back(vertSmoothed);
            normSmootheds->push_back(normSmoothed);
        }
    };
    auto curvatureSmooth = [&]() {
        for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
            auto itr = multiEdges[volID].lower_bound(std::array<GLuint, 2>{vIdx, 0});
            assert(itr != multiEdges[volID].end() && (*itr)[0] == vIdx);

            auto vertSmoothed = (*verts)[vIdx];
            auto norm = (*norms)[vIdx];
            auto projLen = 0.f;
            while (itr != multiEdges[volID].end() && (*itr)[0] == vIdx) {
                auto dlt = (*verts)[(*itr)[1]] - vertSmoothed;
                projLen = dlt * norm;
                ++itr;
            }
            vertSmoothed = vertSmoothed + norm * projLen;

            vertSmootheds->push_back(vertSmoothed);
        }

        normSmootheds->insert(normSmootheds->end(), norms->begin(), norms->end());
    };

    switch (meshSmoothType) {
    case EMeshSmoothType::Laplacian:
        laplacianSmooth();
        break;
    case EMeshSmoothType::Curvature:
        curvatureSmooth();
        break;
    }

    switch (meshSmoothType) {
    case EMeshSmoothType::Laplacian:
    case EMeshSmoothType::Curvature:
        geom->setVertexArray(vertSmootheds);
        geom->setNormalArray(normSmootheds);
        break;
    default:
        geom->setVertexArray(verts);
        geom->setNormalArray(norms);
    }
    geom->setTexCoordArray(0, uvs);

    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

    geom->setInitialBound([]() -> osg::BoundingBox {
        osg::Vec3 max(osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR);
        return osg::BoundingBox(-max, max);
    }()); // 必须，否则不显示
    geom->getPrimitiveSetList().clear();
    geom->addPrimitiveSet(
        new osg::DrawElementsUInt(GL_TRIANGLES, vertIndices.size(), vertIndices.data()));
}
