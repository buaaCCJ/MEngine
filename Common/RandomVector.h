#pragma once
typedef unsigned int UINT;
template<typename T>
class RandomVector final
{
private:
	std::pair<T, UINT*>* arr;
	size_t size;
	size_t capacity;
	void Resize(UINT newCapacity)
	{
		if (newCapacity <= capacity) return;
		UINT maxCapa = capacity * 2;
		newCapacity = max(newCapacity, maxCapa);
		std::pair<T, UINT*>* newArr = (std::pair<T, UINT*>*)malloc(sizeof(std::pair<T, UINT*>) * newCapacity);
		memcpy(newArr, arr, sizeof(std::pair<T, UINT*>) * capacity);
		memset(newArr + capacity, 0, sizeof(std::pair<T, UINT*>) * (newCapacity - capacity));
		free(arr);
		arr = newArr;
		capacity = newCapacity;
	}
public:
	size_t Length() const {
		return size;
	}
	RandomVector(UINT capacity) :
		capacity(capacity),
		size(0)
	{
		arr = (std::pair<T, UINT*>*)malloc(sizeof(std::pair<T, UINT*>) * capacity);
		memset(arr, 0, sizeof(std::pair<T, UINT*>) * capacity);
	}
	T& operator[](UINT index)
	{
#ifdef NDEBUG
		return arr[index].first;
#else
		if (index >= size) throw "Index Out of Range!";
		return arr[index].first;
#endif
	}
	void Add(const T&& value, UINT* indexFlagPtr)
	{
		size++;
		Resize(size);
		*indexFlagPtr = size - 1;
		auto& a = arr[*indexFlagPtr];
		a.first = value;
		a.second = indexFlagPtr;
	}
	void Add(const T& value, UINT* indexFlagPtr)
	{
		Add(std::move(value), indexFlagPtr);
	}
	void Remove(UINT targetIndex)
	{
		if (targetIndex >= size) throw "Index Out of Range!";
		size--;
		arr[targetIndex] = arr[size];
		*arr[targetIndex].second = targetIndex;

	}

	~RandomVector()
	{
		for (UINT i = 0; i < size; ++i)
		{
			arr[i].~pair<T, UINT*>();
		}
		free(arr);
	}
};