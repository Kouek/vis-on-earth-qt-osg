#ifndef VIS4EARTH_SCALAR_VISER_DIRECT_VOLUME_RENDERER_H
#define VIS4EARTH_SCALAR_VISER_DIRECT_VOLUME_RENDERER_H

#include <string>

#include <map>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/ShapeDrawable>
#include <osg/Texture1D>
#include <osg/Texture3D>

#include <vis4earth/callback.h>

namespace VIS4Earth
{
	namespace ScalarViser
	{

		class DirectVolumeRenderer
		{
		private:
			struct PerRendererParam
			{
				osg::ref_ptr<osg::Group> grp;
				osg::ref_ptr<osg::Program> program;

				osg::ref_ptr<osg::Uniform> eyePos;
				osg::ref_ptr<osg::Uniform> sliceCntr;
				osg::ref_ptr<osg::Uniform> sliceDir;
				osg::ref_ptr<osg::Uniform> dt;
				osg::ref_ptr<osg::Uniform> maxStepCnt;
				osg::ref_ptr<osg::Uniform> useSlice;

				class Callback : public osg::NodeCallback
				{
				private:
					osg::ref_ptr<osg::Uniform> eyePos;

				public:
					Callback(osg::ref_ptr<osg::Uniform> eyePos) : eyePos(eyePos)
					{}
					virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
					{
						auto _eyePos = nv->getEyePoint();
						eyePos->setUpdateCallback(new UniformUpdateCallback<osg::Vec3>(_eyePos));

						traverse(node, nv);
					}
				};

				PerRendererParam()
				{
					grp = new osg::Group;

					osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
						osg::Shader::VERTEX,
						VIS4EARTH_SHADER_PREFIX
						"scalar_viser/dvr_vert_d.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT,
						VIS4EARTH_SHADER_PREFIX
						"scalar_viser/dvr_frag_d.glsl");
					program = new osg::Program;
					program->addShader(vertShader);
					program->addShader(fragShader);

#define STATEMENT(name, val) name = new osg::Uniform(#name, val)
					STATEMENT(eyePos, osg::Vec3());
					STATEMENT(dt, static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * .008f);
					STATEMENT(maxStepCnt, 100);
					STATEMENT(useSlice, 0);
					STATEMENT(sliceCntr, osg::Vec3());
					STATEMENT(sliceDir, osg::Vec3());
#undef STATEMENT

					grp->setCullCallback(new Callback(eyePos));
				}
			};
			PerRendererParam param;

			struct PerVolParam
			{
				bool isDisplayed;

				osg::ref_ptr<osg::Uniform> minLatitute;
				osg::ref_ptr<osg::Uniform> maxLatitute;
				osg::ref_ptr<osg::Uniform> minLongtitute;
				osg::ref_ptr<osg::Uniform> maxLongtitute;
				osg::ref_ptr<osg::Uniform> minHeight;
				osg::ref_ptr<osg::Uniform> maxHeight;
				osg::ref_ptr<osg::Uniform> volStartFromZeroLon;

				osg::ref_ptr<osg::ShapeDrawable> sphere;
				osg::ref_ptr<osg::Texture3D> volTex;
				osg::ref_ptr<osg::Texture1D> tfTex;

				PerVolParam(osg::ref_ptr<osg::Texture3D> volTex, osg::ref_ptr<osg::Texture1D> tfTex,
					PerRendererParam* renderer)
					: volTex(volTex), tfTex(tfTex)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					auto tessl = new osg::TessellationHints;
					tessl->setDetailRatio(10.f);
					sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), MaxHeight), tessl);

					auto states = sphere->getOrCreateStateSet();
