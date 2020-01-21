#pragma once
typedef unsigned int UINT;
template <size_t constOffset, typename ... Args>
class CBufferTupleSize;

template <size_t constOffset>
class CBufferTupleSize<constOffset>
{
public:
	static const size_t SIZE = constOffset;
	static void GetOffsets(size_t* offsets, UINT startIndex)
	{

	}
};

template <size_t constOffset, typename A, typename ... Args>
class CBufferTupleSize<constOffset, A, Args...>
{
public:
	static const size_t leftedPos = (constOffset % 16 == 0) ? 0 : 16 - (constOffset % 16);
	static const size_t offsetPos = leftedPos >= sizeof(A) ? 0 : leftedPos;
	static const size_t currentPos = constOffset + offsetPos;
	static const size_t SIZE = CBufferTupleSize<currentPos + sizeof(A), Args...>::SIZE;
	static void GetOffsets(size_t* offsets, UINT startIndex)
	{
		offsets[startIndex] = currentPos;
		CBufferTupleSize<currentPos + sizeof(A), Args...>::GetOffsets(offsets, startIndex + 1);
	}
};

template <typename ... Args>
class ConstantBufferStruct
{
public:
	static const size_t size = CBufferTupleSize<0, Args...>::SIZE;
private:
	alignas(float) char c[size];
	size_t offsets[sizeof...(Args)];
public:
	ConstantBufferStruct()
	{
		CBufferTupleSize<0, Args...>::GetOffsets(offsets, 0);
	}
	void* operator[](UINT index)
	{
		return c + offsets[index];
	}
};