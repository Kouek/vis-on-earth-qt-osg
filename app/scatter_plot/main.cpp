#include <memory>

#include <array>


#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <vis4earth/osg_util.h>

#include <vis4earth/io/tf_io.h>
#include <vis4earth/io/tf_osg_io.h>
#include <vis4earth/io/vol_io.h>
#include <vis4earth/io/vol_osg_io.h>
#include <vis4earth/scalar_viser/marching_cube_renderer.h>

#include <vis4earth/info_viser/scatter_plot.h>

static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
static const std::string volDPath = "D:/Project/OSG/vis-on-earth-qt-osg/data/linechart0.txt";
static const std::string volName = "0";
static const std::array<uint32_t, 3> dim = { 300, 350, 50 };
static const std::array<uint32_t, 3> graphDim = { 100, 100, 50 };
static const std::array<int32_t, 3> coordinateDimMax = { 3, 3, 2 };
static const std::array<int32_t, 3> coordinateDimMin = { -3, -3, -2 };
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95 };
// static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const std::array<float, 2> hRng = { 1.f, 21264.f };
static const float hScale = 100.f;

int main(int argc, char** argv)
{
	VIS4Earth::InfoViser::ScatterPlot p;
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);


	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(VIS4Earth::CreateEarth());

	std::shared_ptr<VIS4Earth::InfoViser::ScatterPlot> mcb
		= std::make_shared<VIS4Earth::InfoViser::ScatterPlot>();
	std::string errMsg;
	{
		auto volDiscreteDat = VIS4Earth::Loader::TXTVolume::LoadFromFile(volDPath);
		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat);
		mcb->AddDiscreteVolume(volName, volDatShrd);
		auto vol = mcb->GetVolume(volName);
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(.7f, .75f);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		std::vector<osg::Vec3f> point = vol->GetDiscreteVec(coordinateDimMax, coordinateDimMin);
		vol->DrawPlot(point);
		grp->addChild(vol->MakeCoordinate());
	}

	grp->addChild(mcb->GetGroup());

	viewer->setSceneData(grp);

	auto prevClk = clock();
	while (!viewer->done()) {
		auto currClk = clock();
		auto duration = currClk - prevClk;

		if (duration >= CLOCKS_PER_SEC / 45) {
			viewer->frame();
			prevClk = clock();
		}
	}
	return 0;

ERR:
	std::cerr << errMsg << std::endl;
	return 1;

}
