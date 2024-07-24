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

#include <vis4earth/info_viser/line_chart.h>
// D:/Project/OSG/vis-on-earth-qt-osg/data
static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
// static const std::string volDPath0 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart0.txt";
static const std::string volDPath0 = "D:/Project/OSG/vis-on-earth-qt-osg/data/linechart0.txt";
static const std::string volDPath1 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart1.txt";
static const std::string volDPath2 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart2.txt";
static const std::string volDPath3 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart3.txt";
static const std::string volName0 = "0";
static const std::string volName1 = "1";
static const std::string volName2 = "2";
static const std::string volName3 = "3";

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
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);

	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(VIS4Earth::CreateEarth());

	std::shared_ptr<VIS4Earth::InfoViser::LineChart> mcb
		= std::make_shared<VIS4Earth::InfoViser::LineChart>();
	std::string errMsg;
	{
		auto volDiscreteDat0 = VIS4Earth::Loader::TXTVolume::LoadFromFile(volDPath0);
		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd0 = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat0);
		mcb->AddDiscreteData(volName0, volDatShrd0);
		auto vol0 = mcb->GetData(volName0);
		vol0->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol0->SetLatituteRange(latRng[0], latRng[1]);
		vol0->SetHeightFromCenterRange(.7f, .75f);
		vol0->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		// std::vector<osg::Vec3f> point = vol->GetDiscreteVec(coordinateDimMax, coordinateDimMin);
		// vol->DrawPlot(point);
		// vol->DrawPlot(volDiscreteDat);


		grp->addChild(vol0->MakeLineChart());
		grp->addChild(vol0->MakeCoordinate());

		auto volDiscreteDat1 = VIS4Earth::Loader::TXTVolume::LoadFromFile(volDPath1);
		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd1 = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat1);
		mcb->AddDiscreteData(volName1, volDatShrd1);
		auto vol1 = mcb->GetData(volName1);
		vol1->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol1->SetLatituteRange(latRng[0], latRng[1]);
		vol1->SetHeightFromCenterRange(.7f, .75f);
		vol1->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		// std::vector<osg::Vec3f> point = vol->GetDiscreteVec(coordinateDimMax, coordinateDimMin);
		// vol->DrawPlot(point);
		// vol->DrawPlot(volDiscreteDat);


		grp->addChild(vol1->MakeLineChart(osg::Vec4f(0.6, 0.3, 0.5, 1.0)));


		auto volDiscreteDat2 = VIS4Earth::Loader::TXTVolume::LoadFromFile(volDPath2);
		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd2 = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat2);
		mcb->AddDiscreteData(volName2, volDatShrd2);
		auto vol2 = mcb->GetData(volName2);
		vol2->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol2->SetLatituteRange(latRng[0], latRng[1]);
		vol2->SetHeightFromCenterRange(.7f, .75f);
		vol2->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		// std::vector<osg::Vec3f> point = vol->GetDiscreteVec(coordinateDimMax, coordinateDimMin);
		// vol->DrawPlot(point);
		// vol->DrawPlot(volDiscreteDat);


		grp->addChild(vol2->MakeLineChart(osg::Vec4f(0.3, 0.6, 0.5, 1.0)));

		auto volDiscreteDat3 = VIS4Earth::Loader::TXTVolume::LoadFromFile(volDPath3);
		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd3 = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat3);
		mcb->AddDiscreteData(volName3, volDatShrd3);
		auto vol3 = mcb->GetData(volName3);
		vol3->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol3->SetLatituteRange(latRng[0], latRng[1]);
		vol3->SetHeightFromCenterRange(.7f, .75f);
		vol3->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		// std::vector<osg::Vec3f> point = vol->GetDiscreteVec(coordinateDimMax, coordinateDimMin);
		// vol->DrawPlot(point);
		// vol->DrawPlot(volDiscreteDat);


		grp->addChild(vol3->MakeLineChart(osg::Vec4f(0.3, 0.4, 0.7, 1.0)));



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
