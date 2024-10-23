#include <vis4earth/scalar_viser/heatmap.h>

#include <array>

#include <ui_heatmap.h>
#include <vis4earth/components_ui_export.h>

VIS4Earth::HeatmapRenderer::HeatmapRenderer(QWidget *parent)
    : volCmpt(true), QtOSGReflectableWidget(ui, parent) {
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&geoCmpt);
    ui->scrollAreaWidgetContents_main->layout()->addWidget(&volCmpt);

    initOSGResource();

    auto onResUpdated = [&]() {
        updateGeometry();
        updateHeatmap2D();
    };
    connect(ui->spinBox_resX, &QSpinBox::editingFinished, onResUpdated);
    connect(ui->spinBox_resY, &QSpinBox::editingFinished, onResUpdated);

    auto genHeatmapTex = [&]() {
        auto vol = volCmpt.GetVolume(0, 0);
        if (!vol)
            return;

        auto img3D = vol->getImage();
        volSliceImg->allocateImage(img3D->s(), img3D->t(), 1, GL_RED, GL_UNSIGNED_BYTE);
        volSliceImg->setInternalTextureFormat(GL_RED);
        auto res = static_cast<size_t>(img3D->s()) * img3D->t();
        std::memcpy(volSliceImg->data(),
                    img3D->data() + ui->spinBox_height_int_VIS4EarthReflectable->value() * res,
                    res);

        auto filterMode = [&]() {
            auto mode = static_cast<ETextureFilterMode>(ui->comboBox_texFilterMode->currentIndex());
            if (mode == ETextureFilterMode::Nearest)
                return osg::Texture::FilterMode::NEAREST;
            return osg::Texture::FilterMode::LINEAR;
        }();
        volSliceTex->setFilter(osg::Texture::MAG_FILTER, filterMode);
        volSliceTex->setFilter(osg::Texture::MIN_FILTER, filterMode);
        volSliceTex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        volSliceTex->setWrap(osg::Texture::WRAP_T, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        volSliceTex->setWrap(osg::Texture::WRAP_R, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        volSliceTex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
        volSliceTex->setImage(volSliceImg);
    };
    connect(&volCmpt, &VolumeComponent::VolumeChanged, [&, genHeatmapTex]() {
        auto vol = volCmpt.GetVolume(0, 0);
        if (!vol)
            return;

        auto h = vol->getImage()->r();
        ui->spinBox_height_int_VIS4EarthReflectable->setMaximum(h - 1);
        volHeight->set(h);

        genHeatmapTex();
        updateHeatmap2D();
    });
    connect(ui->spinBox_height_int_VIS4EarthReflectable,
            QOverload<int>::of(&QSpinBox::valueChanged), [&, genHeatmapTex]() {
                genHeatmapTex();
                updateHeatmap2D();
            });
    connect(ui->comboBox_texFilterMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            genHeatmapTex);

    auto changeTF = [&]() {
        auto stateSet = geode->getOrCreateStateSet();
        stateSet->setTextureAttributeAndModes(0, volCmpt.GetTransferFunction(0),
                                              osg::StateAttribute::ON);

        updateHeatmap2D();
    };
    connect(&volCmpt, &VolumeComponent::TransferFunctionChanged, changeTF);
    changeTF();

    updateGeometry();

    debugProperties({this, &volCmpt, &geoCmpt});
}

void VIS4Earth::HeatmapRenderer::initOSGResource() {
    grp = new osg::Group();
    geom = new osg::Geometry();
    geode = new osg::Geode();
    verts = new osg::Vec3Array();
    volSliceTex = new osg::Texture2D();
    volSliceImg = new osg::Image();
    program = new osg::Program();

    auto stateSet = geode->getOrCreateStateSet();
    volHeight = new osg::Uniform("volHeight", 0);
    stateSet->addUniform(volHeight);
    for (auto obj : std::array<QtOSGReflectableWidget *, 3>{this, &geoCmpt, &volCmpt})
        obj->ForEachProperty([&](const std::string &name, const Property &prop) {
            stateSet->addUniform(prop.GetUniform());
        });
    {
        osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
            osg::Shader::VERTEX,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/heatmap_vert.glsl");
        osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT,
            GetDataPathPrefix() + VIS4EARTH_SHADER_PREFIX "scalar_viser/heatmap_frag.glsl");
        program->addShader(vertShader);
        program->addShader(fragShader);
    }
    {
        auto tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex");
        tfTexUni->set(0);
        stateSet->addUniform(tfTexUni);

        tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "volSliceTex");
        tfTexUni->set(1);
        stateSet->addUniform(tfTexUni);

        stateSet->setTextureAttributeAndModes(1, volSliceTex, osg::StateAttribute::ON);
    }
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(program, osg::StateAttribute::ON);

    geode->addDrawable(geom);
    grp->addChild(geode);
}

