#ifndef VIS4EARTH_INFO_VISER_PC_H
#define VIS4EARTH_INFO_VISER_PC_H

#include <string>

#include <array>
#include <map>
#include <unordered_map>
#include <utility>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>
#include <osg/Shapedrawable>

#include <osgUtil/DelaunayTriangulator>
#include <osgText/Text>
#include <osg/LineWidth>
#include <osg/StateAttribute>


namespace VIS4Earth
{
	namespace InfoViser
	{
		//class ScatterPlot
		class PieChart
		{

		private:
			struct PerPlotParam
			{
				osg::ref_ptr<osg::Group> grp;
				osg::ref_ptr<osg::Program> program;
				PerPlotParam()
				{
					grp = new osg::Group;
					program = new osg::Program;

				}
			};

			PerPlotParam param;

			class PerVolParam
			{
			private:
				uint8_t rndrVertsBufIdx;
				std::array<uint32_t, 3> volDim;
				osg::Vec3 voxSz;
				float isoVal;
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				bool volStartFromLonZero;

				std::shared_ptr<std::vector<float>> volDat;
				std::shared_ptr<std::vector<osg::Vec3f>> volDiscreteDat;
				// std::shared_ptr<std::vector<std::pair<std::wstring, float>>> pieDat;
				std::shared_ptr<std::vector<std::pair<const wchar_t*, float>>> pieDat;


				osg::ref_ptr<osg::Geometry> geom;
				osg::ref_ptr<osg::Geode> geode;
				osg::ref_ptr<osg::Vec3Array> verts;
				osg::ref_ptr<osg::Vec3Array> norms;

			public:
				PerVolParam(decltype(volDat) volDat, const std::array<uint32_t, 3>& volDim,
					PerPlotParam* ploter)
					: volDat(volDat), volDim(volDim), rndrVertsBufIdx(0)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					minLongtitute = deg2Rad(-10.f);
					maxLongtitute = deg2Rad(+10.f);
					minLatitute = deg2Rad(-20.f);
					maxLatitute = deg2Rad(+20.f);
					minHeight = MinHeight;
					maxHeight = MaxHeight;
					volStartFromLonZero = false;

					voxSz = osg::Vec3(1.f / volDim[0], 1.f / volDim[1], 1.f / volDim[2]);

					verts = new osg::Vec3Array;
					norms = new osg::Vec3Array;

					geom = new osg::Geometry;
					geode = new osg::Geode;
					geode->addDrawable(geom);

					auto states = geode->getOrCreateStateSet();

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					states->setAttributeAndModes(ploter->program, osg::StateAttribute::ON);
				}
				PerVolParam(decltype(volDiscreteDat) volDiscreteDat,
					PerPlotParam* ploter)
					: volDiscreteDat(volDiscreteDat), rndrVertsBufIdx(0)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					minLongtitute = deg2Rad(-10.f);
					maxLongtitute = deg2Rad(+10.f);
					minLatitute = deg2Rad(-20.f);
					maxLatitute = deg2Rad(+20.f);
					minHeight = MinHeight;
					maxHeight = MaxHeight;
					volStartFromLonZero = false;

					voxSz = osg::Vec3(1.f / volDim[0], 1.f / volDim[1], 1.f / volDim[2]);

					verts = new osg::Vec3Array;
					norms = new osg::Vec3Array;

					geom = new osg::Geometry;
					geode = new osg::Geode;
					geode->addDrawable(geom);

					auto states = geode->getOrCreateStateSet();

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					states->setAttributeAndModes(ploter->program, osg::StateAttribute::ON);
				}

				PerVolParam(decltype(pieDat) pieDat,
					PerPlotParam* ploter)
					: pieDat(pieDat), rndrVertsBufIdx(0)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					minLongtitute = deg2Rad(-10.f);
					maxLongtitute = deg2Rad(+10.f);
					minLatitute = deg2Rad(-20.f);
					maxLatitute = deg2Rad(+20.f);
					minHeight = MinHeight;
					maxHeight = MaxHeight;
					volStartFromLonZero = false;

