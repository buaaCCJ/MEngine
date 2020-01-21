#pragma once
#include <atomic>
#include <mutex>
#include <vector>
class PtrLink;
class MObject
{
	friend class PtrLink;
private:
	bool ptrInitialized = false;
	static std::mutex mtx;
	std::vector<PtrLink*> allPtrs;
	void AddPtr(PtrLink* ptr);
	void RemovePtr(PtrLink* ptr, std::unique_lock<std::mutex>&);
	static std::atomic<unsigned int> CurrentID;
	unsigned int instanceID;
public:
	
	unsigned int GetInstanceID() const { return instanceID; }
	MObject();
	virtual ~MObject() noexcept;
};

class PtrLink final
{
private:
	void Dispose()  noexcept;
public:
	MObject* mPtr;
	int value;
	PtrLink(MObject* ptr) noexcept;

	PtrLink& operator= (const PtrLink& link) noexcept;
	PtrLink& operator= (MObject* obj) noexcept;

	PtrLink(const PtrLink& link) noexcept;

	~PtrLink() noexcept;
	void Destroy() noexcept;
};


template <typename T>
class ObjectPtr
{
private:
	PtrLink link;

public:
	constexpr ObjectPtr(T* ptr) noexcept :
		link(ptr)
	{}

	constexpr ObjectPtr() noexcept :
		link(nullptr) {}
	constexpr ObjectPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}

	constexpr operator bool() const noexcept
	{
		return link.mPtr != nullptr;
	}

	constexpr operator T*() const noexcept
	{
		return (T*)link.mPtr;
	}

	constexpr ObjectPtr(const PtrLink& ptr) noexcept : link(ptr)
	{

	}
	constexpr void Destroy()
	{
		link.Destroy();
	}

	template<typename F>
	constexpr ObjectPtr<F> Cast() const noexcept
	{
		ObjectPtr<F> mobj = link;
		return mobj;
	}

	constexpr ObjectPtr<T>& operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
		return *this;
	}

	constexpr ObjectPtr<T>& operator=(T* other) noexcept
	{
		link = other;
		return *this;
	}

	constexpr T* operator->() const noexcept
	{
		return (T*)link.mPtr;
	}

	constexpr T& operator*() noexcept
	{
		return *(T*)link.mPtr;
	}

	bool operator==(const T* ptr) const noexcept
	{
		return link.mPtr == ptr;
	}
	bool operator!=(const T* ptr) const noexcept
	{
		return link.mPtr != ptr;
	}
	bool operator==(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.mPtr == ptr.link.mPtr;
	}
	bool operator!=(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.mPtr != ptr.link.mPtr;
	}
};