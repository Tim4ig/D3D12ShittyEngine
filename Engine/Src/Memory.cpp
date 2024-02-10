
#include "Memory.hpp"

namespace engine::mem {

	uint64_t MemUnknown::Release()
	{
		if (m_pRefCount) {
			if (*m_pRefCount)
				(*m_pRefCount)--;
			return (*m_pRefCount);
		} 
		return 0;
	}

	void MemUnknown::IternalAddRef()
	{
		if(m_pRefCount) (*m_pRefCount)++;
	}


}