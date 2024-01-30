
#pragma once

#include <vector>

namespace engine::mem {

	struct AllocatedResource {
	public:
		virtual ~AllocatedResource() = default;
	private:
		uint32_t nSize = 0;
		template<class U>
		friend U* malloc(size_t count);
		friend void free(AllocatedResource* ptr);
		friend void freeall();
	};

	template<class T>
	T* malloc(size_t count = 1);
	void free(AllocatedResource* ptr);
	void freeall();

}