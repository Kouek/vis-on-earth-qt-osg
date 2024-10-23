#include <vis4earth/scalar_viser/dvr.h>

#include <ui_dvr.h>
#include <vis4earth/components_ui_export.h>

VIS4Earth::DirectVolumeRenderer::DirectVolumeRenderer(QWidget *parent)
    : QtOSGReflectableWidget(ui, parent) {
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
    for (auto name : {"sliceDirX", "sliceDirY", "sliceDirZ"}) {
        // 为切片方向进行归一化
        auto &prop = properties.at(name);
        prop->SetConvertor(
            [&, name = std::string(name)](Reflectable::Type val) -> Reflectable::Type {
                assert(val.type == Reflectable::ESupportedType::Float);

                osg::Vec3 dir(ui->doubleSpinBox_sliceDirX_float_VIS4EarthReflectable->value(),
                              ui->doubleSpinBox_sliceDirY_float_VIS4EarthReflectable->value(),
                              ui->doubleSpinBox_sliceDirZ_float_VIS4EarthReflectable->value());
                dir.normalize();

                ui->doubleSpinBox_sliceDirX_float_VIS4EarthReflectable->setValue(dir.x());
                ui->doubleSpinBox_sliceDirY_float_VIS4EarthReflectable->setValue(dir.y());
                ui->doubleSpinBox_sliceDirZ_float_VIS4EarthReflectable->setValue(dir.z());

                if (std::strcmp(name.c_str(), "sliceDirX") == 0)
                    return Reflectable::Type(dir.x());
                else if (std::strcmp(name.c_str(), "sliceDirY") == 0)
                    return Reflectable::Type(dir.y());
                return Reflectable::Type(dir.z());
            });
    }

    initOSGResource();

#ifdef VIS4EARTH_USE_OLD_RENDERER
    ui->spinBox_tessellationX->setEnabled(false);
    ui->spinBox_tessellationY->setEnabled(false);
#else
    connect(ui->spinBox_tessellationX, &QSpinBox::editingFinished, this,
            &DirectVolumeRenderer::updateGeometry);
    connect(ui->spinBox_tessellationY, &QSpinBox::editingFinished, this,
            &DirectVolumeRenderer::updateGeometry);
    updateGeometry();
#endif

#ifdef VIS4EARTH_USE_OLD_RENDERER
    auto stateSet = sphere->getOrCreateStateSet();

    auto onHeightChanged = [&](double) {
        sphere->setShape(new osg::Sphere(
            osg::Vec3(0.f, 0.f, 0.f),
            static_cast<float>(osg::WGS_84_RADIUS_POLAR) +
                geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable->value()));
    };
    connect(geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), onHeightChanged);
    onHeightChanged(0.);
#endif

    connect(&volCmpt, &VolumeComponent::VolumeChanged, [&]() {
        for (int i = 0; i < 2; ++i) {
            auto timeNum = volCmpt.GetVolumeTimeNumber(i);
            if (timeNum == 0)
                continue;

            std::array<int, 3> voxPerVol = {volCmpt.GetVolume(i, 0)->getImage()->s(),
                                            volCmpt.GetVolume(i, 0)->getImage()->t(),
                                            volCmpt.GetVolume(i, 0)->getImage()->r()};
            dSamplePoss[i]->set(
                osg::Vec3(1.f / voxPerVol[0], 1.f / voxPerVol[1], 1.f / voxPerVol[2]));
        }
    });

    auto changeTF = [&]() {
#ifdef VIS4EARTH_USE_OLD_RENDERER
        auto stateSet = sphere->getOrCreateStateSet();
#else
        auto stateSet = geode->getOrCreateStateSet();
#endif
        stateSet->setTextureAttributeAndModes(2, volCmpt.GetPreIntegratedTransferFunction(0),
                                              osg::StateAttribute::ON);
        stateSet->setTextureAttributeAndModes(3, volCmpt.GetPreIntegratedTransferFunction(1),
                                              osg::StateAttribute::ON);
        stateSet->setTextureAttributeAndModes(4, volCmpt.GetTransferFunction(0),
                                              osg::StateAttribute::ON);
        stateSet->setTextureAttributeAndModes(5, volCmpt.GetTransferFunction(1),
                                              osg::StateAttribute::ON);
    };
    connect(&volCmpt, &VolumeComponent::TransferFunctionChanged, changeTF);
    changeTF();

    auto changeVol = [&]() {
        static uint32_t currTimeID = 0;

#ifdef VIS4EARTH_USE_OLD_RENDERER
        auto stateSet = sphere->getOrCreateStateSet();
#else
        auto stateSet = geode->getOrCreateStateSet();
#endif
        int validVolNum = 0;
        for (int i = 0; i < 2; ++i) {
            auto timeNum = volCmpt.GetVolumeTimeNumber(i);
            if (timeNum == 0)
                continue;

            stateSet->setTextureAttributeAndModes(i, volCmpt.GetVolume(i, currTimeID % timeNum),
                                                  osg::StateAttribute::ON);
            ++validVolNum;
        }

        useMultiVols->set(validVolNum == 2);
        ++currTimeID;
    };
    connect(&timer, &QTimer::timeout, changeVol);
    timer.setInterval(330);
    timer.start();

    auto changeStep = [&]() {
        ui->doubleSpinBox_step_float_VIS4EarthReflectable->setMinimum(0.);

        auto heightDlt =
            geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable->value() -
            geoCmpt.GetUI()->doubleSpinBox_heightMin_float_VIS4EarthReflectable->value();
        auto heightVol = volCmpt.GetVolume(0, 0) ? volCmpt.GetVolume(0, 0)->getImage()->r() : 0;
        if (heightDlt == 0. || heightVol == 0) {
            ui->doubleSpinBox_step_float_VIS4EarthReflectable->setValue(0.);
            return;
        }

        ui->doubleSpinBox_step_float_VIS4EarthReflectable->setMaximum(.5 * heightDlt);
        ui->doubleSpinBox_step_float_VIS4EarthReflectable->setValue(.4 * heightDlt / heightVol);
    };
    connect(&volCmpt, &VolumeComponent::VolumeChanged, changeStep);
    connect(&geoCmpt, &GeographicsComponent::GeographicsChanged, changeStep);
    changeStep();

    debugProperties({this, &volCmpt, &geoCmpt});
}

