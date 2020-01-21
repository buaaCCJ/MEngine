#pragma once
template <typename T>
class RingQueue
{
private:
	struct Element
	{
		T value;
		Element() {}
		Element(const T& value) : value(value) {

		}
		template<typename... Args>
		void Emplace(Args&&... args)
		{
			new (&value)T(args...);
		}
	};
	Element* arr = nullptr;
	size_t capacity = 0;
	size_t start = 0;
	size_t end = 0;
	void Resize()
	{
		size_t newCapacity = (size_t)(capacity * 1.5);
		Element* newArr = (Element*)malloc(sizeof(Element) * newCapacity);
		for (size_t i = start; i < end; ++i)
		{
			new (newArr + i - start) Element(arr[i % capacity]);
			arr[i % capacity].~Element();
		}
		free(arr);
		arr = newArr;
		capacity = newCapacity;
	}
public:

	RingQueue() {}
	RingQueue(size_t initCapacity) : capacity(initCapacity)
	{
		arr = (Element*)malloc(sizeof(Element) * initCapacity);
	}
	~RingQueue()
	{
		for (size_t i = start; i < end; ++i)
		{
			arr[i].~Element();
		}
		if (arr)
			free(arr);
	}
	void Clear()
	{
		for (size_t i = start; i < end; ++i)
		{
			arr[i % capacity].~Element();
		}
		start = 0;
		end = 0;
	}

	void Push(const T& value)
	{
		if (end - start >= capacity)
		{
			Resize();
		}
		size_t curr = end++;
		new (arr + curr % capacity) Element(value);
	}

	template<typename... Args>
	void EmplacePush(Args&&... args)
	{
		if (end - start >= capacity)
		{
			Resize();
		}
		size_t curr = end++;
		arr[curr % capacity].Emplace(args...);
	}

	void Push(T&& value)
	{
		Push(value);
	}

	bool TryPop(T* target)
	{
		if (end - start == 0)
			return false;
		Element* result = &arr[start % capacity];
		*target = result->value;
		result->~Element();
		start++;
		return true;
	}
};
