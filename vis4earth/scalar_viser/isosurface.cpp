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
#ifdef VIS4EARTH_USE_PARALLEL_MARCHING_CUBE
    initOCLResource();
#endif

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

VIS4Earth::IsosurfaceRenderer::~IsosurfaceRenderer() {
#ifdef VIS4EARTH_USE_PARALLEL_MARCHING_CUBE
    releaseOCLResource();
#endif
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

#define VIS4EARTH_OCL_CEHCK(call)                                                                  \
    {                                                                                              \
        cl_int _status = ##call;                                                                   \
        if (_status != CL_SUCCESS) {                                                               \
            qDebug() << "OpenCL error: " << _status;                                               \
            assert(_status == CL_SUCCESS);                                                         \
        }                                                                                          \
    }

#ifdef VIS4EARTH_USE_PARALLEL_MARCHING_CUBE
void VIS4Earth::IsosurfaceRenderer::initOCLResource() {
    cl_int status;
    auto printCompileError = [&](cl_int status, cl_program program) {
        if (status == CL_SUCCESS)
            return;

        size_t logSize = 0;
        clGetProgramBuildInfo(program, clDeviceID, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(program, clDeviceID, CL_PROGRAM_BUILD_LOG, logSize, log.data(),
                              nullptr);

        qDebug() << "OpenCL build error: " << status << "\n" << log.data();
    };

    {
        cl_uint platformNum = 0;
        VIS4EARTH_OCL_CEHCK(clGetPlatformIDs(1, nullptr, &platformNum));
        if (platformNum == 0) {
            qDebug() << "No OpenCL platform found.";
            return;
        }

        cl_platform_id clPlatformIDs[2];
        VIS4EARTH_OCL_CEHCK(clGetPlatformIDs(platformNum, clPlatformIDs, nullptr));
        clPlatformID = clPlatformIDs[1];
    }

    VIS4EARTH_OCL_CEHCK(clGetDeviceIDs(clPlatformID, CL_DEVICE_TYPE_GPU, 1, &clDeviceID, nullptr));
    clContext = clCreateContext(nullptr, 1, &clDeviceID, nullptr, nullptr, &status);
    VIS4EARTH_OCL_CEHCK(status);
    clCmdQue = clCreateCommandQueue(clContext, clDeviceID, 0, &status);
    VIS4EARTH_OCL_CEHCK(status);

    {
        const char *kernelSource =
            "__kernel\n"
            "void computeCornerState(__global unsigned char* cornerStates, __global const unsigned "
            "   char* vol, uchar isoval, uint3 gridPerVol, uint3 voxPerVol) {\n"
            "   uint3 coord;\n"
            "   coord.x = get_global_id(0);\n"
            "   coord.y = get_global_id(1);\n"
            "   coord.z = get_global_id(2);\n"
            "   if (coord.x >= gridPerVol.x || coord.y >= gridPerVol.y || coord.z >= "
            "       gridPerVol.z)\n"
            "       return;\n"
            "   uint idx = coord.z * gridPerVol.x * gridPerVol.y + "
            "       coord.y * gridPerVol.x + coord.x;\n"
            "\n"
            "   unsigned char cornerState = 0;\n"
            "   for (int i = 0; i < 8; ++i) {\n"
            "       unsigned char scalar = vol[coord.z * voxPerVol.x * voxPerVol.y + coord.y * "
            "                                   voxPerVol.x + coord.x];\n"
            "       if (scalar >= isoval)\n"
            "           cornerState |= 1 << i;\n"
            "\n"
            "       coord.x += i == 0 || i == 4 ? 1 : i == 2 || i == 6 ? -1 : 0;\n"
            "       coord.y += i == 1 || i == 5 ? 1 : i == 3 || i == 7 ? -1 : 0;\n"
            "       coord.z += i == 3 ? 1 : i == 7 ? -1 : 0;\n"
            "   }\n"
            "\n"
            "   cornerStates[idx] = cornerState;\n"
            "}";
        clProgComputeCornerState =
            clCreateProgramWithSource(clContext, 1, &kernelSource, nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);
        status =
            clBuildProgram(clProgComputeCornerState, 1, &clDeviceID, nullptr, nullptr, nullptr);
        printCompileError(status, clProgComputeCornerState);

        clKernelComputeCornerState =
            clCreateKernel(clProgComputeCornerState, "computeCornerState", &status);
        VIS4EARTH_OCL_CEHCK(status);
    }
    {
        const char *kernelSource =
            "__kernel\n"
            "void computeVertices(__global float* vertices, __global "
            "float* uvs, __global const uint2* vert2packedIndices, __global const "
            "unsigned char* vol, uint vertNum, uint vertOffset, uint3 gridPerVol, uint3 voxPerVol, uint "
            "volID) {\n"
            "   uint idx = get_global_id(0);\n"
            "   if (idx >= vertNum)\n"
            "       return;\n"
            "\n"
            "   uint2 packedIdx = vert2packedIndices[idx];\n"
            "   uint gridIdx = packedIdx[0];\n"
            "   uint ei = packedIdx[1];\n"
            "\n"
            "   uint3 coord;\n"
            "   uint gridPerVolYxX = gridPerVol.x * gridPerVol.y;\n"
            "   coord.z = gridIdx / gridPerVolYxX;\n"
            "   gridIdx -= coord.z * gridPerVolYxX;\n"
            "   coord.y = gridIdx / gridPerVol.x;\n"
            "   coord.x = gridIdx - coord.y * gridPerVol.x;\n"
            "\n"
            "   unsigned char scalars[8];"
            "   for (int i = 0; i < 8; ++i) {\n"
            "       scalars[i] = vol[coord.z * voxPerVol.x * voxPerVol.y + "
            "                       coord.y * voxPerVol.x + coord.x];\n"
            "\n"
            "       coord.x += i == 0 || i == 4 ? 1 : i == 2 || i == 6 ? -1 : 0;\n"
            "       coord.y += i == 1 || i == 5 ? 1 : i == 3 || i == 7 ? -1 : 0;\n"
            "       coord.z += i == 3 ? 1 : i == 7 ? -1 : 0;\n"
            "   }\n"
            "   float omegas[12] = {1.f * scalars[0] / (scalars[1] + scalars[0]),\n"
            "                       1.f * scalars[1] / (scalars[2] + scalars[1]),\n"
            "                       1.f * scalars[3] / (scalars[3] + scalars[2]),\n"
            "                       1.f * scalars[0] / (scalars[0] + scalars[3]),\n"
            "                       1.f * scalars[4] / (scalars[5] + scalars[4]),\n"
            "                       1.f * scalars[5] / (scalars[6] + scalars[5]),\n"
            "                       1.f * scalars[7] / (scalars[7] + scalars[6]),\n"
            "                       1.f * scalars[4] / (scalars[4] + scalars[7]),\n"
            "                       1.f * scalars[0] / (scalars[0] + scalars[4]),\n"
            "                       1.f * scalars[1] / (scalars[1] + scalars[5]),\n"
            "                       1.f * scalars[2] / (scalars[2] + scalars[6]),\n"
            "                       1.f * scalars[3] / (scalars[3] + scalars[7])};\n"
            "   for (int i = 0; i < 12; ++i) {\n"
            "       if (isnan(omegas[i]))\n"
            "           omegas[i] = .5f;\n"
            "   }\n"
            "\n"
            "   float3 pos;\n"
            "   pos.x = coord.x + (ei == 0 || ei == 2 || ei == 4 || ei == 6 ? omegas[ei] : ei == 1 "
            "           || ei == 5 || ei == 9 || ei == 10 ? 1.f : 0.f);\n"
            "   pos.y = coord.y + (ei == 1 || ei == 3 || ei == 5 || ei == 7 ? omegas[ei] : ei == 2 "
            "           || ei == 6 || ei == 10 || ei == 11 ? 1.f : 0.f);\n"
            "   pos.z = coord.z + (ei >= 8 ? omegas[ei] : ei >= 4 ? 1.f : 0.f);\n"
            "   for (int i = 0; i < 3; ++i)\n"
            "       pos[i] /= voxPerVol[i];\n"
            "\n"
            "   float2 uv;\n"
            "   switch (ei) {\n"
            "   case 0:\n"
            "       uv.y = omegas[0] * scalars[0] + (1.f - omegas[0]) * scalars[1];\n"
            "       break;\n"
            "   case 1:\n"
            "       uv.y = omegas[1] * scalars[1] + (1.f - omegas[1]) * scalars[2];\n"
            "       break;\n"
            "   case 2:\n"
            "       uv.y = omegas[2] * scalars[3] + (1.f - omegas[2]) * scalars[2];\n"
            "       break;\n"
            "   case 3:\n"
            "       uv.y = omegas[3] * scalars[0] + (1.f - omegas[3]) * scalars[3];\n"
            "       break;\n"
            "   case 4:\n"
            "       uv.y = omegas[4] * scalars[4] + (1.f - omegas[4]) * scalars[5];\n"
            "       break;\n"
            "   case 5:\n"
            "       uv.y = omegas[5] * scalars[5] + (1.f - omegas[5]) * scalars[6];\n"
            "       break;\n"
            "   case 6:\n"
            "       uv.y = omegas[6] * scalars[7] + (1.f - omegas[6]) * scalars[6];\n"
            "       break;\n"
            "   case 7:\n"
            "       uv.y = omegas[7] * scalars[4] + (1.f - omegas[7]) * scalars[7];\n"
            "       break;\n"
            "   default:\n"
            "       uv.y = omegas[ei] * scalars[ei - 8] + (1.f - omegas[ei]) * scalars[ei - 4];\n"
            "   }\n"
            "   uv.x = volID;\n"
            "   uv.y = uv.y / 255.f;\n"
            "\n"
            "   idx += vertOffset;\n"
            "   vertices[idx * 3 + 0] = pos.x;\n"
            "   vertices[idx * 3 + 1] = pos.y;\n"
            "   vertices[idx * 3 + 2] = pos.z;\n"
            "   uvs[idx * 2 + 0] = uv.x;\n"
            "   uvs[idx * 2 + 1] = uv.y;\n"
            "}";
        clProgComputeVertices =
            clCreateProgramWithSource(clContext, 1, &kernelSource, nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);
        status = clBuildProgram(clProgComputeVertices, 1, &clDeviceID, nullptr, nullptr, nullptr);
        printCompileError(status, clProgComputeVertices);

        clKernelComputeVertices = clCreateKernel(clProgComputeVertices, "computeVertices", &status);
        VIS4EARTH_OCL_CEHCK(status);
    }
    {
        const char *kernelSource =
            "__kernel\n"
            "void computeNormals(__global float* normals, uint vertNum, uint vertOffset) {\n"
            "   uint idx = get_global_id(0);\n"
            "   if (idx >= vertNum)\n"
            "       return;\n"
            "   idx += vertOffset;\n"
            "\n"
            "   float3 normal;\n"
            "   normal.x = normals[idx * 3 + 0];\n"
            "   normal.y = normals[idx * 3 + 1];\n"
            "   normal.z = normals[idx * 3 + 2];\n"
            "   normal = normalize(normal);\n"
            "\n"
            "   normals[idx * 3 + 0] = normal.x;\n"
            "   normals[idx * 3 + 1] = normal.y;\n"
            "   normals[idx * 3 + 2] = normal.z;\n"
            "}";
        clProgComputeNormals =
            clCreateProgramWithSource(clContext, 1, &kernelSource, nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);
        status = clBuildProgram(clProgComputeNormals, 1, &clDeviceID, nullptr, nullptr, nullptr);
        printCompileError(status, clProgComputeNormals);

        clKernelComputeNormals = clCreateKernel(clProgComputeNormals, "computeNormals", &status);
        VIS4EARTH_OCL_CEHCK(status);
    }
}

void VIS4Earth::IsosurfaceRenderer::releaseOCLResource() {
    cl_int status;

    VIS4EARTH_OCL_CEHCK(clReleaseProgram(clProgComputeCornerState));
    VIS4EARTH_OCL_CEHCK(clReleaseKernel(clKernelComputeCornerState));
    VIS4EARTH_OCL_CEHCK(clReleaseProgram(clProgComputeVertices));
    VIS4EARTH_OCL_CEHCK(clReleaseKernel(clKernelComputeVertices));
    VIS4EARTH_OCL_CEHCK(clReleaseProgram(clProgComputeNormals));
    VIS4EARTH_OCL_CEHCK(clReleaseKernel(clKernelComputeNormals));

    VIS4EARTH_OCL_CEHCK(clReleaseCommandQueue(clCmdQue));
    VIS4EARTH_OCL_CEHCK(clReleaseContext(clContext));
}
#endif

void VIS4Earth::IsosurfaceRenderer::marchingCube(uint32_t volID) {
#ifdef VIS4EARTH_USE_PARALLEL_MARCHING_CUBE
    using T = uint8_t;

    cl_uint3 voxPerVol;
    {
        auto _voxPerVol = volCmpt.GetVolumeCPU(volID, 0).GetVoxelPerVolume();
        voxPerVol.x = _voxPerVol[0];
        voxPerVol.y = _voxPerVol[1];
        voxPerVol.z = _voxPerVol[2];
    }
    cl_uint3 gridPerVol = {voxPerVol.x - 1, voxPerVol.y - 1, voxPerVol.z - 1};
    size_t voxNum = static_cast<size_t>(voxPerVol.x) * static_cast<size_t>(voxPerVol.y) *
                    static_cast<size_t>(voxPerVol.z);
    size_t gridNum = static_cast<size_t>(gridPerVol.x) * static_cast<size_t>(gridPerVol.y) *
                     static_cast<size_t>(gridPerVol.z);

    cl_int status;
    auto printKernelExecError = [&](cl_int status, cl_kernel kernel) {
        if (status == CL_SUCCESS)
            return;

        qDebug() << "OpenCL kernel execution error: " << status << "\n"
                 << [&]() {
                        switch (status) {
                        case CL_SUCCESS:
                            return "Success";
                        case CL_INVALID_KERNEL_ARGS:
                            return "Invalid kernel arguments";
                        case CL_OUT_OF_RESOURCES:
                            return "Out of GPU resources";
                        case CL_INVALID_WORK_GROUP_SIZE:
                            return "Invalid workgroup size";
                        default:
                            return "Unknown error";
                        }
                    }();
    };

    cl_mem clVol =
        clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(T) * voxNum,
                       (void *)volCmpt.GetVolumeCPU(volID, 0).GetData().data(), &status);
    VIS4EARTH_OCL_CEHCK(status);

    std::vector<uint8_t> cornerStates(gridNum);
    {
        cl_mem clCornerStates = clCreateBuffer(clContext, CL_MEM_WRITE_ONLY,
                                               sizeof(uint8_t) * gridNum, nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);

        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeCornerState, 0, sizeof(cl_mem), &clCornerStates));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeCornerState, 1, sizeof(cl_mem), &clVol));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeCornerState, 2, sizeof(uint8_t), &isoval));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeCornerState, 3, sizeof(cl_uint3), &gridPerVol));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeCornerState, 4, sizeof(cl_uint3), &voxPerVol));
        size_t threadPerGPU[3] = {
            (gridPerVol.x + ThreadPerBlock3D[0] - 1) / ThreadPerBlock3D[0] * ThreadPerBlock3D[0],
            (gridPerVol.y + ThreadPerBlock3D[1] - 1) / ThreadPerBlock3D[1] * ThreadPerBlock3D[1],
            (gridPerVol.z + ThreadPerBlock3D[2] - 1) / ThreadPerBlock3D[2] * ThreadPerBlock3D[2]};
        clEnqueueNDRangeKernel(clCmdQue, clKernelComputeCornerState, 3, nullptr, threadPerGPU,
                               ThreadPerBlock3D, 0, nullptr, nullptr);
        printKernelExecError(status, clKernelComputeCornerState);

        VIS4EARTH_OCL_CEHCK(clEnqueueReadBuffer(clCmdQue, clCornerStates, CL_TRUE, 0,
                                                sizeof(uint8_t) * gridNum, cornerStates.data(), 0,
                                                nullptr, nullptr));

        VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clCornerStates));
    }

    std::vector<cl_uint2> vert2packedIndices;
    {
        struct HashEdge {
            size_t operator()(const std::array<int, 3> &edgeID) const {
                size_t hash = edgeID[0];
                hash = (hash << 32) | edgeID[1];
                hash = (hash << 2) | edgeID[2];
                return std::hash<size_t>()(hash);
            };
        };
        std::array<std::unordered_map<std::array<int, 3>, GLuint, HashEdge>, 2> edge2vertIDs;

        osg::Vec3i coord;
        uint32_t idx = 0;
        for (coord.z() = 0; coord.z() < gridPerVol.z; ++coord.z()) {
            if (coord.z() != 0) {
                edge2vertIDs[0] = std::move(edge2vertIDs[1]);
                edge2vertIDs[1].clear(); // hash map only stores vertices of 2 consecutive z-layers
            }

            for (coord.y() = 0; coord.y() < gridPerVol.y; ++coord.y())
                for (coord.x() = 0; coord.x() < gridPerVol.x; ++coord.x(), ++idx) {
                    uint8_t cornerState = cornerStates[idx];
                    if (VertNumTable[cornerState] == 0)
                        continue;

                    for (uint32_t vi = 0; vi < VertNumTable[cornerState]; vi += 3) {
                        for (uint32_t vii = 0; vii < 3; ++vii) {
                            auto ei = TriangleTable[cornerState][vi + vii];
                            std::array<int, 3> edgeID = {
                                coord.x() + (ei == 1 || ei == 5 || ei == 9 || ei == 10 ? 1 : 0),
                                coord.y() + (ei == 2 || ei == 6 || ei == 10 || ei == 11 ? 1 : 0),
                                ei >= 8                                    ? 2
                                : ei == 1 || ei == 3 || ei == 5 || ei == 7 ? 1
                                                                           : 0};
                            auto edge2vertIDIdx = ei >= 4 && ei < 8 ? 1 : 0;
                            auto itr = edge2vertIDs[edge2vertIDIdx].find(edgeID);
                            if (itr != edge2vertIDs[edge2vertIDIdx].end()) {
                                vertIndices.emplace_back(itr->second);
                                continue;
                            }

                            vertIndices.emplace_back(vert2packedIndices.size());
                            norms->push_back(osg::Vec3(0.0f, 0.0f, 0.0f));
                            edge2vertIDs[edge2vertIDIdx].emplace(edgeID, vertIndices.back());

                            vert2packedIndices.emplace_back(cl_uint2{idx, ei});
                        }

                        std::array<GLuint, 3> triVertIdxs = {vertIndices[vertIndices.size() - 3],
                                                             vertIndices[vertIndices.size() - 2],
                                                             vertIndices[vertIndices.size() - 1]};
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
    }

    uint32_t prevVertNum = verts->size();
    uint32_t vertNum = vert2packedIndices.size();
    if (vertNum == 0)
        return;

    verts->resize(prevVertNum + vertNum);
    uvs->resize(prevVertNum + vertNum);
    {
        cl_mem clVerts = clCreateBuffer(clContext, CL_MEM_WRITE_ONLY, sizeof(osg::Vec3) * vertNum,
                                        nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);
        cl_mem clUVs = clCreateBuffer(clContext, CL_MEM_WRITE_ONLY, sizeof(osg::Vec2) * vertNum,
                                      nullptr, &status);
        VIS4EARTH_OCL_CEHCK(status);
        cl_mem clVert2packedIndices =
            clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           sizeof(cl_uint2) * vertNum, vert2packedIndices.data(), &status);
        VIS4EARTH_OCL_CEHCK(status);

        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeVertices, 0, sizeof(cl_mem), &clVerts));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeVertices, 1, sizeof(cl_mem), &clUVs));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeVertices, 2, sizeof(cl_mem), &clVert2packedIndices));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeVertices, 3, sizeof(cl_mem), &clVol));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeVertices, 4, sizeof(uint32_t), &vertNum));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeVertices, 5, sizeof(uint32_t), &prevVertNum));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeVertices, 6, sizeof(cl_uint3), &gridPerVol));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeVertices, 7, sizeof(cl_uint3), &voxPerVol));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeVertices, 8, sizeof(uint32_t), &volID));
        size_t threadPerGPU[1] = {(vertNum + ThreadPerBlock1D[0] - 1) / ThreadPerBlock1D[0] *
                                  ThreadPerBlock1D[0]};
        clEnqueueNDRangeKernel(clCmdQue, clKernelComputeVertices, 1, nullptr, threadPerGPU,
                               ThreadPerBlock1D, 0, nullptr, nullptr);
        printKernelExecError(status, clKernelComputeVertices);

        VIS4EARTH_OCL_CEHCK(
            clEnqueueReadBuffer(clCmdQue, clVerts, CL_TRUE, 0, sizeof(osg::Vec3) * vertNum,
                                (void *)verts->getDataPointer(), 0, nullptr, nullptr));
        VIS4EARTH_OCL_CEHCK(
            clEnqueueReadBuffer(clCmdQue, clUVs, CL_TRUE, 0, sizeof(osg::Vec2) * vertNum,
                                (void *)uvs->getDataPointer(), 0, nullptr, nullptr));

        VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clVerts));
        VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clUVs));
        VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clVert2packedIndices));
    }

    VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clVol));

    for (uint32_t vi = 0; vi < vertIndices.size(); vi += 3) {
        std::array<uint32_t, 3> triVertIdxs = {vertIndices[vi + 0], vertIndices[vi + 1],
                                               vertIndices[vi + 2]};
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
    }

    {
        cl_mem clNorms =
            clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                           sizeof(osg::Vec3) * vertNum, (void *)norms->getDataPointer(), &status);
        VIS4EARTH_OCL_CEHCK(status);

        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeNormals, 0, sizeof(cl_mem), &clNorms));
        VIS4EARTH_OCL_CEHCK(clSetKernelArg(clKernelComputeNormals, 1, sizeof(uint32_t), &vertNum));
        VIS4EARTH_OCL_CEHCK(
            clSetKernelArg(clKernelComputeNormals, 2, sizeof(uint32_t), &prevVertNum));
        size_t threadPerGPU[1] = {(vertNum + ThreadPerBlock1D[0] - 1) / ThreadPerBlock1D[0] *
                                  ThreadPerBlock1D[0]};
        clEnqueueNDRangeKernel(clCmdQue, clKernelComputeNormals, 1, nullptr, threadPerGPU,
                               ThreadPerBlock1D, 0, nullptr, nullptr);
        printKernelExecError(status, clKernelComputeNormals);

        VIS4EARTH_OCL_CEHCK(
            clEnqueueReadBuffer(clCmdQue, clNorms, CL_TRUE, 0, sizeof(osg::Vec3) * vertNum,
                                (void *)norms->getDataPointer(), 0, nullptr, nullptr));

        VIS4EARTH_OCL_CEHCK(clReleaseMemObject(clNorms));
    }

#else
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
                    for (uint32_t ii = 0; ii < 3; ++ii) {
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
#endif
}

void VIS4Earth::IsosurfaceRenderer::updateGeometry(uint32_t volID) {
    if (vertIndices.empty())
        return;

    // Output vertices and normals to file
    {
        //QString fileName = QString("isosurface_%1.txt").arg(isoval);
        //QFile file(fileName);
        //if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //    QTextStream out(&file);
        //    for (uint32_t vi = 0; vi < verts->size(); ++vi) {
        //        const auto &vert = (*verts)[vi];
        //        const auto &uv = (*uvs)[vi];
        //        const auto &norm = (*norms)[vi];

        //        out << "v " << vert.x() << " " << vert.y() << " " << vert.z() << "\n";
        //        out << "uv " << uv.x() << " " << uv.y() << "\n";
        //        out << "n " << norm.x() << " " << norm.y() << " " << norm.z() << "\n";
        //    }
        //    file.close();
        //} else {
        //    qDebug() << "Failed to open file for writing: " << fileName;
        //}
    }

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
