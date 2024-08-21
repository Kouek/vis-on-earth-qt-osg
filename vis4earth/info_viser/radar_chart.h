#ifndef VIS4EARTH_INFO_VISER_PC_H
#define VIS4EARTH_INFO_VISER_PC_H

#include <string>

#include <array>
#include <map>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>
#include <osg/Shapedrawable>
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
		class RadarChart
		{

		public:
			struct Radar {
				std::string name;
				float maximum; 
			};
			std::vector<Radar> radars;
			std::vector<std::pair<std::string, std::vector<float>>> data;

			void LoadFromJsonFile(const std::string& filePath)
			{
				std::cout << "LoadFromJsonFile ------------" << std::endl;

				// 从文件中读取配置
				std::ifstream file(filePath);
				json config;
				file >> config;
				// 提取雷达图元信息
				std::cout << "Radar ----------------------" << std::endl;
				for (const auto& radarSchema : config["radar"]["indicator"]) {
					Radar radar;
					radar.name = radarSchema["name"];
					std::cout << "  name: " << radar.name << std::endl;
					radar.maximum = radarSchema["max"];
					radars.push_back(radar);
				}

				std::cout << std::endl << "Data --------------------" << std::endl;
				// 提取数据
				for (const auto& radarData : config["series"]["data"]) {
					// 一组数据 -> 得到一个float（相对于1的比例值）
					std::string name = radarData["name"];
					std::vector<float> coordinates;
					// float pos;

					for (int i = 0; i < radars.size(); i++) {
						float pos = (float)(radarData["value"][i]) / (float)(radars[i].maximum); // 归一化的数据
						coordinates.push_back(pos);
					}

					data.push_back(std::make_pair(name, coordinates));
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

			int level; // 轴的精度

		public:	
			RadarChart() {
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
			void DrawPlot(osg::Vec3Array& vec, const double size = osg::WGS_84_RADIUS_EQUATOR / 100) {
				//精细度
				osg::TessellationHints* hints1 = new osg::TessellationHints();
				//设置精细度
				hints1->setDetailRatio(0.3f);

				for (size_t i = 0; i < vec.size(); ++i) {
					// osg::Vec3f sphereLoc = vec3ToSphere(vec[i]);
					osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(vec[i], size);
					osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints1);
					//pShapeDrawable->setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
					geode->addDrawable(pShapeDrawable.get());
				}
				auto states = geode->getOrCreateStateSet();
				states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
			}

			osg::Geode* MakeRadarBackground() {
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
				
				osg::ref_ptr<osg::Geode> background = new osg::Geode();

				background->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
				background->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				background->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0), osg::StateAttribute::ON);

				osg::Vec3f center = osg::Vec3f(0.5, 2.0, 0.0);
				std::vector<osg::ref_ptr<osg::Vec3Array>> circles;
					
				// 在地球表面的适当位置画出一系列的同心圆
				for (int count = 0; count < GetLevel(); count++) {
					osg::ref_ptr<osg::Geometry> circle = new osg::Geometry();
					// 设置颜色
					if (count == 0) {
						osg::ref_ptr<osg::Material> material = new osg::Material();
						material->setDiffuse(osg::Material::FRONT, osg::Vec4(82.0 / 255.0, 139.0 / 255.0, 139.0 / 255.0, 1.0)); // 设置漫射光颜色为绿色  
						circle->getOrCreateStateSet()->setAttribute(material);
					}
					else
					{
						osg::ref_ptr<osg::Material> material = new osg::Material();
						material->setDiffuse(osg::Material::FRONT, osg::Vec4(121.0 / 255.0, 205.0 / 255.0, 205.0 / 255.0, 1.0)); // 设置漫射光颜色为绿色  
						circle->getOrCreateStateSet()->setAttribute(material);
					}
					double y = 0.8 + count * (0.2 / (double)GetLevel());
					circles.push_back(new osg::Vec3Array());
					for (double i = 0.0; i <= 1.0; i = i + 0.01) {
						osg::Vec3f loc;
						loc.x() = i;
						loc.y() = y;
						loc.z() = 0.0;
						circles[count]->push_back(vec3ToSphere(loc));
					}
					circle->setVertexArray(circles[count].get());
					circle->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, circles[count]->size()));
					background->addDrawable(circle.get());
						
					if (count == 0) {
							
						for (int i = 0; i < GetDimension(); i++) {
							// 最外圈的文字标注
							osg::ref_ptr<osgText::Text> nameText = new osgText::Text;
							nameText->setFont("fonts/simhei.ttf");
							nameText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 20);
							nameText->setAxisAlignment(osgText::Text::SCREEN);
							nameText->setAutoRotateToScreen(true);
							nameText->setPosition(vec3ToSphere(osg::Vec3f((double)i / (double)GetDimension(), 0.7, 0.3)));
							nameText->setColor(osg::Vec4f(1.0, 1.0, 1.0, 1.0));
							nameText->setText(radars[i].name);
							background->addDrawable(nameText.get());

							osg::ref_ptr<osgText::Text> maxText = new osgText::Text;
							maxText->setFont("fonts/simhei.ttf");
							maxText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 20);
							maxText->setAxisAlignment(osgText::Text::SCREEN);
							maxText->setAutoRotateToScreen(true);
							maxText->setPosition(vec3ToSphere(osg::Vec3f((double)i / (double)GetDimension(), 0.8, 0.1)));
							maxText->setColor(osg::Vec4f(1.0, 1.0, 1.0, 1.0));
							maxText->setText(std::to_string((int)radars[i].maximum));
							background->addDrawable(maxText.get());
							// 从圆心朝各个方向连线
							osg::ref_ptr<osg::Geometry> line = new osg::Geometry();
							osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array();
							for (int j = 0; j <= 100; j++) {
								osg::Vec3f p;
								p.x() = (double)i / (double)GetDimension();
								p.y() = 0.8 + ((double)j / 100.0) * 0.2;
								p.z() = 0.0;
								points->push_back(vec3ToSphere(p));
							}
							line->setVertexArray(points.get());
							line->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, points->size()));
							osg::ref_ptr<osg::Material> color = new osg::Material();
							color->setDiffuse(osg::Material::FRONT, osg::Vec4(1.0, 1.0, 1.0, 1.0));
							line->getOrCreateStateSet()->setAttribute(color);
							osg::ref_ptr<osg::LineWidth> width = new osg::LineWidth();
							width->setWidth(2.0);
							line->getOrCreateStateSet()->setAttribute(width);
								
							background->addDrawable(line);
						}
					}
				}
				return background.release();
			}

			osg::Geode* MakeRadarChart() {
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
					
				int idx = 0;
				for (auto& grp : data) {
					osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array();
					for (int dim = 0; dim < GetDimension(); dim++) {
						/*
							找到每个离散点对应的三维坐标，其中
							经度（x）取决于维度编号
							纬度（y）取决于该维度的数值（data）
							高度（z）不同组别会处于不同的高度
						*/
							
						osg::Vec3f base = vec3ToSphere(osg::Vec3f(
							(double)dim / (double)GetDimension(),
							0.8 + 0.2 * (1.0 - grp.second[dim]), 0.0));
						osg::Vec3f point = base + osg::Vec3f(0.0, 0.0, 0.0);
						point.z() = (1.1 + (double)idx * 0.1) * 6.37825e+06;
						points->push_back(point);
						// 为每个数据点画虚线辅助线
						DrawGuides(base, point);
						// 文字标注
						osg::ref_ptr<osgText::Text> dataText = new osgText::Text;
						dataText->setFont("fonts/simhei.ttf");
						dataText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 20);
						dataText->setAxisAlignment(osgText::Text::SCREEN);
						dataText->setAutoRotateToScreen(true);
						dataText->setPosition(point);
						dataText->setColor(colors[idx]);
						dataText->setText(std::to_string((int)(radars[dim].maximum * grp.second[dim])));
						geode->addDrawable(dataText.get());
					}
					DrawPlot(*(points.get()));
					// 给每一组点连线
					osg::ref_ptr<osg::Geometry> graph = new osg::Geometry();
					graph->setVertexArray(points);
					graph->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, points->size()));
					geode->addDrawable(graph);

					// 颜色和宽度
					osg::ref_ptr<osg::Material> color = new osg::Material();
					color->setDiffuse(osg::Material::FRONT, colors[idx]);
					graph->getOrCreateStateSet()->setAttribute(color);
					osg::ref_ptr<osg::LineWidth> width = new osg::LineWidth();
					width->setWidth(4.0);
					graph->getOrCreateStateSet()->setAttribute(width);

					// 文字标注
					osg::ref_ptr<osgText::Text> nameText = new osgText::Text;
					nameText->setFont("fonts/simhei.ttf");
					nameText->setCharacterSize(osg::WGS_84_RADIUS_EQUATOR / 30);
					//nameText->setFontResolution(100, 100);
					nameText->setAxisAlignment(osgText::Text::SCREEN);
					nameText->setAutoRotateToScreen(true);
					// 找一个合适的位置
					// osg::Vec3f pos;
					osg::Vec3f pos1 = points->at(idx % GetDimension());
					osg::Vec3f pos2 = (idx % GetDimension()) == (GetDimension() - 1) ?
						points->at(0) : points->at(idx % GetDimension() + 1);
					nameText->setPosition((pos1 + pos2) / 2.0);
					nameText->setColor(colors[idx]);
					nameText->setText(data[idx].first);
					geode->addDrawable(nameText.get());


					idx++;
				}

				return geode.release();
			}

			void DrawGuides(osg::Vec3f from, osg::Vec3f to) {
				std::vector<osg::Vec3f> points;
				osg::Vec3f point = from;
				point.z() += 0.01 * 6.37825e+06;
				do {
					points.push_back(point);
					point.z() += 0.01 * 6.37825e+06;
				} while (point.z() < to.z());

				//精细度
				osg::TessellationHints* hints1 = new osg::TessellationHints();
				//设置精细度
				hints1->setDetailRatio(0.3f);

				for (size_t i = 0; i < points.size(); ++i) {
					// osg::Vec3f sphereLoc = vec3ToSphere(vec[i]);
					osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(points[i], osg::WGS_84_RADIUS_EQUATOR / 500);
					osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints1);
					//pShapeDrawable->setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
					geode->addDrawable(pShapeDrawable.get());
				}
				auto states = geode->getOrCreateStateSet();
				states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
			}

			int GetLevel() { return level; }
			void SetLevel(int _level) { level = _level; }

			int GetDimension() { return radars.size(); }

			std::vector<osg::Vec4f> colors;
			void addColors(int count) {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_real_distribution<float> dis(0.0, 1.0);

				for (int i = 0; i < count; i++) {
					colors.push_back(osg::Vec4f(dis(gen), dis(gen), dis(gen), 1.0f));
				}
			}

		}; // class RadarChart
	} // namespace InfoViser
} // namespace VIS4Earth


#endif