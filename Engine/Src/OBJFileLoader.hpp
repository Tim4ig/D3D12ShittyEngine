
#pragma once

#ifndef __cplusplus
#error C++ Only
#endif // !__cplusplus

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

namespace Loader {


	template<class T>
	struct Vec3 {
		T x, y, z;
		Vec3(T i = 0) : x(i), y(i), z(i)		{}
	};

	template<class T>
	struct Vec4 : public Vec3<T> {
		T w;
		Vec4(T i = 0) : w(i) {}
	};

	using Vec3f = Vec3<float>;
	using Vec4f = Vec4<float>;
	using Vec3i = Vec3<int>;
	using Vec4i = Vec4<int>;

	struct Ind
	{
		Vec3i points[3];
	};

	struct Mesh {
		std::string								Name;
		std::vector<Vec4f>						Coords;
		std::vector<Vec3f>						Normals;
		std::vector<Vec3f>						UVs;
		std::vector<Ind>						Indecies;
	};



	struct FileData {
		std::vector<Mesh> FileMeshes;
	};

	inline [[nodiscard]] bool LoadObj(std::wstring wFileName, FileData* pFileData) noexcept;

}

namespace Loader {

	namespace sys {

		enum Key {
			error,
			comment,
			o,
			v,
			vn,
			vt,
			f,
		};

		const std::pair<const char*, Key> KeyMap[] = {
			{"#",			comment	},
			{"o",			o		},
			{"vn",			vn		},
			{"vt",			vt		},
			{"v",			v		},
			{"f",			f		},
		};

		sys::Key ParseKey(std::string string) {
			for (auto key : KeyMap) {
				if (string.find(key.first) != std::string::npos) 
					return key.second;
			}
			return Key::error;
		}

		std::vector<Mesh> ParseMesh(std::string first, std::istringstream& stream) {

			std::vector<Mesh> out;
			char temp = 0;
			char indexType = 0;
			Key key = Key::error;

			int i = 0;
			while (1 && i++ < 256) {
				Mesh mesh;
				key = ParseKey(first);

				std::istringstream parser;

				if (key == Key::o) {
					mesh.Name.assign(&first[2]);
					std::getline(stream, first);
					key = ParseKey(first);
		
				}

				parser = std::istringstream(first);

				while (!stream.eof()) {

					switch (key)
					{
					case Loader::sys::v:
					{
						Vec4f vec;
						parser >> temp >> vec.x >> vec.y >> vec.z >> vec.w;
						if (parser.fail()) vec.w = 1;
						mesh.Coords.push_back(vec);
						break;
					}
					case Loader::sys::vn:
					{
						Vec4f vec;
						parser >> temp >> vec.x >> vec.y >> vec.z >> vec.w;
						mesh.Normals.push_back(vec);
						break;
					}
					case Loader::sys::vt:
					{
						Vec4f vec;
						parser >> temp >> vec.x >> vec.y >> vec.z >> vec.w;
						mesh.UVs.push_back(vec);
						break;
					}
					case Loader::sys::f:
					{

						if (!indexType) {
							if (first.find("//") != std::string::npos) indexType = 3;
							else if (first.find("/") != std::string::npos) {
								size_t slashCount = std::count(first.begin(), first.end(), '/');
								if (slashCount == 3) indexType = 2;
								else indexType = 4;
							}
							else indexType = 1;
						}

						Ind index;
						parser >> temp;
						for (int i = 0; i < 3; ++i) {
							if (indexType == 4) parser >> index.points[i].x >> temp >> index.points[i].y >> temp >> index.points[i].z;
							if (indexType == 3) parser >> index.points[i].x >> temp >> temp >> index.points[i].z;
							if (indexType == 2) parser >> index.points[i].x >> temp >> index.points[i].y;
							if (indexType == 1) parser >> index.points[i].x;
							mesh.Indecies.push_back(index);

						}
						break;
					}
					}

					std::getline(stream, first);
					parser = std::istringstream(first);
					key = ParseKey(first);
					if (key == Key::o) 
						break;
				}
				indexType = 0;
				out.push_back(mesh);

				if (key != Key::o) break;
			}

			return out;
		}
	}

	bool Loader::LoadObj(std::wstring wFileName, FileData* pFileData) noexcept
	{

		std::ifstream		File;
		std::string			RawData;
		std::istringstream	RawDataStream;

		File.open("d/1.obj");
		if (!File.is_open()) return false;

		File.seekg(0, File.end);
		size_t fileSize = File.tellg();
		File.seekg(0, File.beg);

		
		RawData.resize(fileSize, '\n');
		File.read(const_cast<char*> (RawData.c_str()), fileSize);
		File.close();

		RawDataStream = std::istringstream(RawData);


		while (!RawDataStream.eof()) {
			std::getline(RawDataStream, RawData);
			sys::Key key = sys::ParseKey(RawData);
			
			if (key == sys::Key::v || key == sys::Key::o) {
				pFileData->FileMeshes = sys::ParseMesh(RawData, RawDataStream);
				return !!pFileData->FileMeshes.size();
			}
		}

		return false;
	}

}