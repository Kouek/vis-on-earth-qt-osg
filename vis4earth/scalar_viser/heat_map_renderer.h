#ifndef VIS4EARTH_SCALAR_VISER_HMR_H
#define VIS4EARTH_SCALAR_VISER_HMR_H

#include <string>

#include <map>

#include <osg/CoordinateSystemNode>
#include <osg/CullFace>
#include <osg/ShapeDrawable>
#include <osg/Texture1D>
#include <osg/Texture3D>

namespace VIS4Earth
{
	namespace ScalarViser
	{

		class HeatMap3DRenderer
		{
		private:
			struct PerRendererParam
			{
				osg::ref_ptr<osg::Group> grp;
				osg::ref_ptr<osg::Program> program;

				PerRendererParam()
				{
					grp = new osg::Group;

					osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
						osg::Shader::VERTEX,
						VIS4EARTH_SHADER_PREFIX
						"scalar_viser/hmp_vert_d.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT, 
						VIS4EARTH_SHADER_PREFIX
						"scalar_viser/hmp_frag_d.glsl");
					program = new osg::Program;
					program->addShader(vertShader);
					program->addShader(fragShader);
				}
			};
			PerRendererParam param;

			class PerVolParam
			{
			private:
				osg::ref_ptr<osg::Uniform> minLatitute;
				osg::ref_ptr<osg::Uniform> maxLatitute;
				osg::ref_ptr<osg::Uniform> minLongtitute;
				osg::ref_ptr<osg::Uniform> maxLongtitute;
				osg::ref_ptr<osg::Uniform> minHeight;
				osg::ref_ptr<osg::Uniform> maxHeight;
				osg::ref_ptr<osg::Uniform> height;
				osg::ref_ptr<osg::Uniform> volStartFromZeroLon;

				osg::ref_ptr<osg::ShapeDrawable> sphere;
				osg::ref_ptr<osg::Texture3D> volTex;
				osg::ref_ptr<osg::Texture1D> colTblTex;

			public:
				PerVolParam(osg::ref_ptr<osg::Texture3D> volTex, osg::ref_ptr<osg::Texture1D> colTblTex,
					PerRendererParam* renderer)
					: volTex(volTex), colTblTex(colTblTex)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					auto tessl = new osg::TessellationHints;
					tessl->setDetailRatio(10.f);
					sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), MinHeight), tessl);

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
					STATEMENT(height, .5f * (MinHeight + MaxHeight));
					STATEMENT(volStartFromZeroLon, 0);
