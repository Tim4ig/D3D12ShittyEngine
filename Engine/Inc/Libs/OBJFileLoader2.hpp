
#pragma once

#ifndef __cplusplus
#error C++ required
#endif // !__cplusplus

#include <vector>
#include <string>

namespace loader {

	template<class T>
	struct Vec3
	{
		Vec3() : x(0), y(0), z(0) {};
		T x, y, z;
	};

	template<class T>
	struct Vec4 : public Vec3<T>
	{
		Vec4() : w(1) {};
		T w;
	};

	struct Index {
		Vec3<int> points[3];
	};

	struct Mesh {
		std::string name;
		uint32_t c_v, c_vt, c_vn, c_f;
		Vec4<float>* pPoints;
		Vec3<float>* pUVs;
		Vec3<float>* pNormals;
		Index* pIndex;
		void Release() { delete[] pPoints; delete[] pUVs; delete[] pNormals; delete[] pIndex; }
	};

	struct File
	{
		std::vector<Mesh> meshes;
		void Release() { for (auto& e : meshes) e.Release(); }
	};

	bool LoadOBJ(std::wstring fileName, File* m);

}