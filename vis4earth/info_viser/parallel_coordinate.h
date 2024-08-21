#ifndef VIS4EARTH_INFO_VISER_LC_H
#define VIS4EARTH_INFO_VISER_LC_H

#include <string>

#include <array>
#include <map>
#include <unordered_map>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <cmath>


#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>
#include <osg/Shapedrawable>
#include <osg/Vec4f>
#include <osg/Vec3f>
#include <osg/Material>

#include <osgUtil/DelaunayTriangulator>
#include <osgText/Text>
#include <osg/LineWidth>
#include <osg/StateAttribute>

#include "vis4earth/io/nlohmann/json.hpp"

using json = nlohmann::json;

namespace VIS4Earth
{
	namespace InfoViser
	{
		class ParallelCoordinate
		{
		public: 
			// 平行轴的元信息
			struct Axis {
				// 包含信息：name/type/range(取值范围)
				std::string name;
				float pos_x, pos_y;
				std::string type; // 'category' or 'numeric'
				std::vector<std::string> category;
				float maximum, minimum;
			};
			std::vector<Axis> parallelAxis;
			// data -> 转化为相应高度的比例值 -> 再转化为一系列依据名称分类三维点集
			std::vector<std::pair<std::string, std::vector<osg::Vec3f>>> pointSetsClassified;

			void LoadFromJsonFile(const std::string& filePath) {
				std::cout << "LoadFromJsonFile ------------" << std::endl;
				
				// 从文件中读取配置
				std::ifstream file(filePath);
				json config;
				file >> config;
				// 提取平行轴元信息
				std::cout << "Parallel Axis ---------------" << std::endl;
				for (const auto& axisData : config["parallelAxis"]) {
					Axis axis;
					axis.name = axisData["name"];
					std::cout << "  name: " << axis.name << std::endl;
					axis.pos_x = axisData["coordinate"][0];
					axis.pos_y = axisData["coordinate"][1];
					std::cout << "   pos: (" << axis.pos_x << ", " << axis.pos_y << ")" << std::endl;
					axis.type = axisData.value("type", "numeric");
					std::cout << "  type: " << axis.type << std::endl;
					if (axis.type == "category") {
						for (int i = 0; i < axisData["data"].size(); i++) {
							axis.category.push_back(axisData["data"][i]);
							std::cout << "  category: " << axisData["data"][i] << std::endl;
						}
							
					}
					else {
						// numeric 
						axis.minimum = axisData["minimum"];
						axis.maximum = axisData["maximum"];
						std::cout << "  minimum: " << axisData["minimum"] << std::endl;
						std::cout << "  maximum: " << axisData["maximum"] << std::endl;
					}
					parallelAxis.push_back(axis);
				}
				
				
				std::cout << std::endl << "Data --------------------" << std::endl;
				// 提取数据
				for (const auto& data : config["series"]["data"]) {
					// 一组数据
					// std::vector<float> heights;
					std::vector<osg::Vec3f> points;
					std::string name = (std::string)data[0];
					std::cout << "  " << name << std::endl;
					int idx = 1;
					// name = (std::string)data[0];
					

					for (auto &axis : parallelAxis) {
						osg::Vec3f point;
						// 根据idx的值，指定三维点的x和y方向
						//point.x() = (float)idx * 0.1;
						//point.y() = (float)idx * 0.1;
						point.x() = axis.pos_x;
						point.y() = axis.pos_y;
						// 数值
						if (axis.type == "numeric") {
							
							// point.z() = minHeight + (maxHeight - minHeight) * ((float)data[idx] - axis.minimum) / (axis.maximum - axis.minimum);
							point.z() = 0.0 + (1.0 - 0.0) * ((float)data[idx] - axis.minimum) / (axis.maximum - axis.minimum);
							// heights.push_back();
							// 
						} // 枚举
						else {
							for (int i = 0; i < axis.category.size(); i++) {
								// 成功匹配
								if (axis.category[i] == (std::string)data[idx]) {
									// point.z() = minHeight + (maxHeight - minHeight) * (idx / axis.category.size());
									point.z() = 0.0 + (1.0 - 0.0) * (((float)i + 0.5) / (float)axis.category.size());
									// heights.push_back();
								}
							}
						}
						std::cout << "  point: (" << point.x() << ", " << point.y() << ", " << point.z() << ")" << std::endl;
						points.push_back(point);
						idx++;
					}

					pointSetsClassified.push_back(std::make_pair(name, points));
				}
			}