void VIS4Earth::DirectVolumeRenderer::initOSGResource() {
    grp = new osg::Group();
    program = new osg::Program;

#ifdef VIS4EARTH_USE_OLD_RENDERER
    sphere = new osg::ShapeDrawable();
    auto stateSet = sphere->getOrCreateStateSet();
#else
    geom = new osg::Geometry();
    geode = new osg::Geode();
    verts = new osg::Vec3Array();
    auto stateSet = geode->getOrCreateStateSet();

    MVP = new osg::Uniform("MVP", osg::Matrixf());
    stateSet->addUniform(MVP);
#endif

    eyePos = new osg::Uniform("eyePos", osg::Vec3());
    dSamplePoss[0] = new osg::Uniform("dSamplePos0", osg::Vec3(1.f, 1.f, 1.f));
    dSamplePoss[1] = new osg::Uniform("dSamplePos1", osg::Vec3(1.f, 1.f, 1.f));
    useMultiVols = new osg::Uniform("useMultiVols", false);
    stateSet->addUniform(eyePos);
    stateSet->addUniform(dSamplePoss[0]);
    stateSet->addUniform(dSamplePoss[1]);
    stateSet->addUniform(useMultiVols);
    stateSet->addUniform(geoCmpt.GetRotateMatrix());
    for (auto obj : std::array<QtOSGReflectableWidget *, 3>{this, &geoCmpt, &volCmpt})
        obj->ForEachProperty([&](const std::string &name, const Property &prop) {
            stateSet->addUniform(prop.GetUniform());
        });
    {
#ifdef VIS4EARTH_USE_OLD_RENDERER
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_vert.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_frag.glsl");
#else
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_vert_new.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_frag_new.glsl");
#endif
        program->addShader(vertShader);
        program->addShader(fragShader);
    }
    {
        auto volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex0");
        volTexUni->set(0);
        stateSet->addUniform(volTexUni);
        volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex1");
        volTexUni->set(1);
        stateSet->addUniform(volTexUni);

        auto tfTexPreIntUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tfTexPreInt0");
        tfTexPreIntUni->set(2);
        stateSet->addUniform(tfTexPreIntUni);
        tfTexPreIntUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tfTexPreInt1");
        tfTexPreIntUni->set(3);
        stateSet->addUniform(tfTexPreIntUni);

        auto tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex0");
        tfTexUni->set(4);
        stateSet->addUniform(tfTexUni);
        tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex1");
        tfTexUni->set(5);
        stateSet->addUniform(tfTexUni);
    }

#ifdef VIS4EARTH_USE_OLD_RENDERER
    osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
#else
    osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::FRONT);
#endif
    stateSet->setAttributeAndModes(cf);

    stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

#ifdef VIS4EARTH_USE_OLD_RENDERER
    sphere->setCullCallback(new EyePositionUpdateCallback(eyePos));
    grp->addChild(sphere);
