#ifndef VIS4EARTH_INFO_VISER_LC_H
#define VIS4EARTH_INFO_VISER_LC_H

#include <string>
#include <iostream>

#include <array>
#include <map>
#include <unordered_map>

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
		class CurveChart
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
				uint8_t rndrVertsBufIdx; // unused
				std::array<uint32_t, 3> volDim;
				osg::Vec3 voxSz;
				float isoVal;
				// 经纬度/高度 bounds
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				bool volStartFromLonZero; // 经度起点是否从零

				std::shared_ptr<std::vector<float>> volDat;
				std::shared_ptr<std::vector<osg::Vec3f>> volDiscreteDat;
				// added: 密集插值点（两点通过线段相连）
				std::vector<osg::Vec3f> denseInterPoints;
				// added to draw Cardinal
				double curveTension{0};
				double McMatrix[4][4];

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
				* 函数: GetConsecutiveVec
				* 功能: 得到连续数据的散点的三维球坐标
				* 参数:
				* -- eps : 误差
				* -- base : 基准值
				*/
				std::vector<osg::Vec3f> GetConsecutiveVec(
					std::array<uint32_t, 3> graphDim, std::array<uint8_t, 3> logDstDim,
					float eps = 0.011f, float base = 30.f / 255.f)
				{

					std::string errMsg;
					{
						auto volDimYxX = static_cast<size_t>(volDim[1]) * volDim[0];
						std::vector<osg::Vec3f> ret;
						std::unordered_map<int, osg::Vec3f> idxMap;
						std::array<int, 3> dstDim = { volDim[0], volDim[1], volDim[2] };

						std::array<float, 3> scaleDst2Graph = { static_cast<float>(graphDim[0] - 1) / (dstDim[0] - 1),
										static_cast<float>(graphDim[1] - 1) / (dstDim[1] - 1),
										static_cast<float>(graphDim[2] - 1) / (dstDim[2] - 1) };
						std::array<float, 3> scaleGraph2Dst = { static_cast<float>(dstDim[0] - 1) / (graphDim[0] - 1),
										static_cast<float>(dstDim[1] - 1) / (graphDim[1] - 1),
										static_cast<float>(dstDim[2] - 1) / (graphDim[2] - 1) };

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
						auto XYZ2Offs = [&](int x, int y, int z) {
							//return static_cast<size_t>(z) * graphDim[1] * graphDim[0] + y * graphDim[0] + x;
							return static_cast<size_t>(z) * dstDim[1] * dstDim[0] + y * dstDim[0] + x;
							};

						auto graph2Offs = [&](int x, int y, int z) {
							return static_cast<size_t>(z) * graphDim[1] * graphDim[0] + y * graphDim[0] + x;
							};

						// for (size_t i = 0; i < volDat->size(); ++i) {
						// 	if (volDat->at(i) < (base - eps) || volDat->at(i) > (base + eps) ) continue;
						// 	int z = i / volDimYxX;
						// 	int y = (i - z * volDimYxX) / volDim[0];
						// 	int x = i - z * volDimYxX - y * volDim[0];

						// 	osg::Vec3f pointer1 = osg::Vec3f(x, y, z);
						// 	osg::Vec3f volLoc = osg::Vec3f(x * voxSz.x(), y * voxSz.y(), z * voxSz.z());
						// 	osg::Vec3f sphereLoc = vec3ToSphere(volLoc);

						// 	ret.push_back(sphereLoc);
						// }

						int maxSub = 0;
						int maxX = 0;

						for (int z = 0; z < dstDim[2]; ++z) {
							for (int y = 0; y < dstDim[1]; ++y) {
								for (int x = 0; x < dstDim[0]; ++x) {

									auto offs = XYZ2Offs(x, y, z);
									if (volDat->at(offs) < (base - eps) || volDat->at(offs) > (base + eps)) continue;
									maxX = std::max(maxX, x);
									auto graphZ = static_cast<int>(z * scaleDst2Graph[2]);
									auto graphY = static_cast<int>(y * scaleDst2Graph[1]);
									auto graphX = static_cast<int>(x * scaleDst2Graph[0]);
									if (graphX >= graphDim[0])
										graphX = graphDim[0] - 1;
									if (graphY >= graphDim[1])
										graphY = graphDim[1] - 1;
									if (graphZ >= graphDim[2])
										graphZ = graphDim[2] - 1;
									maxSub = std::max(maxSub, graphX);
									// osg::Vec3f volLoc = osg::Vec3f(x * voxSz.x(), y * voxSz.y(), z * voxSz.z());
									// osg::Vec3f sphereLoc = vec3ToSphere(volLoc);
									// ret.push_back(sphereLoc);

									if (graphX != 0)
										--graphX;
									if (graphY != 0)
										--graphY;
									if (graphZ != 0)
										--graphZ;
									auto graphX1 = graphX == graphDim[0] - 1 ? graphX : graphX + 1;
									auto graphY1 = graphY == graphDim[1] - 1 ? graphY : graphY + 1;
									auto graphZ1 = graphZ == graphDim[2] - 1 ? graphZ : graphZ + 1;


									for (int subZ = graphZ; subZ <= graphZ1; ++subZ)
										for (int subY = graphY; subY <= graphY1; ++subY)
											for (int subX = graphX; subX <= graphX1; ++subX) {

												//auto offs = XYZ2Offs(subX, subY, subZ);

												auto graphOffs = graph2Offs(subX, subY, subZ);
												osg::Vec3f volLoc = osg::Vec3f(subX * scaleGraph2Dst[0] * voxSz.x(), subY * scaleGraph2Dst[1] * voxSz.y(), subZ * scaleGraph2Dst[2] * voxSz.z());
												osg::Vec3f sphereLoc = vec3ToSphere(volLoc);
												if (idxMap.find(graphOffs) == idxMap.end()) {
													idxMap[graphOffs] = sphereLoc;
													ret.push_back(sphereLoc);
												}

											}
								}
							}
						}
						/*std::cout << "maxX: " << maxX << std::endl;
						std::cout << "maxSub: " << maxSub << std::endl;*/
						/*osg::Vec3f volLoc = osg::Vec3f((graphDim[0] - 1) * scaleGraph2Dst[0] * voxSz.x(), (graphDim[1] - 1) * scaleGraph2Dst[1] * voxSz.y(), (graphDim[2] - 1) * scaleGraph2Dst[2] * voxSz.z());
						osg::Vec3f sphereLoc = vec3ToSphere(volLoc);
						ret.push_back(sphereLoc);*/
						return ret;
					}
				ERR:
					std::cerr << errMsg << std::endl;
					return std::vector<osg::Vec3f>();
				}
				
				// added
				// 计算MC矩阵
				void calMcMatrix()
				{
					double s = (1 - curveTension) / 2;
					
					McMatrix[0][0] = -s, McMatrix[0][1] = 2 - s, McMatrix[0][2] = s - 2, McMatrix[0][3] = s;
					McMatrix[1][0] = 2 * s, McMatrix[1][1] = s - 3, McMatrix[1][2] = 3 - 2 * s, McMatrix[1][3] = -s;
					McMatrix[2][0] = -s, McMatrix[2][1] = 0, McMatrix[2][2] = s, McMatrix[2][3] = 0;
					McMatrix[3][0] = 0, McMatrix[3][1] = 1, McMatrix[3][2] = 0, McMatrix[3][3] = 0;
				}
				// added
				// 画Cardinal曲线
				void drawCardinal(std::vector<osg::Vec3f>& points)
				{
					// 当样条插值的样本点总数小于4时，无法画Cardinal曲线
					if (points.size() < 4)
					{
						return;
					}

					// denseInterPoints = new std::vector<osg::Vec3f>;
					// denseInterPoints = std::make_shared<std::vector<osg::Vec3f>>();  
					// 先将点集中的第二个记入插值点，作为起点
					auto iter = points.begin();
					++iter;
					denseInterPoints.push_back(*iter);
					--iter;
					// 找到点集中的倒数第三个
					auto endIter = points.end();
					int nIndex = 0;
					while (true)
					{
						--endIter;
						++nIndex;
						if (nIndex == 3)
						{
							break;
						}
					}
					// 计算Mc矩阵
					calMcMatrix();
					// 计算每个样条的方程，并拟合每一段样条

					for (; iter != endIter; ++iter)
					{
						auto& p0 = *iter;
						auto& p1 = *(++iter);
						auto& p2 = *(++iter);
						auto& p3 = *(++iter);

						--iter;
						--iter;
						--iter;

						std::vector<osg::Vec3f>vtTempPoint;
						vtTempPoint.push_back(p0);
						vtTempPoint.push_back(p1);
						vtTempPoint.push_back(p2);
						vtTempPoint.push_back(p3);

						for (auto i = 0; i < 4; ++i)
						{
							vtTempPoint[i] = p0 * McMatrix[i][0] + p1 * McMatrix[i][1] + p2 * McMatrix[i][2] + p3 * McMatrix[i][3];
						}

						float t3, t2, t1, t0;
						for (double t = 0.0; t < 1; t += 0.01)
						{
							t3 = t * t * t; t2 = t * t; t1 = t; t0 = 1;
							osg::Vec3f newPoint;
							newPoint = vtTempPoint[0] * t3 + vtTempPoint[1] * t2 + vtTempPoint[2] * t1 + vtTempPoint[3] * t0;
							denseInterPoints.push_back(newPoint);
						}
					}
					return;
				}
				
				// changed
				/*
				* 函数: MakeCurveChart
				* 功能: 绘制曲线图
				*/
				osg::Geode* MakeCurveChart(
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
					// 

					// 开始画
					drawCardinal(locaPoint);


					// 设置
					// 折线图信息
					osg::ref_ptr<osg::Geode> lcGeode = new osg::Geode();
					// osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
					osg::ref_ptr<osg::Geometry> lGeom = new osg::Geometry();

					// 创建顶点数组
					osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
					// 创建颜色数组
					osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

					for (size_t i = 1; i < denseInterPoints.size(); ++i) {
						// 点
						osg::Vec3f sphereLoc0, sphereLoc1;
						sphereLoc0 = vec3ToSphere(denseInterPoints[i - 1]);
						sphereLoc1 = vec3ToSphere(denseInterPoints[i]);
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
				* 函数: MakeCurveChart
				* 功能: 绘制曲线图
				*/
				osg::Geode* MakeCurveChart(
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

					// added

					// 开始画
					drawCardinal(locaPoint);

					// 设置
					// 折线图信息
					osg::ref_ptr<osg::Geode> lcGeode = new osg::Geode();
					// osg::ref_ptr<osg::Geometry> pGeom = new osg::Geometry();
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
							// vertices->push_back(sphereLoc0);
							// colors->push_back(lcColor);
						}
						// vertices->push_back(sphereLoc1);
						// colors->push_back(lcColor);
					}

					for (size_t i = 1; i < denseInterPoints.size(); ++i) {
						// 点
						osg::Vec3f sphereLoc0, sphereLoc1;
						sphereLoc0 = vec3ToSphere(denseInterPoints[i - 1]);
						sphereLoc1 = vec3ToSphere(denseInterPoints[i]);
						// osg::ref_ptr<osg::Sphere> pSphereShape = new osg::Sphere(sphereLoc1, osg::WGS_84_RADIUS_EQUATOR / 100);
						// osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get(), hints);
						// pShapeDrawable->setColor(lcColor);
						// lcGeode->addDrawable(pShapeDrawable);

						if (i == 1) {
							// osg::ref_ptr<osg::Sphere> pSphereShape0 = new osg::Sphere(sphereLoc0, osg::WGS_84_RADIUS_EQUATOR / 100);
							// osg::ref_ptr<osg::ShapeDrawable> pShapeDrawable0 = new osg::ShapeDrawable(pSphereShape0.get(), hints);
							// pShapeDrawable0->setColor(lcColor);
							// lcGeode->addDrawable(pShapeDrawable0);
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
					std::cout << "原点（球）：" << axisOrigin.x() << " " << axisOrigin.y() << " " << axisOrigin.z() << std::endl;
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

				friend class CurveChart;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			CurveChart() {}


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

		}; // class CurveChart
	} // namespace InfoViser
} // namespace VIS4Earth


#endif