					voxSz = osg::Vec3(1.f / volDim[0], 1.f / volDim[1], 1.f / volDim[2]);

					verts = new osg::Vec3Array;
					norms = new osg::Vec3Array;

					geom = new osg::Geometry;
					geode = new osg::Geode;
					geode->addDrawable(geom);

					auto states = geode->getOrCreateStateSet();

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					states->setAttributeAndModes(ploter->program, osg::StateAttribute::ON);
				}
				/*
				* 函数: SetLongtituteRange
				* 功能: 设置该体绘制时的经度范围（单位为角度）
				* 参数:
				* -- minLonDeg: 经度最小值
				* -- maxLonDeg: 经度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetLongtituteRange(float minLonDeg, float maxLonDeg)
				{
					if (minLonDeg < -180.f) return false;
					if (maxLonDeg > +180.f) return false;
					if (minLonDeg >= maxLonDeg) return false;

					minLongtitute = deg2Rad(minLonDeg);
					maxLongtitute = deg2Rad(maxLonDeg);
					return true;
				}
				/*
				* 函数: SetLatituteRange
				* 功能: 设置该体绘制时的纬度范围（单位为角度）
				* 参数:
				* -- minLatDeg: 纬度最小值
				* -- maxLatDeg: 纬度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetLatituteRange(float minLatDeg, float maxLatDeg)
				{
					if (minLatDeg < -90.f) return false;
					if (maxLatDeg > +90.f) return false;
					if (minLatDeg >= maxLatDeg) return false;

					minLatitute = deg2Rad(minLatDeg);
					maxLatitute = deg2Rad(maxLatDeg);
					return true;
				}
				/*
				* 函数: SetHeightFromCenterRange
				* 功能: 设置该体绘制时的高度（距球心）范围
				* 参数:
				* -- minH: 高度最小值
				* -- maxH: 高度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetHeightFromCenterRange(float minH, float maxH)
				{
					if (minH < 0.f) return false;
					if (minH >= maxH) return false;

					minHeight = minH;
					maxHeight = maxH;
					return true;
				}
				/*
				* 函数: SetVolumeStartFromLongtituteZero
				* 功能: 若全球体数据X=0对应的经度为0度，需要开启该功能
				* 参数:
				* -- flag: 为true时，开启该功能。为false时，关闭该功能
				*/
				void SetVolumeStartFromLongtituteZero(bool flag)
				{
					volStartFromLonZero = flag;
				}



				/*
				* 函数: DrawPlot
				* 功能: 绘制散点图
				* 参数:
				* -- vec: 三维散点坐标集合
				*/

