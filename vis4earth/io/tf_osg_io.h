#ifndef VIS4EARTH_IO_TF_OSG_IO_H
#define VIS4EARTH_IO_TF_OSG_IO_H

#include <array>
#include <vector>

#include <osg/Texture1D>

namespace VIS4Earth
{
	namespace OSGConvertor
	{
		class TransferFunctionPoints
		{
		public:
			static osg::ref_ptr<osg::Texture1D> ToTexture(
				const std::vector<std::pair<uint8_t, std::array<float, 4>>>& tfPnts) {
				osg::ref_ptr<osg::Image> img = new osg::Image;
				img->allocateImage(256, 1, 1, GL_RGBA, GL_FLOAT);
				img->setInternalTextureFormat(GL_RGBA);

				auto pntItr = tfPnts.begin();
				auto lftPntItr = pntItr;
				auto lft2Rht = 1.f;
				auto* pxPtr = reinterpret_cast<osg::Vec4*>(img->data());

				for (int i = 0; i < 256; ++i) {
					auto assign = [&](float t) {
						pxPtr[i].r() = (1.0 - t) * lftPntItr->second[0] + t * pntItr->second[0];
						pxPtr[i].g() = (1.0 - t) * lftPntItr->second[1] + t * pntItr->second[1];
						pxPtr[i].b() = (1.0 - t) * lftPntItr->second[2] + t * pntItr->second[2];
						pxPtr[i].a() = (1.0 - t) * lftPntItr->second[3] + t * pntItr->second[3];
						};

					if (pntItr == tfPnts.end())
						assign(1.f);
					else if (i == static_cast<int>(pntItr->first)) {
						assign(1.f);
						lftPntItr = pntItr;
						++pntItr;
						if (pntItr != tfPnts.end())
							lft2Rht = pntItr->first - lftPntItr->first;
						else
							lft2Rht = 1.f;
					}
					else
						assign((i - lftPntItr->first) / lft2Rht);
				}

				osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
				tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode::LINEAR);
				tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode::NEAREST);
				tex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP);
				tex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
				tex->setImage(img);

				return tex;
			}
		};
	}
}

#endif // !VIS4EARTH_IO_TF_OSG_IO_H
