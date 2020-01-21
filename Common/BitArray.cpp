#include "BitArray.h"
#include <string>
const UCHAR BitArray::bitOffsetArray[8] =
{
	0b00000001,
	0b00000010,
	0b00000100,
	0b00001000,
	0b00010000,
	0b00100000,
	0b01000000,
	0b10000000
};

const UCHAR BitArray::bitOffsetReversedArray[8] =
{
	0b11111110,
	0b11111101,
	0b11111011,
	0b11110111,
	0b11101111,
	0b11011111,
	0b10111111,
	0b01111111
};

bool BitArray::Get(size_t index) const
{
#ifndef NDEBUG
	if (index >= length) throw "Index Out of Range!";
#endif
	size_t elementIndex = index / 8;
	size_t factor = index - (elementIndex * 8);
	return ptr[elementIndex] & bitOffsetArray[factor];

}

BitArray::Iterator BitArray::begin() const
{
	return Iterator((BitArray*)this, 0);
}

BitArray::Iterator BitArray::end() const
{
	return Iterator((BitArray*)this, length);
}

BitArray::Iterator BitArray::operator[](size_t index)
{
	return Iterator((BitArray*)this, index);
}

void BitArray::Set(size_t index, bool value)
{
#ifndef NDEBUG
	if (index >= length) throw "Index Out of Range!";
#endif
	size_t elementIndex = index / 8;
	size_t factor = index - (elementIndex * 8);
	if (value)
	{
		ptr[elementIndex] |= bitOffsetArray[factor];
	}
	else
	{
		ptr[elementIndex] &= bitOffsetReversedArray[factor];
	}
}

BitArray::BitArray(size_t length)
{
	size_t capa = (length % 8 > 0) ? length / 8 + 1 : length / 8;
	this->length = length;
	ptr = new UCHAR[capa];
	memset(ptr, 0, sizeof(UCHAR) * capa);
}

void BitArray::Clear()
{
	size_t capa = (length % 8 > 0) ? length / 8 + 1 : length / 8;
	memset(ptr, 0, sizeof(UCHAR) * capa);
}

BitArray::~BitArray()
{
	if (ptr)
	{
		delete ptr;
	}
}