#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <vis4earth/osg_util.h>
#include <vis4earth/info_viser/radar_chart.h>


static const std::array<float, 2> lonRng = { -180.00f, 180.00f };
static const std::array<float, 2> latRng = { -90.00f, 90.00f };
static const std::array<float, 2> hRng = { 1.f, 21264.f };
static const float hScale = 100.f;

static const std::string filePath = "D:/Project/OSG/vis-on-earth-qt-osg/data/radar_chart.json";

int main(int argc, char** argv)
{
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);

	// 相机
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(VIS4Earth::CreateEarth());

	VIS4Earth::InfoViser::RadarChart* mcb
		= new VIS4Earth::InfoViser::RadarChart;
	std::string errMsg;
	{
		mcb->SetLongtituteRange(lonRng[0], lonRng[1]);
		mcb->SetLatituteRange(latRng[0], latRng[1]);
		mcb->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		
		mcb->LoadFromJsonFile(filePath);

		mcb->addColors(100);
		mcb->SetLevel(10);

		grp->addChild(mcb->MakeRadarBackground());
		grp->addChild(mcb->MakeRadarChart());
		grp->addChild(mcb->GetGeode());
	}

	

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