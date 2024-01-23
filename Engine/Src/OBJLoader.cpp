
#include "OBJLoader.hpp"

#include "OBJFileLoader.hpp"

namespace engine::filesystem {



	UINT LoadObj(PCWSTR pcwFileName, Vertex** ppVertex, UINT* pnVertexCount)
	{
		
		Loader::FileData f;
		if (!Loader::LoadObj(pcwFileName, &f)) return 0;

		

	}

}