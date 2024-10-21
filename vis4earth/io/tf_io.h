#ifndef VIS4EARTH_IO_TF_IO_H
#define VIS4EARTH_IO_TF_IO_H

#include <fstream>

#include <vector>

namespace VIS4Earth
{
	namespace Loader
	{
		class TransferFunctionPoints
		{
		public:
			static std::vector<std::pair<uint8_t, std::array<float, 4>>> LoadFromFile(
				const std::string& filePath, std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath, std::ios::in);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return std::vector<std::pair<uint8_t, std::array<float, 4>>>();
				}

				std::string buf;
				std::vector<std::pair<uint8_t, std::array<float, 4>>> tfPnts;
				while (std::getline(is, buf)) {
					float scalar;
					std::array<float, 4> f4;
					auto validRead =
						sscanf(buf.c_str(), "%f%f%f%f%f", &scalar, &f4[0], &f4[1], &f4[2], &f4[3]);
					if (validRead != 5) continue;

					for (uint8_t rgba = 0; rgba < 4; ++rgba)
						f4[rgba] /= 255.f;
					tfPnts.emplace_back(std::make_pair(static_cast<uint8_t>(scalar), f4));
				}

				return tfPnts;
			}

			static bool DumpToFile(
				const std::string& filePath,
				const std::vector<std::pair<uint8_t, std::array<float, 4>>>& dat,
				std::string* errMsg = nullptr)
			{
				std::ofstream os(filePath, std::ios::out);
				if (!os.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return false;
				}

				for (auto itr = dat.begin(); itr != dat.end(); ++itr) {
					os << std::to_string(static_cast<float>(itr->first));
					os << ' ';
					os << std::to_string(itr->second[0] * 255.f);
					os << ' ';
					os << std::to_string(itr->second[1] * 255.f);
					os << ' ';
					os << std::to_string(itr->second[2] * 255.f);
					os << ' ';
					os << std::to_string(itr->second[3] * 255.f);
					os << '\n';
				}
				os.close();

				return true;
			}
		};

	}
}

#endif // !VIS4EARTH_IO_TF_IO_H
