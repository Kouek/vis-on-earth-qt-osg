#include <memory>
#include <array>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>


#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>


#include <vis4earth/osg_util.h>


#include "vis4earth/io/nlohmann/json.hpp"
#include <vis4earth/scalar_viser/marching_cube_renderer.h>

#include <vis4earth/info_viser/parallel_coordinate.h>
static const std::string filePath = "D:/Project/OSG/vis-on-earth-qt-osg/data/para_coor.json";

static const std::array<uint32_t, 3> dim = { 300, 350, 50 };			// unused
static const std::array<uint32_t, 3> graphDim = { 100, 100, 50 };		// unused
static const std::array<int32_t, 3> coordinateDimMax = { 3, 3, 2 };		// unused
static const std::array<int32_t, 3> coordinateDimMin = { -3, -3, -2 };	// unused
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };				// unused

static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95f };
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

	VIS4Earth::InfoViser::ParallelCoordinate* mcb
		= new VIS4Earth::InfoViser::ParallelCoordinate;
	std::string errMsg;
	{
		mcb->SetLongtituteRange(lonRng[0], lonRng[1]);
		mcb->SetLatituteRange(latRng[0], latRng[1]);
		mcb->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		// 随机生成50种不同颜色
		mcb->addColors(50);
		
		mcb->LoadFromJsonFile(filePath);
		mcb->DrawParallelAxis();
		
		std::vector<osg::Vec3f> volDiscreteDat0;
		
		
		int colorIdx = 0;
		// 把点集完成分类后，需要循环画他们的折线图
		for (auto& t : mcb->pointSetsClassified) {
			volDiscreteDat0 = t.second;
			
			if (!errMsg.empty())
				goto ERR;
			auto volDatShrd0 = std::make_shared<std::vector<osg::Vec3f>>(volDiscreteDat0);

			
			grp->addChild(mcb->MakeLineChart(volDiscreteDat0, mcb->colors[colorIdx]));
			//grp->addChild(mcb->MakeCoordinate());
			
			// 给这条线做文字标注
			osg::ref_ptr<osg::Geode> geodeText = new osg::Geode;
			osg::ref_ptr<osgText::Text> dText = new osgText::Text;
			dText->setFont("fonts/simhei.ttf");
			dText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 40);
			dText->setAxisAlignment(osgText::Text::SCREEN);
			dText->setAutoRotateToScreen(true); // 跟随屏幕旋转
			dText->setPosition(mcb->vec3ToSphere(volDiscreteDat0[0] - (volDiscreteDat0[1] - volDiscreteDat0[0]) / 3.0));
			dText->setColor(mcb->colors[colorIdx]);
			dText->setText(t.first);
			geodeText->addDrawable(dText.get());
			grp->addChild(geodeText.get());

			colorIdx++;
		}
		
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


/*


option = {
  parallelAxis: [
	{ name: 'Price', minimum: 0.0, maximum: 20.0},
	{ name: 'Net Weight', minimum: 0, maximum: 120},
	{ name: 'Amount' , minimum: 0, maximum: 100},
	{
	  name: 'Score',
	  type: 'category',
	  data: ['Excellent', 'Good', 'OK', 'Bad']
	}
  ],
  series: {
	data: [
	  ['A', 12.99, 100, 82, 'Good'],
	  ['B', 9.99, 80, 77, 'OK'],
	  ['C', 20, 120, 60, 'Excellent']
	]
  }
};


*/