			void DrawGuides(osg::Vec3f from, osg::Vec3f to) {
				// std::cout << "from: " << from.x() <<  << std::endl;
				std::cout << "  from: (" << from.x() << ", " << from.y() << ", " << from.z() << ")" << std::endl;
				std::cout << "    to: (" << to.x() << ", " << to.y() << ", " << to.z() << ")" << std::endl;
				float xStep = (from.x() - to.x()) / 100.0;
				float yStep = (from.y() - to.y()) / 100.0;
				float zStep = (from.z() - to.z()) / 100.0;


				std::vector<osg::Vec3f> points;
				osg::Vec3f point = from;
				// point.z() += 0.01 * 6.37825e+06;
				// point.x() += xStep;
				point -= osg::Vec3f(xStep, yStep, zStep);
				int idx = 100;
				do {
					std::cout << "  point: (" << point.x() << ", " << point.y() << ", " << point.z() << ")" << std::endl;
					points.push_back(point);
					// point.z() += 0.01 * 6.37825e+06;
					point -= osg::Vec3f(xStep, yStep, zStep);
					idx--;
					if (!idx) break;
					
				} while (point != to);

				//精细度
				osg::TessellationHints* hints1 = new osg::TessellationHints();
				//设置精细度
				hints1->setDetailRatio(0.3f);

				for (size_t i = 0; i < points.size(); ++i) {
					osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(points[i], osg::WGS_84_RADIUS_EQUATOR / 500);
					osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints1);
					geode->addDrawable(pShapeDrawable.get());
				}
				auto states = geode->getOrCreateStateSet();
				states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
			}

			void printPoint(osg::Vec3f p) {
				std::cout << " (" << p.x() << ", " << p.y() << ", " << p.z() << ")" << std::endl;
			}

			void DrawParallelAxis() {
				auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
					//std::cout << "v3: ";
					//printPoint(v3);
					//std::cout << "Lon Range: " << minLongtitute << " " << maxLongtitute << std::endl;
					//std::cout << "Lat Range: " << minLatitute << " " << maxLatitute << std::endl;
					float dlt = maxLongtitute - minLongtitute;
					//std::cout << "dlt: " << std::endl;
					float x = volStartFromLonZero == 0 ? v3.x() :
						v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
					float lon = minLongtitute + x * dlt;
					dlt = maxLatitute - minLatitute;
					//std::cout << "dlt: " << std::endl;
					float lat = minLatitute + v3.y() * dlt;
					dlt = maxHeight - minHeight;
					//std::cout << "dlt: " << std::endl;
					float h = minHeight + v3.z() * dlt;

					//std::cout << "lat: " << lat << std::endl;
					//std::cout << "lon: " << lon << std::endl;
					//std::cout << "  h: " << h << std::endl;

					osg::Vec3 ret;
					ret.z() = h * sinf(lat);
					h = h * cosf(lat);
					ret.y() = h * sinf(lon);
					ret.x() = h * cosf(lon);

					return ret;
					};
			