#else
    geode->setCullCallback(new EyePositionUpdateCallback(eyePos));
    geode->addChild(geom);
    grp->addChild(geode);
#endif
}

#ifdef VIS4EARTH_USE_OLD_RENDERER
#else
void VIS4Earth::DirectVolumeRenderer::SetCamera(osg::ref_ptr<osg::Camera> camera) {
    MVP->setUpdateCallback(new MVPCallback(camera));
}

void VIS4Earth::DirectVolumeRenderer::updateGeometry() {
    verts->clear();

    auto res = ui->spinBox_tessellationX->value() * ui->spinBox_tessellationY->value();
    verts->reserve(res);
    uint32_t btmSurfVertStart;
    auto genSurfVertices = [&](bool isTop) {
        for (int latIdx = 0; latIdx < ui->spinBox_tessellationY->value(); ++latIdx)
            for (int lonIdx = 0; lonIdx < ui->spinBox_tessellationX->value(); ++lonIdx) {
                osg::Vec3 pos(1.f * lonIdx / (ui->spinBox_tessellationX->value() - 1),
                              1.f * latIdx / (ui->spinBox_tessellationY->value() - 1),
                              isTop ? 1.f : 0.f);

                verts->push_back(pos);
            }
    };
    genSurfVertices(true);
    btmSurfVertStart = verts->size();
    genSurfVertices(false);

    std::vector<GLuint> vertIndices;
    auto addTri = [&](std::array<GLuint, 3> triIndices) {
        for (uint8_t i = 0; i < 3; ++i)
            vertIndices.emplace_back(triIndices[i]);
    };
    auto addTopBotSurf = [&](bool isTop, int latIdx, int lonIdx) {
        auto start = isTop ? 0 : btmSurfVertStart;
        std::array<GLuint, 4> quadIndices = {
            start + latIdx * ui->spinBox_tessellationX->value() + lonIdx,
            start + latIdx * ui->spinBox_tessellationX->value() + lonIdx + (isTop ? 1 : -1),
            start + (latIdx + 1) * ui->spinBox_tessellationX->value() + lonIdx + (isTop ? 1 : -1),
            start + (latIdx + 1) * ui->spinBox_tessellationX->value() + lonIdx};

        addTri({quadIndices[0], quadIndices[1], quadIndices[2]});
        addTri({quadIndices[0], quadIndices[2], quadIndices[3]});
    };
    for (int latIdx = 0; latIdx < ui->spinBox_tessellationY->value() - 1; ++latIdx)
        for (int lonIdx = 0; lonIdx < ui->spinBox_tessellationX->value() - 1; ++lonIdx) {
            addTopBotSurf(true, latIdx, lonIdx);
            addTopBotSurf(false, latIdx, ui->spinBox_tessellationX->value() - 1 - lonIdx);
        }

    auto addSideSurf = [&](int latIdx, int lonIdx, const osg::Vec2i &dir) {
        std::array<GLuint, 4> quadIndices = {
            btmSurfVertStart + latIdx * ui->spinBox_tessellationX->value() + lonIdx,
            btmSurfVertStart + (latIdx + dir.y()) * ui->spinBox_tessellationX->value() + lonIdx +
                dir.x(),
            (latIdx + dir.y()) * ui->spinBox_tessellationX->value() + lonIdx + dir.x(),
            latIdx * ui->spinBox_tessellationX->value() + lonIdx};
        addTri({quadIndices[0], quadIndices[1], quadIndices[2]});
        addTri({quadIndices[0], quadIndices[2], quadIndices[3]});
    };
    for (int lonIdx = 0; lonIdx < ui->spinBox_tessellationX->value() - 1; ++lonIdx) {
        addSideSurf(0, lonIdx, {1, 0});
        addSideSurf(ui->spinBox_tessellationY->value() - 1,
                    ui->spinBox_tessellationX->value() - 1 - lonIdx, {-1, 0});
    }
    for (int latIdx = 0; latIdx < ui->spinBox_tessellationY->value() - 1; ++latIdx) {
        addSideSurf(ui->spinBox_tessellationY->value() - 1 - latIdx, 0, {0, -1});
        addSideSurf(latIdx, ui->spinBox_tessellationX->value() - 1, {0, 1});
    }

    geom->setInitialBound([]() -> osg::BoundingBox {
        osg::Vec3 max(osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR);
        return osg::BoundingBox(-max, max);
    }()); // 必须，否则不显示

    geom->setVertexAttribArray(0, verts);
    geom->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(
        new osg::DrawElementsUInt(GL_TRIANGLES, vertIndices.size(), vertIndices.data()));
}
#endif