				void DrawPlot(std::vector<osg::Vec3f>& vec)
				{
					//精细度
					osg::TessellationHints* hints1 = new osg::TessellationHints();
					//设置精细度
					hints1->setDetailRatio(0.3f);

					auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						float h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					for (size_t i = 0; i < vec.size(); ++i) {
						osg::Vec3f sphereLoc = vec3ToSphere(vec[i]);
						osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(sphereLoc, osg::WGS_84_RADIUS_EQUATOR / 100);
						osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints1);
						pShapeDrawable->setColor(osg::Vec4(vec[i], 1.0));
						geode->addDrawable(pShapeDrawable.get());
					}
					auto states = geode->getOrCreateStateSet();
					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
				}

				/*
				* 函数: GetDiscreteVec
				* 功能: 得到离散数据的散点的三维球坐标
				* 参数:
				* -- eps : 误差
				* -- base : 基准值
				*/
				std::vector<osg::Vec3f> GetDiscreteVec(
					const std::array<int32_t, 3>& cooDimMax, const std::array<int32_t, 3>& cooDimMin,
					float eps = 0.011f, float base = 30.f / 255.f)
				{
					std::string errMsg;
					{
						std::vector<osg::Vec3f> ret;

						for (size_t i = 0; i < volDiscreteDat->size(); ++i) {
							osg::Vec3f pointer1 = volDiscreteDat->at(i);
							osg::Vec3f volLoc = osg::Vec3f((pointer1.x() - cooDimMin[0]) * 1.0f / (cooDimMax[0] - cooDimMin[0]), (pointer1.y() - cooDimMin[1]) * 1.0f / (cooDimMax[1] - cooDimMin[1]), (pointer1.z() - cooDimMin[2]) * 1.0f / (cooDimMax[2] - cooDimMin[2]));

							ret.push_back(volLoc);
						}
						return ret;
					}
				ERR:
					std::cerr << errMsg << std::endl;
					return std::vector<osg::Vec3f>();
				}

				/*
				* 函数: MakeLineChart
				* 功能: 绘制折线图
				*/
				osg::Geode* MakeLineChart(
					const std::array<int32_t, 3>& cooDimMax, const std::array<int32_t, 3>& cooDimMin,
					osg::Vec4f lcColor = osg::Vec4f(1.0, 1.0, 1.0, 1.0)
				)
				{
					auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						float h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					std::vector<osg::Vec3f> locaPoint;
					//精细度
					osg::TessellationHints* hints = new osg::TessellationHints();
					//设置精细度
					hints->setDetailRatio(0.3f);

					for (size_t i = 0; i < volDiscreteDat->size(); ++i) {
						osg::Vec3f pointer1 = volDiscreteDat->at(i);
						osg::Vec3f volLoc = osg::Vec3f((pointer1.x() - cooDimMin[0]) * 1.0f / (cooDimMax[0] - cooDimMin[0]), (pointer1.y() - cooDimMin[1]) * 1.0f / (cooDimMax[1] - cooDimMin[1]), (pointer1.z() - cooDimMin[2]) * 1.0f / (cooDimMax[2] - cooDimMin[2]));

						locaPoint.push_back(volLoc);
					}

					// 设置
					// 折线图信息
					osg::ref_ptr<osg::Geode> lcGeode = new osg::Geode();
					osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
					osg::ref_ptr<osg::Geometry> lGeom = new osg::Geometry();

					// 创建顶点数组
					osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
					// 创建颜色数组
					osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

					for (size_t i = 1; i < locaPoint.size(); ++i) {
						// 点
						osg::Vec3f sphereLoc0, sphereLoc1;
						sphereLoc0 = vec3ToSphere(locaPoint[i - 1]);
						sphereLoc1 = vec3ToSphere(locaPoint[i]);
						osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(sphereLoc1, osg::WGS_84_RADIUS_EQUATOR / 100);
						osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints);
						pShapeDrawable->setColor(lcColor);
						lcGeode->addDrawable(pShapeDrawable);

						if (i == 1) {
							osg::ref_ptr<osg::Sphere> pSphereShape0 = new osg::Sphere(sphereLoc0, osg::WGS_84_RADIUS_EQUATOR / 100);
							osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable0 = new osg::ShapeDrawable(pSphereShape0.get(), hints);
							pShapeDrawable0->setColor(lcColor);
							lcGeode->addDrawable(pShapeDrawable0);
							vertices->push_back(sphereLoc0);
							colors->push_back(lcColor);
						}
						vertices->push_back(sphereLoc1);
						colors->push_back(lcColor);
					}

					lGeom->setVertexArray(vertices.get());
					lGeom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
					lGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size()));
					lcGeode->addDrawable(lGeom);

					lcGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
					lcGeode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					lcGeode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0), osg::StateAttribute::ON);

					return lcGeode.release();

				}

				/*
				* 函数: MakeLineChart
				* 功能: 绘制折线图
				*/
				osg::Geode* MakeLineChart(
					osg::Vec4f lcColor = osg::Vec4f(1.0, 1.0, 1.0, 1.0)
				)
				{
					auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						float h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					std::vector<osg::Vec3f> locaPoint;
					//精细度
					osg::TessellationHints* hints = new osg::TessellationHints();
					//设置精细度
					hints->setDetailRatio(0.3f);

					for (size_t i = 0; i < volDiscreteDat->size(); ++i) {
						osg::Vec3f pointer1 = volDiscreteDat->at(i);
						locaPoint.push_back(pointer1);
					}

					// 设置
					// 折线图信息
					osg::ref_ptr<osg::Geode> lcGeode = new osg::Geode();
					osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
					osg::ref_ptr<osg::Geometry> lGeom = new osg::Geometry();

					// 创建顶点数组
					osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
					// 创建颜色数组
					osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

					for (size_t i = 1; i < locaPoint.size(); ++i) {
						// 点
						osg::Vec3f sphereLoc0, sphereLoc1;
						sphereLoc0 = vec3ToSphere(locaPoint[i - 1]);
						sphereLoc1 = vec3ToSphere(locaPoint[i]);
						osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(sphereLoc1, osg::WGS_84_RADIUS_EQUATOR / 100);
						osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints);
						pShapeDrawable->setColor(lcColor);
						lcGeode->addDrawable(pShapeDrawable);

						if (i == 1) {
							osg::ref_ptr<osg::Sphere> pSphereShape0 = new osg::Sphere(sphereLoc0, osg::WGS_84_RADIUS_EQUATOR / 100);
							osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable0 = new osg::ShapeDrawable(pSphereShape0.get(), hints);
							pShapeDrawable0->setColor(lcColor);
							lcGeode->addDrawable(pShapeDrawable0);
							vertices->push_back(sphereLoc0);
							colors->push_back(lcColor);
						}
						vertices->push_back(sphereLoc1);
						colors->push_back(lcColor);

					}

					// std::cout << locaPoint.size() << std::endl;

					lGeom->setVertexArray(vertices.get());
					lGeom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
					lGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size()));
					lcGeode->addDrawable(lGeom);

					lcGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
					lcGeode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					lcGeode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0), osg::StateAttribute::ON);

					return lcGeode.release();

				}

				/*
				* 函数: MakePieChart
				* 功能: 绘制饼状图
				*/
				osg::Geode* MakePieChart(float fontSize = osg::WGS_84_RADIUS_EQUATOR / 40)
				{
					auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						float h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					// 底面曲线
					osg::ref_ptr<osg::Geometry> bottomGeom = new osg::Geometry();
					osg::Vec3f bottomCenterPoint = osg::Vec3f(0.5, 0.5, 0.0);
					osg::ref_ptr<osg::Vec3Array> bVer = new osg::Vec3Array();
					osg::ref_ptr<osg::Vec4Array> bColor = new osg::Vec4Array();

					for (int i = 0; i < 360; i++) {
						osg::Vec3f loc;
						loc.x() = 0.5 * sinf(deg2Rad(i / 1.0)) + bottomCenterPoint.x();
						loc.y() = 0.5 * cosf(deg2Rad(i / 1.0)) + bottomCenterPoint.y();
						loc.z() = 0.0 + bottomCenterPoint.z();
						bVer->push_back(osg::Vec3(vec3ToSphere(loc)));
					}

					bottomGeom->setVertexArray(bVer.get());

					bColor->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));  // 红色
					bottomGeom->setColorArray(bColor, osg::Array::BIND_OVERALL);

					bottomGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, bVer->size()));

					osg::ref_ptr<osg::Geode> pcGeode = new osg::Geode();

					pcGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
					pcGeode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					pcGeode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0), osg::StateAttribute::ON);

					pcGeode->addDrawable(bottomGeom.get());

					float startAngle = 0.0;
					for (size_t i = 0; i < pieDat->size(); ++i) {
						float prop = pieDat->at(i).second;
						//osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
						osg::Vec3f point0 = osg::Vec3f(bottomCenterPoint.x(), bottomCenterPoint.y(), prop);
						osg::Vec3f point1 = osg::Vec3f(bottomCenterPoint.x(), bottomCenterPoint.y(), 0.0);

						osg::Vec3f point2 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle)), prop);
						osg::Vec3f point3 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle)), 0.0);

						osg::Vec3f point4 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + prop * 360.0)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + prop * 360.0)), prop);
						osg::Vec3f point5 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + prop * 360.0)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + prop * 360.0)), 0.0);
						float step = prop * 360.0 / 50.0;

						osg::ref_ptr<osg::Vec4Array> pColor = new osg::Vec4Array();
						pColor->push_back(osg::Vec4(prop, 1 - prop, i * 1.0 / pieDat->size(), 1.0f));
						// 顶部扇形面
						for (float i = startAngle; i < startAngle + prop * 360.0; i += step) {
							osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
							osg::ref_ptr<osg::Vec3Array> pVer = new osg::Vec3Array();

							osg::Vec3f loc0, loc1;
							loc0.x() = 0.5 * sinf(deg2Rad(i)) + bottomCenterPoint.x();
							loc0.y() = 0.5 * cosf(deg2Rad(i)) + bottomCenterPoint.y();
							loc0.z() = prop;

							loc1.x() = 0.5 * sinf(deg2Rad(i + step)) + bottomCenterPoint.x();
							loc1.y() = 0.5 * cosf(deg2Rad(i + step)) + bottomCenterPoint.y();
							loc1.z() = prop;
							pVer->push_back(vec3ToSphere(point0));
							pVer->push_back(vec3ToSphere(loc0));
							pVer->push_back(vec3ToSphere(loc1));
							pGeom->setVertexArray(pVer.get());

							pGeom->setColorArray(pColor, osg::Array::BIND_OVERALL);
							pGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, pVer->size()));

							osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

							normals->push_back(vec3ToSphere(osg::Vec3(0.5, 0.5, 1.0)) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
							//normals->push_back(osg::Vec3(0.0, 0.0, 1.0));
							normals->back().normalize();
							pGeom->setNormalArray(normals, osg::Array::BIND_OVERALL);

							pcGeode->addDrawable(pGeom.get());
						}

						// 侧面长方形面0
						osg::ref_ptr<osg::Geometry> pGeom0 = new osg::Geometry();
						osg::ref_ptr<osg::Vec3Array> pVer0 = new osg::Vec3Array();

						pVer0->push_back(vec3ToSphere(point0));
						// pVer0->push_back(vec3ToSphere(point1));
						for (int i = 0; i <= 50; ++i) {
							pVer0->push_back(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle)) * i / 50.0, bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle)) * i / 50.0, 0.0)));
						}
						pVer0->push_back(vec3ToSphere(point2));
						//// pVer0->push_back(vec3ToSphere(point3));
						pGeom0->setVertexArray(pVer0.get());

						pGeom0->setColorArray(pColor, osg::Array::BIND_OVERALL);
						pGeom0->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, pVer0->size()));

						osg::ref_ptr<osg::Vec3Array> normal0 = new osg::Vec3Array;

						normal0->push_back(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle - 90.0)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle - 90.0)), 0.0)) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
						//normals->push_back(vec3ToSphere(loc1) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
						normal0->back().normalize();
						pGeom0->setNormalArray(normal0, osg::Array::BIND_OVERALL);
						pcGeode->addDrawable(pGeom0.get());


						// 侧面长方形面1
						osg::ref_ptr<osg::Geometry> pGeom1 = new osg::Geometry();
						osg::ref_ptr<osg::Vec3Array> pVer1 = new osg::Vec3Array();

						pVer1->push_back(vec3ToSphere(point0));
						// pVer0->push_back(vec3ToSphere(point1));
						for (int i = 0; i <= 50; ++i) {
							pVer1->push_back(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + prop * 360.0)) * i / 50.0, bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + prop * 360.0)) * i / 50.0, 0.0)));
						}
						pVer1->push_back(vec3ToSphere(point4));
						//// pVer0->push_back(vec3ToSphere(point3));
						pGeom1->setVertexArray(pVer1.get());
						pGeom1->setColorArray(pColor, osg::Array::BIND_OVERALL);
						pGeom1->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, pVer1->size()));

						osg::ref_ptr<osg::Vec3Array> normal1 = new osg::Vec3Array;
						normal1->push_back(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + prop * 360.0 + 90.0)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + prop * 360.0 + 90.0)), 0.0)) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
						//normals->push_back(vec3ToSphere(loc1) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
						normal1->back().normalize();
						pGeom1->setNormalArray(normal1, osg::Array::BIND_OVERALL);
						pcGeode->addDrawable(pGeom1.get());


						// 侧面长方形面2 - 圆弧

						for (int i = 0; i < 50; ++i) {
							osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
							osg::ref_ptr<osg::Vec3Array> pVer = new osg::Vec3Array();


							osg::Vec3f loc0 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + i * prop * 360.0 / 50)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + i * prop * 360.0 / 50)), prop);
							osg::Vec3f loc1 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + i * prop * 360.0 / 50)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + i * prop * 360.0 / 50)), 0.0);

							osg::Vec3f loc2 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + (i + 1) * prop * 360.0 / 50)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + (i + 1) * prop * 360.0 / 50)), 0.0);
							osg::Vec3f loc3 = osg::Vec3f(bottomCenterPoint.x() + 0.5 * sinf(deg2Rad(startAngle + (i + 1) * prop * 360.0 / 50)), bottomCenterPoint.y() + 0.5 * cosf(deg2Rad(startAngle + (i + 1) * prop * 360.0 / 50)), prop);

							pVer->push_back(vec3ToSphere(loc0));
							pVer->push_back(vec3ToSphere(loc1));
							pVer->push_back(vec3ToSphere(loc2));
							pVer->push_back(vec3ToSphere(loc3));


							pGeom->setVertexArray(pVer.get());

							pGeom->setColorArray(pColor, osg::Array::BIND_OVERALL);
							pGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, pVer->size()));


							osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

							normals->push_back(vec3ToSphere(loc1) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
							//normals->push_back(vec3ToSphere(loc1) - vec3ToSphere(osg::Vec3(0.5, 0.5, 0.0)));
							normals->back().normalize();
							pGeom->setNormalArray(normals, osg::Array::BIND_OVERALL);
							pcGeode->addDrawable(pGeom.get());
						}

						// 文字标注
						osg::ref_ptr<osgText::Text> dText = new osgText::Text;
						dText->setFont("fonts/simhei.ttf");
						dText->setCharacterSize(fontSize);
						dText->setAxisAlignment(osgText::Text::SCREEN);
						dText->setPosition(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.55 * sinf(deg2Rad(startAngle + prop * 360.0 / 2)), bottomCenterPoint.y() + 0.55 * cosf(deg2Rad(startAngle + prop * 360.0 / 2)), prop + 0.1)));
						dText->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
						dText->setText(pieDat->at(i).first);

						osg::ref_ptr<osgText::Text> pText = new osgText::Text;
						pText->setFont("fonts/simhei.ttf");
						pText->setCharacterSize(fontSize);
						pText->setAxisAlignment(osgText::Text::SCREEN);
						pText->setPosition(vec3ToSphere(osg::Vec3f(bottomCenterPoint.x() + 0.65 * sinf(deg2Rad(startAngle + prop * 360.0 / 2 + 5.0)), bottomCenterPoint.y() + 0.65 * cosf(deg2Rad(startAngle + prop * 360.0 / 2 + 5.0)), prop + 0.1)));
						pText->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));

						float percent = pieDat->at(i).second * 100;
						std::string str = std::to_string(percent);
						str = str.substr(0, str.find(".") + 2);
						str = str + "%";
						pText->setText(str);
						pcGeode->addDrawable(dText.get());
						pcGeode->addDrawable(pText.get());


						startAngle += prop * 360.0;
					}


					return pcGeode.release();



				}


				/*
				* 函数: MakeCoordinate
				* 功能: 绘制坐标轴
				*/
				osg::Geode* MakeCoordinate(const wchar_t* nameX = L"x", const wchar_t* nameY = L"y", const wchar_t* nameZ = L"z", float fontSize = osg::WGS_84_RADIUS_EQUATOR / 20)
				{
					auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						float h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					// 绘制弧线
					osg::ref_ptr<osg::Geometry> zGeom = new osg::Geometry();
					// 原点

					osg::Vec3f axisOrigin = vec3ToSphere(osg::Vec3f(0.0f, 0.0f, 0.0f));
					// z轴
					osg::ref_ptr<osg::Vec3Array> zVer = new osg::Vec3Array();
					zVer->push_back(axisOrigin);
					zVer->push_back(vec3ToSphere(osg::Vec3f(0.0f, 0.0f, 1.0f)));
					zGeom->setVertexArray(zVer.get());

					osg::ref_ptr<osg::Vec4Array> zColor = new osg::Vec4Array();
					zColor->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
					zColor->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
					zGeom->setColorArray(zColor.get());
					zGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
					zGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));

					// y轴
					osg::ref_ptr<osg::Geometry> yGeom = new osg::Geometry();
					osg::ref_ptr<osg::Vec3Array> yVer = new osg::Vec3Array();
					for (float i = 0.0f; i <= 1.0f; i += 0.01f) {
						yVer->push_back(osg::Vec3(vec3ToSphere(osg::Vec3f(0.0f, i, 0.0f))));
					}
					yGeom->setVertexArray(yVer.get());
					osg::ref_ptr<osg::Vec4Array> yColor = new osg::Vec4Array();
					for (int i = 0; i < yVer->size(); ++i) {
						yColor->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
					}
					yGeom->setColorArray(yColor.get());
					yGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
					yGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, yVer->size()));

					// x轴
					osg::ref_ptr<osg::Geometry> xGeom = new osg::Geometry();
					osg::ref_ptr<osg::Vec3Array> xVer = new osg::Vec3Array();
					for (float i = 0.0f; i <= 1.0f; i += 0.01f) {
						xVer->push_back(osg::Vec3(vec3ToSphere(osg::Vec3f(i, 0.0f, 0.0f))));
					}
					xGeom->setVertexArray(xVer.get());
					osg::ref_ptr<osg::Vec4Array> xColor = new osg::Vec4Array();
					for (int i = 0; i < xVer->size(); ++i) {
						xColor->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
					}
					xGeom->setColorArray(xColor.get());
					xGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
					xGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, xVer->size()));


					osg::ref_ptr<osg::Geode> axisGeode = new osg::Geode();
					axisGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
					axisGeode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					axisGeode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(3.0), osg::StateAttribute::ON);

					axisGeode->addDrawable(zGeom.get());
					axisGeode->addDrawable(yGeom.get());
					axisGeode->addDrawable(xGeom.get());

					// 添加文字
					osg::ref_ptr<osgText::Text> pTextXAuxis1 = new osgText::Text;
					pTextXAuxis1->setText(nameX);

					pTextXAuxis1->setFont("Fonts/simhei.ttf");
					pTextXAuxis1->setAxisAlignment(osgText::Text::SCREEN);
					pTextXAuxis1->setCharacterSize(fontSize);
					pTextXAuxis1->setPosition(osg::Vec3(osg::Vec3(vec3ToSphere(osg::Vec3f(1.0f, 0.0f, 0.1f)))));
					pTextXAuxis1->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
					osg::ref_ptr<osgText::Text> pTextYAuxis1 = new osgText::Text;
					pTextYAuxis1->setText(nameY);
					pTextYAuxis1->setFont("Fonts/simhei.ttf");
					pTextYAuxis1->setAxisAlignment(osgText::Text::SCREEN);
					pTextYAuxis1->setCharacterSize(fontSize);
					pTextYAuxis1->setPosition(osg::Vec3(osg::Vec3(vec3ToSphere(osg::Vec3f(0.0f, 1.0f, 0.1f)))));
					pTextYAuxis1->setColor(osg::Vec4(0.0, 1.0, 0.0, 1.0));
					osg::ref_ptr<osgText::Text> pTextZAuxis1 = new osgText::Text;
					pTextZAuxis1->setText(nameZ);
					pTextZAuxis1->setFont("Fonts/simhei.ttf");
					pTextZAuxis1->setAxisAlignment(osgText::Text::SCREEN);
					pTextZAuxis1->setCharacterSize(fontSize);
					pTextZAuxis1->setPosition(osg::Vec3(osg::Vec3(vec3ToSphere(osg::Vec3f(0.01f, 0.01f, 1.01f)))));
					pTextZAuxis1->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
					axisGeode->addDrawable(pTextXAuxis1.get());
					axisGeode->addDrawable(pTextYAuxis1.get());
					axisGeode->addDrawable(pTextZAuxis1.get());


					return axisGeode.release();
				}

			private:
				float deg2Rad(float deg)
				{
					return deg * osg::PI / 180.f;
				};

				friend class PieChart;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			PieChart() {}


			/*
			* 函数: GetGroup
			* 功能: 获取该绘制组件的OSG节点
			* 返回值: OSG节点
			*/
			osg::Group* GetGroup() { return param.grp.get(); }
			/*
			* 函数: AddVolume
			* 功能: 向该绘制组件添加一个体
			* 参数:
			* -- name: 添加体的名称。不同体的名称需不同，用于区分
			* -- volDat: 体数据，需按Z-Y-X的顺序存放体素
			* -- dim: 体数据的三维尺寸（XYZ顺序）
			*/
			void AddData(const std::string& name, std::shared_ptr<std::vector<float>> volDat,
				const std::array<uint32_t, 3>& volDim)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.geode);
					vols.erase(itr);
				}
				vols.emplace(std::pair<std::string, PerVolParam>
					(name, PerVolParam(volDat, volDim, &param)));
				param.grp->addChild(vols.at(name).geode);
			}

			/*
			* 函数: AddDiscreteVolume
			* 功能: 向该绘制组件添加一个离散数据体
			* 参数:
			* -- name: 添加体的名称。不同体的名称需不同，用于区分
			* -- volDat: 体数据，需按Z-Y-X的顺序存放体素
			*/
			void AddDiscreteData(const std::string& name, std::shared_ptr<std::vector<osg::Vec3f>> volDiscreteDat)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.geode);
					vols.erase(itr);
				}
				vols.emplace(std::pair<std::string, PerVolParam>
					(name, PerVolParam(volDiscreteDat, &param)));
				param.grp->addChild(vols.at(name).geode);
			}

			/*
			* 函数: AddPieData
			* 功能: 向该绘制组件添加一个离散数据体
			* 参数:
			* -- name: 添加体的名称。不同体的名称需不同，用于区分
			* -- volDat: 体数据，需按Z-Y-X的顺序存放体素
			*/
			void AddPieData(const std::string& name, std::shared_ptr<std::vector<std::pair<const wchar_t*, float>>> volPieDat)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.geode);
					vols.erase(itr);
				}
				vols.emplace(std::pair<std::string, PerVolParam>
					(name, PerVolParam(volPieDat, &param)));
				param.grp->addChild(vols.at(name).geode);
			}
			/*
			* 函数: GetVolumes
			* 功能: 获取该组件中，体在绘制时所需的所有数据
			*/
			std::map<std::string, PerVolParam>& GetDatas()
			{
				return vols;
			}
			/*
			* 函数: GetVolume
			* 功能: 获取该组件中，体在绘制时所需的数据
			* 参数:
			* -- name: 体的名称
			* 返回值: 体的绘制数据
			*/
			PerVolParam* GetData(const std::string& name)
			{
				auto itr = vols.find(name);
				if (itr == vols.end())
					return nullptr;
				return &(itr->second);
			}
			/*
			* 函数: GetVolumeNum
			* 功能: 获取该绘制组件中体的数量
			* 返回值: 体的数量
			*/
			size_t GetDataNum() const
			{
				return vols.size();
			}

		}; // class PieChart
	} // namespace InfoViser
} // namespace VIS4Earth


#endif