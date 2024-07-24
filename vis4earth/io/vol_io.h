#ifndef VIS4EARTH_IO_VOL_IO_H
#define VIS4EARTH_IO_VOL_IO_H

#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <iostream>

#include <array>
#include <vector>
#include <unordered_set>

#include <osg/Vec3>

namespace VIS4Earth
{
	namespace Loader
	{
		class RAWVolume
		{
		public:
			static std::vector<uint8_t> LoadU8FromFile(
				const std::string& filePath, const std::array<uint32_t, 3>& dim,
				std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath, std::ios::in | std::ios::binary | std::ios::ate);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return std::vector<uint8_t>();
				}

				auto voxNum = static_cast<size_t>(is.tellg()) / sizeof(uint8_t);
				{
					auto _voxNum = (size_t)dim[0] * dim[1] * dim[2];
					if (voxNum < _voxNum) {
						if (errMsg)
							*errMsg = "File Size is Smaller than Volume Size";
						is.close();
						return std::vector<uint8_t>();
					}
					voxNum = std::min(voxNum, _voxNum);
				}

				std::vector<uint8_t> dat(voxNum);
				is.seekg(0);
				is.read(reinterpret_cast<char*>(dat.data()), sizeof(uint8_t) * voxNum);
				is.close();

				return dat;
			}

			static bool DumpToFile(
				const std::string& filePath, const std::vector<uint8_t>& dat,
				std::string* errMsg = nullptr)
			{
				std::ofstream os(filePath, std::ios::out | std::ios::binary);
				if (!os.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return false;
				}

				os.write(reinterpret_cast<const char*>(dat.data()), sizeof(uint8_t) * dat.size());
				os.close();

				return true;
			}
		};

		class TXTVolume
		{
		public:
			static std::vector<float> LoadFromFile(
				const std::string& filePath, const std::array<uint32_t, 3>& dim,
				float nullVal, std::string* errMsg = nullptr)
			{
			}
			/* 读取txt点云文件 */
			static std::vector<osg::Vec3f> LoadFromFile(const std::string& filePath, std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return std::vector<osg::Vec3f>();
				}

				std::string line;
				std::vector<osg::Vec3f> ret;
				int flag = 0;
				while (std::getline(is, line)) {
					/*if (flag == 0) {
						flag++;
						continue;
					}*/
					float x, y, z;
					std::stringstream ss(line);
					ss >> x;
					ss >> y;
					ss >> z;
					osg::Vec3f point(x, y, z);
					ret.push_back(point);
					//flag--;
				}
				return ret;

			}
			///* ��ȡtxt��״ͼ�ļ� */
			//static std::vector<std::pair<std::wstring, float>> LoadFromPieFile(const std::string& filePath, std::string* errMsg = nullptr)
			//{
			//	std::wifstream is(filePath);
			//	if (!is.is_open()) {
			//		if (errMsg)
			//			*errMsg = "Invalid File Path";
			//		return std::vector<std::pair<std::wstring, float>>();
			//	}

			//	std::wstring line;
			//	std::vector<std::pair<std::wstring, float>> ret;
			//	int flag = 0;
			//	while (std::getline(is, line)) {
			//		/*if (flag == 0) {
			//			flag++;
			//			continue;
			//		}*/
			//		std::wstring label;
			//		float proportion;
			//		std::wstringstream ss(line);
			//		ss >> label;
			//		ss >> proportion;

			//		std::wcout << label << L" " << proportion << std::endl;
			//		std::wcout << line << std::endl;


			//		ret.push_back(std::make_pair(label, proportion));
			//		//flag--;
			//	}
			//	return ret;

			//}
		};

		class LabeledTXTVolume
		{
		public:
			bool isDense;
			std::array<uint32_t, 3> dim;
			std::array<float, 2> valRng;
			std::array<float, 2> lonRng;
			std::array<float, 2> latRng;
			std::array<float, 2> hRng;
			std::vector<float> dat;

			void Normalize(const std::array<float, 2>* valRng = nullptr)
			{
				if (valRng && (*valRng)[1] > (*valRng)[0])
					this->valRng = *valRng;

				auto rngWid = this->valRng[1] - this->valRng[0];
				for (auto itr = dat.begin(); itr != dat.end(); ++itr) {
					if (isnan(*itr))
						*itr = this->valRng[0];
					else
					{
						*itr = (*itr - this->valRng[0]) / rngWid;
						if (*itr < 0.f)
							*itr = 0.f;
					}
				}

				this->valRng[0] = 0.f;
				this->valRng[1] = 1.f;
			}

		public:
			static LabeledTXTVolume LoadFromFile(
				const std::string& filePath, std::string* errMsg = nullptr)
			{
				LabeledTXTVolume ret;

				std::ifstream is(filePath, std::ios::in);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return ret;
				}

				auto computeMinMax = [](std::array<float, 2>& rng, float val) {
					if (rng[0] > val)
						rng[0] = val;
					if (rng[1] < val)
						rng[1] = val;
					};

				std::unordered_set<float> hSets, latSets, lonSets;
				std::string buf;
				std::getline(is, buf); // 跳过第一行
				while (std::getline(is, buf)) {
					std::array<float, 5> f5;

					auto validRead = sscanf(buf.c_str(), "%f%f%f%f%f",
						&f5[0], &f5[1], &f5[2], &f5[3], &f5[4]);
					if (validRead < 4) continue;

					hSets.emplace(f5[1]);
					latSets.emplace(f5[2]);
					lonSets.emplace(f5[3]);

					computeMinMax(ret.hRng, f5[1]);
					computeMinMax(ret.latRng, f5[2]);
					computeMinMax(ret.lonRng, f5[3]);
					if (validRead == 5)
						computeMinMax(ret.valRng, f5[4]);
					else
						f5[4] = std::numeric_limits<float>::quiet_NaN();

					ret.dat.emplace_back(f5[4]);
				}

				ret.dim[2] = hSets.size();
				ret.dim[1] = latSets.size();
				ret.dim[0] = lonSets.size();
				ret.isDense = ret.dat.size() ==
					static_cast<size_t>(ret.dim[2]) * ret.dim[1] * ret.dim[0];

				return ret;
			}

		private:
			LabeledTXTVolume()
			{
				dim[0] = dim[1] = dim[2] = 0;
				valRng[0] = lonRng[0] = latRng[0] = hRng[0]
					= std::numeric_limits <float>::max();
				valRng[1] = lonRng[1] = latRng[1] = hRng[1]
					= std::numeric_limits <float>::min();
			}
		};
	}

	namespace Convertor
	{
		class RAWVolume
		{
		public:
			static std::vector<float> U8ToNormalizedFloat(const std::vector<uint8_t>& u8Dat)
			{
				std::vector<float> dat(u8Dat.size());
				size_t i = 0;
				for (size_t i = 0; i < u8Dat.size(); ++i)
					dat[i] = u8Dat[i] / 255.f;
				return dat;
			}

			static std::vector<uint8_t> NormalizedFloatToU8(const std::vector<float>& fDat)
			{
				std::vector<uint8_t> dat(fDat.size());
				size_t i = 0;
				for (size_t i = 0; i < fDat.size(); ++i)
					dat[i] = fDat[i] * 255.f;
				return dat;
			}
		};
	}
}

#endif // !VIS4EARTH_IO_VOL_IO_H
