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

    auto onHeightChanged = [&](double) {
        sphere->setShape(new osg::Sphere(
            osg::Vec3(0.f, 0.f, 0.f),
            static_cast<float>(osg::WGS_84_RADIUS_POLAR) +
                geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable->value()));
        ssaoSphere->setShape(new osg::Sphere(
            osg::Vec3(0.f, 0.f, 0.f),
            static_cast<float>(osg::WGS_84_RADIUS_POLAR) +
                geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable->value()));
    };
    connect(geoCmpt.GetUI()->doubleSpinBox_heightMax_float_VIS4EarthReflectable,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), onHeightChanged);
    onHeightChanged(0.);

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
        auto stateSet = sphere->getOrCreateStateSet();
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

        auto stateSet = sphere->getOrCreateStateSet();
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
    sphere = new osg::ShapeDrawable();
    program = new osg::Program;

    auto stateSet = sphere->getOrCreateStateSet();
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
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_vert.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_frag.glsl");
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

    colorTex = new osg::Texture2D;
    normalTex = new osg::Texture2D;
    depthTex = new osg::Texture2D;
    {
        colorTex->setTextureSize(2048, 1024);
        colorTex->setInternalFormat(GL_RGBA32F_ARB);
        colorTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        colorTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        auto colorImage = new osg::Image;
        colorImage->allocateImage(2048, 1024, 1, GL_RGBA, GL_FLOAT);
        colorTex->setImage(colorImage);
        auto colorBinding = new osg::BindImageTexture(
            0, colorTex, osg::BindImageTexture::WRITE_ONLY, GL_RGBA32F_ARB);
        stateSet->setTextureAttributeAndModes(0, colorTex, osg::StateAttribute::ON);
        stateSet->setAttribute(colorBinding);
        stateSet->addUniform(new osg::Uniform("colorImage", 0));

        normalTex->setTextureSize(2048, 1024);
        normalTex->setInternalFormat(GL_RGBA32F_ARB);
        normalTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        normalTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        auto normalImage = new osg::Image;
        normalImage->allocateImage(2048, 1024, 1, GL_RGBA, GL_FLOAT);
        normalTex->setImage(normalImage);
        auto normalBinding = new osg::BindImageTexture(
            1, normalTex, osg::BindImageTexture::WRITE_ONLY, GL_RGBA32F_ARB);
        stateSet->setTextureAttributeAndModes(1, normalTex, osg::StateAttribute::ON);
        stateSet->setAttribute(normalBinding);
        stateSet->addUniform(new osg::Uniform("normalImage", 1));

        depthTex->setTextureSize(2048, 1024);
        depthTex->setInternalFormat(GL_RGBA32F_ARB);
        depthTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        depthTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        auto depthImage = new osg::Image;
        depthImage->allocateImage(2048, 1024, 1, GL_RGBA, GL_FLOAT);
        depthTex->setImage(depthImage);
        auto depthBinding = new osg::BindImageTexture(
            2, depthTex, osg::BindImageTexture::WRITE_ONLY, GL_RGBA32F_ARB);
        stateSet->setTextureAttributeAndModes(2, depthTex, osg::StateAttribute::ON);
        stateSet->setAttribute(depthBinding);
        stateSet->addUniform(new osg::Uniform("depthImage", 2));
    }

    osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
    stateSet->setAttributeAndModes(cf);

    stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    sphere->setCullCallback(new EyePositionUpdateCallback(eyePos));

    grp->addChild(sphere);

    // SSAO shader
    ssaoSphere = new osg::ShapeDrawable();
    ssaoProgram = new osg::Program;

    auto ssaoStateSet = ssaoSphere->getOrCreateStateSet();
    ssaoStateSet->addUniform(dSamplePoss[0]);
    for (auto obj : std::array<QtOSGReflectableWidget *, 3>{this, &geoCmpt, &volCmpt})
        obj->ForEachProperty([&](const std::string &name, const Property &prop) {
            ssaoStateSet->addUniform(prop.GetUniform());
        });
    {
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_vert.glsl");
        ssaoProgram->addShader(vertShader);

        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/dvr_ssao_frag.glsl");
        ssaoProgram->addShader(fragShader);
    }
    {
        ssaoStateSet->setTextureAttributeAndModes(0, colorTex, osg::StateAttribute::ON);
        auto colorBinding = new osg::BindImageTexture(0, colorTex, osg::BindImageTexture::READ_ONLY,
                                                      GL_RGBA32F_ARB);
        ssaoStateSet->setAttribute(colorBinding);
        ssaoStateSet->addUniform(new osg::Uniform("colorImage", 0));

        ssaoStateSet->setTextureAttributeAndModes(1, normalTex, osg::StateAttribute::ON);
        auto normalBinding = new osg::BindImageTexture(
            1, normalTex, osg::BindImageTexture::READ_ONLY, GL_RGBA32F_ARB);
        ssaoStateSet->setAttribute(normalBinding);
        ssaoStateSet->addUniform(new osg::Uniform("normalImage", 1));

        ssaoStateSet->setTextureAttributeAndModes(2, depthTex, osg::StateAttribute::ON);
        auto depthBinding = new osg::BindImageTexture(2, depthTex, osg::BindImageTexture::READ_ONLY,
                                                      GL_RGBA32F_ARB);
        ssaoStateSet->setAttribute(depthBinding);
        ssaoStateSet->addUniform(new osg::Uniform("depthImage", 2));
    }
    {
        // generate sample kernel
        const int kernelSize = 64;
        ssaoStateSet->addUniform(new osg::Uniform("kernelSize", kernelSize));
        auto sampleUni = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "samples", 64);
        std::uniform_real_distribution<float> randomFloats(0.0,
                                                           1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator(time(nullptr));
        for (unsigned int i = 0; i < kernelSize; ++i) {
            osg::Vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                             randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            sample.normalize();
            sample *= randomFloats(generator);
            float scale = float(i) / kernelSize;

            // scale samples s.t. they're more aligned to center of kernel
            scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
            sample *= scale;
            sampleUni->setElement(i, sample);
        }
        ssaoStateSet->addUniform(sampleUni);

        // generate noise texture
        ssaoNoise = new float[16 * 4];
        for (unsigned int i = 0; i < 16; i++) {
            ssaoNoise[i * 4] = randomFloats(generator) * 2.0 - 1.0;
            ssaoNoise[i * 4 + 1] = randomFloats(generator) * 2.0 - 1.0;
            ssaoNoise[i * 4 + 2] = 0.f;
            ssaoNoise[i * 4 + 3] = 1.f;
        }
        auto noiseTex = new osg::Texture2D;
        noiseTex->setTextureSize(4, 4);
        noiseTex->setInternalFormat(GL_RGBA16F_ARB);
        noiseTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        noiseTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        noiseTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        noiseTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        auto noiseImage = new osg::Image;
        noiseImage->setImage(4, 4, 1, GL_RGBA, GL_RGBA, GL_FLOAT, (unsigned char *)ssaoNoise,
                             osg::Image::USE_NEW_DELETE);
        noiseImage->dirty();
        noiseTex->setImage(noiseImage);
        ssaoStateSet->setTextureAttributeAndModes(3, noiseTex, osg::StateAttribute::ON);
        ssaoStateSet->addUniform(new osg::Uniform("texNoise", 3));
    }

    ssaoStateSet->setAttributeAndModes(cf);

    ssaoStateSet->setAttributeAndModes(ssaoProgram, osg::StateAttribute::ON);
    ssaoStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    ssaoStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    ssaoSphere->setCullCallback(new EyePositionUpdateCallback(eyePos));

    grp->addChild(ssaoSphere);
}
