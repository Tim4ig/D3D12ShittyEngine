
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <string>

#include "Allocator.hpp"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace engine {
	struct ee {
		ee(int code, std::string what) : code(code), what(what) {};
		int			code;
		std::string what;
	};
}
 
#define zmem(OBJ)				 {memset(&OBJ,0,sizeof(OBJ));}
#define srel(OBJ)				 {if(OBJ) OBJ -> Release(); OBJ = nullptr;}
#define throw_if_err			;{if(FAILED(hr)) {throw ee(__LINE__, "undeclared error");}  }
#define throw_if_err_desc(what)	;{if(FAILED(hr)) {throw ee(__LINE__, what);}  }
#define throw_err(what)			 {throw ee(__LINE__, what);}

using Microsoft::WRL::ComPtr;
