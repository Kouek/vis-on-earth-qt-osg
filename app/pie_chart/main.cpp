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

#include <vis4earth/info_viser/pie_chart.h>

//static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
//static const std::string volDPath0 = "C:/Code/bin/data/vis-osg-scatter-plot/piechart0.txt";
//static const std::string volDPath1 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart1.txt";
//static const std::string volDPath2 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart2.txt";
//static const std::string volDPath3 = "C:/Code/bin/data/vis-osg-scatter-plot/linechart3.txt";
static const std::string volName0 = "0";
//static const std::string volName1 = "1";
//static const std::string volName2 = "2";
//static const std::string volName3 = "3";

// unused
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

	// 相机
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(VIS4Earth::CreateEarth());

	std::shared_ptr<VIS4Earth::InfoViser::PieChart> mcb
		= std::make_shared<VIS4Earth::InfoViser::PieChart>();
	std::string errMsg;
	{
		std::vector<std::pair<const wchar_t*, float>> pieDat0;
		pieDat0.push_back(std::make_pair(L"第一季度", 0.2f));
		pieDat0.push_back(std::make_pair(L"第二季度", 0.3f));
		pieDat0.push_back(std::make_pair(L"第三季度", 0.4f));
		pieDat0.push_back(std::make_pair(L"第四季度", 0.1f));
		//pieDat0.push_back(std::make_pair(L"w", 0.15f));

		if (!errMsg.empty())
			goto ERR;
		auto volDatShrd0 = std::make_shared<std::vector<std::pair<const wchar_t*, float>>>(pieDat0);
		mcb->AddPieData(volName0, volDatShrd0);
		auto vol0 = mcb->GetData(volName0);
		vol0->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol0->SetLatituteRange(latRng[0], latRng[1]);
		vol0->SetHeightFromCenterRange(.7f, .75f);
		vol0->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);

		grp->addChild(vol0->MakePieChart());
		//grp->addChild(vol0->MakeCoordinate());

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