
#pragma once

#include <cstdint>

namespace engine::mem {

	struct MemUnknown {
		virtual uint64_t Release();
		virtual ~MemUnknown() = default;
	protected:
		template<class T>
		void IternalFree();
		void IternalAddRef();
	
		uint64_t*	m_pRefCount = nullptr;
		void*		m_pUnknownPtr = nullptr;
		uint64_t*	m_pCount = nullptr;
	};

	template <class T>
	struct AllocatedResource : public MemUnknown {

		AllocatedResource() = default;
		AllocatedResource(AllocatedResource<T>& copy);

		T* operator->() const;
		T& operator[](const uint64_t index);
		void operator=(const AllocatedResource<T>& copy);

		uint64_t Release() override;
		~AllocatedResource();

	protected:
		template <class U>
		friend AllocatedResource<U> malloc(uint64_t count);
	};

	template<class T>
	AllocatedResource<T> malloc(uint64_t count);

}

namespace engine::mem {

	template<class T>
	inline void MemUnknown::IternalFree()
	{
		if(m_pCount) (*m_pCount == 1) ? delete reinterpret_cast<T*>(m_pUnknownPtr) : delete[] reinterpret_cast<T*>(m_pUnknownPtr);
		delete m_pRefCount;
		delete m_pCount;
		m_pRefCount = nullptr;
		m_pUnknownPtr = nullptr;
		m_pCount = nullptr;
	}

	template<class T>
	inline AllocatedResource<T>::AllocatedResource(AllocatedResource<T>& copy)
	{
		this->operator=(copy);
	}

	template<class T>
	inline T* AllocatedResource<T>::operator->() const
	{
		return reinterpret_cast<T*>(m_pUnknownPtr);
	}

	template<class T>
	inline T& AllocatedResource<T>::operator[](const uint64_t index)
	{
		return reinterpret_cast<T*>(m_pUnknownPtr)[index];
	}

	template<class T>
	inline void AllocatedResource<T>::operator=(const AllocatedResource<T>& copy)
	{
		if (this == &copy) return;
		this->Release();
		m_pRefCount = copy.m_pRefCount;
		m_pUnknownPtr = copy.m_pUnknownPtr;
		m_pCount = copy.m_pCount;
		this->IternalAddRef();
	}

	template<class T>
	inline uint64_t engine::mem::AllocatedResource<T>::Release() {
		MemUnknown::Release();
		if (m_pRefCount) {
			if (*m_pRefCount == 0) {
				this->IternalFree<T>();
			}
			return (m_pRefCount) ? *m_pRefCount : 0;
		}
		return 0;
	}

	template<class T>
	inline AllocatedResource<T>::~AllocatedResource()
	{
		this->Release();
	}
	
	template<class T>
	AllocatedResource<T> malloc(uint64_t count = 1)
	{
		AllocatedResource<T> tempRes;
		tempRes.m_pUnknownPtr = (count == 1) ? new T : new T[count];
		tempRes.m_pRefCount = new uint64_t;
		tempRes.m_pCount = new uint64_t;
		*tempRes.m_pRefCount = 0;
		*tempRes.m_pCount = count;
		tempRes.IternalAddRef();
		return tempRes;
	}

}