void VIS4Earth::HeatmapRenderer::updateHeatmap2D() {
    if (heatmap2D.width() != ui->spinBox_resX->value() ||
        heatmap2D.height() != ui->spinBox_resY->value())
        heatmap2D =
            QImage(ui->spinBox_resX->value(), ui->spinBox_resY->value(), QImage::Format_RGB32);

    if (!volCmpt.GetVolume(0, 0))
        return;

    auto &vol = volCmpt.GetVolumeCPU(0, 0);
    auto &tfFlatDat = volCmpt.GetTransferFunctionCPU(0).GetFlatData();
    std::array<float, 3> scale{1.f * (vol.GetVoxelPerVolume()[0] - 1) / heatmap2D.width(),
                               1.f * (vol.GetVoxelPerVolume()[1] - 1) / heatmap2D.height(),
                               1.f * (vol.GetVoxelPerVolume()[2] - 1) /
                                   (volCmpt.GetVolume(0, 0)->getImage()->r() - 1)};
    int z = ui->spinBox_height_int_VIS4EarthReflectable->value() * scale[2];
    for (int y = 0; y < heatmap2D.height(); ++y) {
        auto pxPtr = reinterpret_cast<QRgb *>(heatmap2D.scanLine(heatmap2D.height() - 1 - y));
        for (int x = 0; x < heatmap2D.width(); ++x, ++pxPtr) {
            auto scalar = vol.Sample<uint8_t>(scale[0] * x, scale[1] * y, z);
            auto &rgba = tfFlatDat[scalar];
            *pxPtr = qRgb(rgba[0] * 255.f, rgba[1] * 255.f, rgba[2] * 255.f);
        }
    }

    auto pixmap = QPixmap::fromImage(heatmap2D);
    if (!ui->graphicsView_2DView->scene())
        ui->graphicsView_2DView->setScene(new QGraphicsScene);
    auto scn = ui->graphicsView_2DView->scene();
    scn->clear();
    scn->addPixmap(pixmap);

    ui->graphicsView_2DView->fitInView(scn->itemsBoundingRect(), Qt::KeepAspectRatio);
    update();
}

void VIS4Earth::HeatmapRenderer::updateGeometry() {
    verts->clear();

    auto res = ui->spinBox_resX->value() * ui->spinBox_resY->value();
    verts->reserve(res);
    for (int latIdx = 0; latIdx <= ui->spinBox_resY->value(); ++latIdx)
        for (int lonIdx = 0; lonIdx <= ui->spinBox_resX->value(); ++lonIdx) {
            osg::Vec3 pos(1.f * lonIdx / ui->spinBox_resX->value(),
                          1.f * latIdx / ui->spinBox_resY->value(), 0.f);

            verts->push_back(pos);
        }

    std::vector<GLuint> vertIndices;
    auto addTri = [&](std::array<GLuint, 3> triIndices) {
        for (uint8_t i = 0; i < 3; ++i)
            vertIndices.emplace_back(triIndices[i]);
    };
    auto addBotSurf = [&](int latIdx, int lonIdx) {
        std::array<GLuint, 4> quadIndices = {latIdx * ui->spinBox_resX->value() + lonIdx,
                                             latIdx * ui->spinBox_resX->value() + lonIdx + 1,
                                             (latIdx + 1) * ui->spinBox_resX->value() + lonIdx + 1,
                                             (latIdx + 1) * ui->spinBox_resX->value() + lonIdx};

        addTri({quadIndices[0], quadIndices[1], quadIndices[2]});
        addTri({quadIndices[2], quadIndices[3], quadIndices[0]});
    };
    for (int latIdx = 0; latIdx < ui->spinBox_resY->value(); ++latIdx)
        for (int lonIdx = 0; lonIdx < ui->spinBox_resX->value(); ++lonIdx)
            addBotSurf(latIdx, lonIdx);

    geom->setInitialBound([]() -> osg::BoundingBox {
        osg::Vec3 max(osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR, osg::WGS_84_RADIUS_POLAR);
        return osg::BoundingBox(-max, max);
    }()); // 必须，否则不显示

    geom->setVertexArray(verts);
    geom->getPrimitiveSetList().clear();
    geom->addPrimitiveSet(
        new osg::DrawElementsUInt(GL_TRIANGLES, vertIndices.size(), vertIndices.data()));
}
