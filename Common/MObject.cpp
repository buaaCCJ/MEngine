#include "MObject.h"
std::atomic<unsigned int> MObject::CurrentID = 0;
std::mutex MObject::mtx;

MObject::MObject()
{
	instanceID = CurrentID++;
}

void MObject::AddPtr(PtrLink* ptr)
{
	if (this == nullptr)
	{
		return;
	}
	if (!ptrInitialized)
	{
		allPtrs.reserve(10);
		ptrInitialized = true;
	}
	ptr->value = allPtrs.size();
	allPtrs.push_back(ptr);
}

void PtrLink::Destroy() noexcept
{
	if (mPtr)
	{
		delete mPtr;
		mPtr = nullptr;
	}
}

MObject::~MObject() noexcept
{
	if (allPtrs.empty()) return;
	std::lock_guard<std::mutex> lck(MObject::mtx);
	for (auto ite = allPtrs.begin(); ite != allPtrs.end(); ++ite)
	{
		(*ite)->mPtr = nullptr;
	}
}

void MObject::RemovePtr(PtrLink* ptr, std::unique_lock<std::mutex>& lck)
{
	auto&& ite = allPtrs.end() - 1;
	allPtrs[ptr->value] = *ite;
	memcpy(allPtrs[ptr->value], ptr, sizeof(PtrLink));
	allPtrs.erase(ite);
	lck.unlock();
	bool deleteThis = allPtrs.empty();
	if (deleteThis)
	{
		delete this;
	}
}

PtrLink::PtrLink(MObject* ptr)  noexcept :
	mPtr(ptr)
{
	MObject* obj = (MObject*)(mPtr);
	if (obj != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		obj->AddPtr(this);
	}
}

PtrLink::~PtrLink() noexcept
{
	Dispose();
}

PtrLink::PtrLink(const PtrLink& link)  noexcept :
	mPtr(link.mPtr)
{
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}
}

PtrLink& PtrLink::operator= (const PtrLink& link)  noexcept
{
	if (link.mPtr == mPtr) return *this;
	Dispose();
	mPtr = link.mPtr;
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}

	return *this;
}


PtrLink& PtrLink::operator= (MObject* link)  noexcept
{
	if (link == mPtr) return *this;
	Dispose();
	mPtr = link;
	if (mPtr != nullptr)
	{
		std::lock_guard<std::mutex> lck(MObject::mtx);
		mPtr->AddPtr(this);
	}
	return *this;
}

void PtrLink::Dispose() noexcept
{
	std::unique_lock<std::mutex> lck(MObject::mtx);
	if (mPtr)
	{
		mPtr->RemovePtr(this, lck);
	}
}