				for (int i = 0; i < parallelAxis.size(); i++) {
					std::cout << "DrawGuides" << std::endl;
					// 画分段点
					std::vector<osg::Vec3f> vec;
					//parallelAxis[i].pos_x
					//0.1 * (i + 1)

					if (parallelAxis[i].type == "numeric") {
						// 分5段（暂定分度值为5）

						for (int j = 0; j <= 5; j++) {
							vec.push_back(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, (1.0 / 5.0) * j));
							// 文字标注
							osg::ref_ptr<osgText::Text> dText = new osgText::Text;
							dText->setFont("fonts/simhei.ttf");
							dText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 60);
							dText->setAxisAlignment(osgText::Text::SCREEN);
							dText->setAutoRotateToScreen(true); // 跟随屏幕旋转
							dText->setPosition(vec3ToSphere(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, (1.0 / 5.0) * j)));
							dText->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
							dText->setText(std::to_string((int)(parallelAxis[i].minimum + (parallelAxis[i].maximum - parallelAxis[i].minimum) * (j / 5.0))));
							geode->addDrawable(dText.get());
						}
						
					}
					else {
						// 要依据category有几个离散分段值确定
						for (int j = 0; j <= parallelAxis[i].category.size(); j++) {
							vec.push_back(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, (1.0 / parallelAxis[i].category.size()) * j));
							if (j != parallelAxis[i].category.size()) {
								// 文字标注
								osg::ref_ptr<osgText::Text> dText = new osgText::Text;
								dText->setFont("fonts/simhei.ttf");
								dText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 60);
								dText->setAxisAlignment(osgText::Text::SCREEN);
								dText->setAutoRotateToScreen(true); // 跟随屏幕旋转
								dText->setPosition(vec3ToSphere(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, (1.0 / parallelAxis[i].category.size()) * (j + 0.5))));
								dText->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
								dText->setText(parallelAxis[i].category[j]);
								geode->addDrawable(dText.get());
							}
						}
					}
					DrawPlot(vec, osg::WGS_84_RADIUS_EQUATOR / 250);
					
					
					// 画轴
					osg::ref_ptr<osg::Vec3Array> v = new osg::Vec3Array;
					v->push_back(vec3ToSphere(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, 0.0)));
					v->push_back(vec3ToSphere(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, 1.0)));
					osg::ref_ptr<osg::Geometry> axisGeom = new osg::Geometry;
					axisGeom->setVertexArray(v); 
					osg::ref_ptr<osg::DrawArrays> drawArrayLines = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2); // 两个顶点  
					axisGeom->addPrimitiveSet(drawArrayLines);

					// 线宽和颜色
					osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;  
					osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
					lineWidth->setWidth(5.0f); 
					stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON); 
					

					osg::ref_ptr<osg::Material> material = new osg::Material;
					material->setDiffuse(osg::Material::FRONT, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
					stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);

					axisGeom->setStateSet(stateSet);
					geode->addDrawable(axisGeom);

					// 标注轴的名称
					osg::ref_ptr<osgText::Text> axisText = new osgText::Text;
					axisText->setFont("fonts/simhei.ttf");
					axisText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 60);
					axisText->setAxisAlignment(osgText::Text::SCREEN);
					axisText->setAutoRotateToScreen(true); // 跟随屏幕旋转
					axisText->setPosition(vec3ToSphere(osg::Vec3f(parallelAxis[i].pos_x, parallelAxis[i].pos_y, 1.1)));
					axisText->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
					axisText->setText(parallelAxis[i].name);
					geode->addDrawable(axisText.get());
				}
			}

			osg::Geode* GetGeode() { return geode.release(); }
		private:
			float minLongtitute, maxLongtitute;
			float minLatitute, maxLatitute;
			float minHeight, maxHeight;
			bool volStartFromLonZero{ true };

			osg::ref_ptr<osg::Geometry> geom;
			osg::ref_ptr<osg::Geode> geode;

			float deg2Rad(float deg)
			{
				return deg * osg::PI / 180.f;
			};
		public:
			
			ParallelCoordinate() {
				geom = new osg::Geometry;
				geode = new osg::Geode;
				geode->addDrawable(geom);
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
			void DrawPlot(std::vector<osg::Vec3f>& vec, const double size = osg::WGS_84_RADIUS_EQUATOR / 100)
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
					osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(sphereLoc, size);
					osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints1);
					// pShapeDrawable->setColor(osg::Vec4(vec[i], 1.0));
					pShapeDrawable->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
					geode->addDrawable(pShapeDrawable.get());
				}
				auto states = geode->getOrCreateStateSet();
				states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
			}

			/*
				* 函数: MakeLineChart
				* 功能: 绘制折线图
				*/
			osg::Geode* MakeLineChart(
				std::vector<osg::Vec3f>& vec, osg::Vec4f lcColor = osg::Vec4f(1.0, 1.0, 1.0, 1.0)
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

					//std::cout << "lat: " << lat << std::endl;
					//std::cout << "lon: " << lon << std::endl;
					//std::cout << "  h: " << h << std::endl;

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

				for (size_t i = 0; i < vec.size(); ++i) {
					osg::Vec3f pointer1 = vec.at(i);
					locaPoint.push_back(pointer1);
				}

				// 设置
				// 折线图信息
				osg::ref_ptr<osg::Geode> lcGeode = new osg::Geode();
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

			std::vector<osg::Vec4f> colors;
			void addColors(int count) {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_real_distribution<float> dis(0.0, 1.0);

				for (int i = 0; i < count; i++) {
					colors.push_back(osg::Vec4f(dis(gen), dis(gen), dis(gen), 1.0f));
				}
			}
		
			osg::Vec3 vec3ToSphere(const osg::Vec3& v3) {
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

		}; // class ParallelCoordinate
	} // namespace InfoViser
} // namespace VIS4Earth


#endif