#define STATEMENT(name, val)                                                                       \
    name = new osg::Uniform(#name, val);                                                           \
    states->addUniform(name)
					STATEMENT(minLatitute, deg2Rad(-10.f));
					STATEMENT(maxLatitute, deg2Rad(+10.f));
					STATEMENT(minLongtitute, deg2Rad(-20.f));
					STATEMENT(maxLongtitute, deg2Rad(+20.f));
					STATEMENT(minHeight, MinHeight);
					STATEMENT(maxHeight, MaxHeight);
					STATEMENT(volStartFromZeroLon, 0);
#undef STATEMENT
					states->addUniform(renderer->eyePos);
					states->addUniform(renderer->dt);
					states->addUniform(renderer->maxStepCnt);
					states->addUniform(renderer->useSlice);
					states->addUniform(renderer->sliceCntr);
					states->addUniform(renderer->sliceDir);

					states->setTextureAttributeAndModes(0, volTex, osg::StateAttribute::ON);
					states->setTextureAttributeAndModes(1, tfTex, osg::StateAttribute::ON);

					auto volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex");
					volTexUni->set(0);
					auto tfTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "tfTex");
					tfTexUni->set(1);
					states->addUniform(volTexUni);
					states->addUniform(tfTexUni);

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setAttributeAndModes(renderer->program, osg::StateAttribute::ON);
					states->setMode(GL_BLEND, osg::StateAttribute::ON);
					states->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
				}
				/*
				* 函数: SetTransferFunction
				* 功能: 设置该体绘制时的传输函数
				* 参数:
				* -- tfTex: 体的传输函数的OSG一维纹理
				*/
				void SetTransferFunction(osg::ref_ptr<osg::Texture1D> tfTex)
				{
					this->tfTex = tfTex;
					auto states = sphere->getOrCreateStateSet();
					states->setTextureAttributeAndModes(1, this->tfTex, osg::StateAttribute::ON);
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

					minLongtitute->set(deg2Rad(minLonDeg));
					maxLongtitute->set(deg2Rad(maxLonDeg));
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

					minLatitute->set(deg2Rad(minLatDeg));
					maxLatitute->set(deg2Rad(maxLatDeg));
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

					minHeight->set(minH);
					maxHeight->set(maxH);

					sphere->setShape(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), maxH));
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
					if (flag) volStartFromZeroLon->set(1);
					else volStartFromZeroLon->set(0);
				}

			private:
				float deg2Rad(float deg)
				{
					return deg * osg::PI / 180.f;
				};

				friend class DirectVolumeRenderer;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			DirectVolumeRenderer()
			{}

			/*
			* 函数: GetGroup
			* 功能: 获取该绘制组件的OSG节点
			* 返回值: OSG节点
			*/
			osg::Group* GetGroup()
			{
				return param.grp.get();
			}
			/*
			* 函数: AddVolume
			* 功能: 向该绘制组件添加一个体
			* 参数:
			* -- name: 添加体的名称。不同体的名称需不同，用于区分
			* -- volTex: 体的OSG三维纹理
			* -- tfTex: 体的传输函数的OSG一维纹理
			* -- isDisplayed: 为true时，体被加入后会被绘制。否则体只被加入绘制组件，但不会被绘制
			*/
			void AddVolume(const std::string& name, osg::ref_ptr<osg::Texture3D> volTex,
				osg::ref_ptr<osg::Texture1D> tfTex, bool isDisplayed = true)
			{
				auto itr = vols.find(name);
				if (itr != vols.end() && itr->second.isDisplayed) {
					param.grp->removeChild(itr->second.sphere);
					vols.erase(itr);
				}
				auto opt = vols.emplace(std::pair<std::string, PerVolParam>(name, PerVolParam(volTex, tfTex, &param)));

				opt.first->second.isDisplayed = isDisplayed;
				if (isDisplayed)
					param.grp->addChild(opt.first->second.sphere);
			}
			/*
			* 函数: DisplayVolume
			* 功能: 绘制该绘制组件中的一个体，位于组件中的其他体将不被绘制。一般用于产生体动画。当输入的体不在该组件中时，所有体都不会被绘制。
			* 参数:
			* -- name: 体的名称
			*/
			void DisplayVolume(const std::string& name)
			{
				for (auto itr = vols.begin(); itr != vols.end(); ++itr) {
					if (itr->first == name) {
						itr->second.isDisplayed = true;
						param.grp->addChild(itr->second.sphere);
					}
					else if (itr->second.isDisplayed == true) {
						itr->second.isDisplayed = false;
						param.grp->removeChild(itr->second.sphere);
					}
				}
			}
			/*
			* 函数: GetVolumes
			* 功能: 获取该组件中，体在绘制时所需的所有数据
			*/
			std::map<std::string, PerVolParam>& GetVolumes()
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
			PerVolParam* GetVolume(const std::string& name)
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
			size_t GetVolumeNum() const
			{
				return vols.size();
			}
			/*
			* 函数: SetDeltaT
			* 功能: 设置体绘制时，光线传播的步长
			* 参数:
			* -- dt: 光线传播的步长
			*/
			void SetDeltaT(float dt)
			{
				param.dt->set(dt);
			}
			float GetDeltaT() const
			{
				float ret;
				param.dt->get(ret);
				return ret;
			}
			/*
			* 函数: SetMaxStepCount
			* 功能: 设置体绘制时，光线传播的最大步数
			* 参数:
			* -- maxStepCnt: 光线传播的最大步数
			*/
			void SetMaxStepCount(int maxStepCnt)
			{
				param.maxStepCnt->set(maxStepCnt);
			}
			int GetMaxStepCount() const
			{
				int ret;
				param.maxStepCnt->get(ret);
				return ret;
			}
			/*
			* 函数: SetSlicing
			* 功能: 设置体绘制中的切面
			* 参数:
			* -- cntr: 切面的中心点（旋转时的参考点），位于体局部，三分量取值范围为[0, 1]
			* -- dir: 切面的单位法向量，位于球局部坐标
			*/
			void SetSlicing(const osg::Vec3& cntr, const osg::Vec3& dir)
			{
				param.useSlice->set(1);
				param.sliceCntr->set(cntr);
				param.sliceDir->set(dir);
			}
			void DisableSlicing()
			{
				param.useSlice->set(0);
			}
		};

	} // namespace ScalarViser
} // namespace SciVis

#endif // !VIS4EARTH_SCALAR_VISER_DIRECT_VOLUME_RENDERER_H
 