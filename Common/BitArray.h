#pragma once
typedef unsigned char UCHAR;
class BitArray
{
public:
	struct Iterator
	{
		friend class BitArray;
	private:
		BitArray* arr;
		size_t index;
		constexpr Iterator(BitArray* arr, size_t index) :
			arr(arr), index(index) {}
	public:

		operator bool() const
		{
			return arr->Get(index);
		}
		void operator=(bool value)
		{
			arr->Set(index, value);
		}
		constexpr bool operator==(const Iterator& another) const
		{
			return arr == another.arr && index == another.index;
		}
		constexpr bool operator!=(const Iterator& another) const
		{
			return !operator==(another);
		}
		constexpr void operator++()
		{
			index++;
		}
	};
private:
	static const UCHAR bitOffsetArray[8];
	static const UCHAR bitOffsetReversedArray[8];
	UCHAR* ptr = nullptr;
	size_t length = 0;
	bool Get(size_t index) const;
	void Set(size_t index, bool value);
public:
	Iterator begin() const;
	Iterator end() const;
	Iterator operator[](size_t index);
	BitArray(size_t capacity);
	void Clear();
	~BitArray();
};