#undef STATEMENT

					states->setTextureAttributeAndModes(0, volTex, osg::StateAttribute::ON);
					states->setTextureAttributeAndModes(1, colTblTex, osg::StateAttribute::ON);

					auto volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex");
					volTexUni->set(0);
					auto colTblTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "colTblTex");
					colTblTexUni->set(1);
					states->addUniform(volTexUni);
					states->addUniform(colTblTexUni);

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setAttributeAndModes(renderer->program, osg::StateAttribute::ON);
					states->setMode(GL_BLEND, osg::StateAttribute::ON);
					states->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
				}
				/*
				* ����: SetColorTable
				* ����: ���ø������ʱ����ɫӳ���
				* ����:
				* -- colTblTex: ����ͼ��ɫӳ���OSGһά����
				*/
				void SetColorTable(osg::ref_ptr<osg::Texture1D> colTblTex)
				{
					this->colTblTex = colTblTex;
					auto states = sphere->getOrCreateStateSet();
					states->setTextureAttributeAndModes(1, this->colTblTex, osg::StateAttribute::ON);
				}
				/*
				* ����: SetLongtituteRange
				* ����: ���ø������ʱ�ľ��ȷ�Χ����λΪ�Ƕȣ�
				* ����:
				* -- minLonDeg: ������Сֵ
				* -- maxLonDeg: �������ֵ
				* ����ֵ: ������Ĳ������Ϸ�������false�������óɹ�������true
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
				* ����: SetLatituteRange
				* ����: ���ø������ʱ��γ�ȷ�Χ����λΪ�Ƕȣ�
				* ����:
				* -- minLatDeg: γ����Сֵ
				* -- maxLatDeg: γ�����ֵ
				* ����ֵ: ������Ĳ������Ϸ�������false�������óɹ�������true
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
				* ����: SetHeightFromCenterRange
				* ����: ���ø������ʱ�ĸ߶ȣ������ģ���Χ
				* ����:
				* -- minH: �߶���Сֵ
				* -- maxH: �߶����ֵ
				* ����ֵ: ������Ĳ������Ϸ�������false�������óɹ�������true
				*/
				bool SetHeightFromCenterRange(float minH, float maxH)
				{
					if (minH < 0.f) return false;
					if (minH >= maxH) return false;

					minHeight->set(minH);
					maxHeight->set(maxH);
					height->set(.5f * (minH + maxH));

					sphere->setShape(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), minH));

					return true;
				}
				std::array<float, 2> GetHeightFromCenterRange() const
				{
					std::array<float, 2> ret;
					minHeight->get(ret[0]);
					maxHeight->get(ret[1]);
					return ret;
				}
				/*
				* ����: SetHeightFromCenter
				* ����: ���ø������ʱ�ĸ߶ȣ������ģ�
				* ����:
				* -- h: �߶�
				* ����ֵ: ������Ĳ������Ϸ�������false�������óɹ�������true
				*/
				bool SetHeightFromCenter(float h)
				{
					float minH, maxH;
					minHeight->get(minH);
					maxHeight->get(maxH);
					if (h < minH || h > maxH) return false;

					height->set(h);
					return true;
				}
				float GetHeightFromCenter() const
				{
					float ret;
					height->get(ret);
					return ret;
				}
				/*
				* ����: SetVolumeStartFromLongtituteZero
				* ����: ��ȫ��������X=0��Ӧ�ľ���Ϊ0�ȣ���Ҫ����ù���
				* ����:
				* -- flag: Ϊtrueʱ������ù��ܡ�Ϊfalseʱ���رոù���
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

				friend class HeatMap3DRenderer;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			HeatMap3DRenderer() {}

			/*
			* ����: GetGroup
			* ����: ��ȡ�û��������OSG�ڵ�
			* ����ֵ: OSG�ڵ�
			*/
			osg::Group* GetGroup() { return param.grp.get(); }
			/*
			* ����: AddVolume
			* ����: ��û���������һ����
			* ����:
			* -- name: ���������ơ���ͬ��������費ͬ����������
			* -- volTex: ���OSG��ά����
			* -- colTblTex: ����ͼ��ɫӳ���OSGһά����
			*/
			void AddVolume(const std::string& name, osg::ref_ptr<osg::Texture3D> volTex,
				osg::ref_ptr<osg::Texture1D> colTblTex)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.sphere);
					vols.erase(itr);
				}
				auto opt = vols.emplace(std::pair<std::string, PerVolParam>(name, PerVolParam(volTex, colTblTex, &param)));
				param.grp->addChild(opt.first->second.sphere);
			}
			/*
			* ����: GetVolumes
			* ����: ��ȡ������У����ڻ���ʱ�������������
			*/
			std::map<std::string, PerVolParam>& GetVolumes()
			{
				return vols;
			}
			/*
			* ����: GetVolume
			* ����: ��ȡ������У����ڻ���ʱ���������
			* ����:
			* -- name: �������
			* ����ֵ: ��Ļ�������
			*/
			PerVolParam* GetVolume(const std::string& name)
			{
				auto itr = vols.find(name);
				if (itr == vols.end())
					return nullptr;
				return &(itr->second);
			}
			/*
			* ����: GetVolumeNum
			* ����: ��ȡ�û���������������
			* ����ֵ: �������
			*/
			size_t GetVolumeNum() const
			{
				return vols.size();
			}
		};

	} // namespace ScalarViser
} // namespace VIS4Earth

#endif // !VIS4EARTH_SCALAR_VISER_HMR_H
