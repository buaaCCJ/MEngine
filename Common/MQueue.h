#pragma once
#include "Pool.h"
template <typename T>
class MQueue
{
private:
	struct QueueData
	{
		T data;
		QueueData* next;
	};
	QueueData* first = nullptr;
	QueueData* last = nullptr;
	Pool<QueueData> pool;
	unsigned int mSize;
	void AddNewNode(const T&& value)
	{
		QueueData* newData = pool.New<>();
		newData->data = value;
		newData->next = nullptr;
		if (last != nullptr)
		{
			last->next = newData;
		}
		last = newData;
		if (first == nullptr)
		{
			first = newData;
		}
		mSize++;
	}
	
public:
	MQueue(int capacity) : pool(capacity), mSize(0)
	{
		
	}

	void Enqueue(const T&& value)
	{
		AddNewNode(std::move(value));
	}

	void Enqueue(const T& value)
	{
		AddNewNode(std::move(value));
	}

	bool TryDeque(T* target)
	{
		if (first == nullptr) return false;
		if (first == last) last = nullptr;
		QueueData* deletePtr = first;
		first = first->next;
		*target = deletePtr->data;
		pool.Delete(deletePtr);
		mSize--;
		return true;
	}
	unsigned int size() const { return mSize; }
	~MQueue()
	{
		while (first != nullptr)
		{
			QueueData* ptr = first;
			first = first->next;
			pool.Delete(ptr);
		}
	}
};

