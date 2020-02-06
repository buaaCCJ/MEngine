#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix4.h"
using namespace DirectX;
inline float XM_CALLCONV CombineVector4(const XMVECTOR& V2)
{
	XMVECTOR vTemp2 = _mm_shuffle_ps(V2, V2, _MM_SHUFFLE(1, 0, 0, 0)); // Copy X to the Z position and Y to the W position
	vTemp2 = _mm_add_ps(vTemp2, V2);          // Add Z = X+Z; W = Y+W;
	return vTemp2.m128_f32[2] + vTemp2.m128_f32[3];
}

inline float XM_CALLCONV dot(const Math::Vector4& vec, const Math::Vector4& vec1)
{
	return CombineVector4(vec * vec1);
}



inline float XM_CALLCONV CombineVector3(const XMVECTOR& v)
{
	return v.m128_f32[0] + v.m128_f32[1] + v.m128_f32[2];
}

inline float XM_CALLCONV dot(const Math::Vector3& vec, const Math::Vector3& vec1)
{
	return CombineVector3(vec * vec1);
}

inline Math::Vector4 XM_CALLCONV mul(const Math::Matrix4& m, const Math::Vector4& vec)
{
	Math::Matrix4& mat = (Math::Matrix4&)m;
	return {
		dot(mat[0], vec),
		dot(mat[1], vec),
		dot(mat[2], vec),
		dot(mat[3], vec)
	};
}

inline Math::Vector3 XM_CALLCONV mul(const Math::Matrix3& m, const  Math::Vector3& vec)
{
	Math::Matrix4& mat = (Math::Matrix4&)m;
	return {
		CombineVector3(mat[0] * vec),
		CombineVector3(mat[1] * vec),
		CombineVector3(mat[2] * vec)
	};
}

inline Math::Vector3 XM_CALLCONV sqrt(const Math::Vector3& vec)
{
	return _mm_sqrt_ps((XMVECTOR&)vec);
}

inline Math::Vector4 XM_CALLCONV sqrt(const Math::Vector4& vec)
{
	return _mm_sqrt_ps((XMVECTOR&)vec);
}

inline float XM_CALLCONV length(const Math::Vector3& vec1)
{
	Math::Vector3 diff = vec1 * vec1;
	float dotValue = CombineVector3((const XMVECTOR&)diff);
	return sqrt(dotValue);
}

inline float XM_CALLCONV distance(const Math::Vector3& vec1, const Math::Vector3& vec2)
{
	Math::Vector3 diff = vec1 - vec2;
	diff *= diff;
	float dotValue = CombineVector3((const XMVECTOR&)diff);
	return sqrt(dotValue);
}

inline float XM_CALLCONV length(const Math::Vector4& vec1)
{
	Math::Vector4 diff = vec1 * vec1;
	float dotValue = CombineVector4((const XMVECTOR&)diff);
	return sqrt(dotValue);
}

inline float XM_CALLCONV distance(const Math::Vector4& vec1, const Math::Vector4& vec2)
{
	Math::Vector4 diff = vec1 - vec2;
	diff *= diff;
	float dotValue = CombineVector4((const XMVECTOR&)diff);
	return sqrt(dotValue);
}

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


Math::Matrix4 XM_CALLCONV mul
(
	const Math::Matrix4& m1,
	const Math::Matrix4& m2);

template <typename T>
inline T max(const T& a, const T& b)
{
	return (((a) > (b)) ? (a) : (b));
}

template <typename T>
inline T min(const T& a, const T& b)
{
	return (((a) < (b)) ? (a) : (b));
}
template <>
inline Math::Vector3 max<Math::Vector3>(const Math::Vector3& vec1, const Math::Vector3& vec2)
{
	return _mm_max_ps((XMVECTOR&)vec1, (XMVECTOR&)vec2);
}
template <>
inline Math::Vector4 max<Math::Vector4>(const Math::Vector4& vec1, const Math::Vector4& vec2)
{
	return _mm_max_ps((XMVECTOR&)vec1, (XMVECTOR&)vec2);
}
template <>
inline Math::Vector3 min<Math::Vector3>(const Math::Vector3& vec1, const Math::Vector3& vec2)
{
	return _mm_min_ps((XMVECTOR&)vec1, (XMVECTOR&)vec2);
}
template <>
inline Math::Vector4 min<Math::Vector4>(const Math::Vector4& vec1, const Math::Vector4& vec2)
{
	return _mm_min_ps((XMVECTOR&)vec1, (XMVECTOR&)vec2);
}

Math::Matrix4 XM_CALLCONV transpose(const Math::Matrix4& m);


Math::Matrix3 XM_CALLCONV transpose(const Math::Matrix3& m);

Math::Matrix4 XM_CALLCONV inverse(const Math::Matrix4& m);

Math::Matrix4 XM_CALLCONV QuaternionToMatrix(const Math::Vector4& q);

Math::Matrix4 GetTransformMatrix(const Math::Vector3& right, const Math::Vector3& up, const Math::Vector3& forward, const Math::Vector3& position);
Math::Vector4 cross(const Math::Vector4& v1, const Math::Vector4& v2, const Math::Vector4& v3);
Math::Vector3 cross(const Math::Vector3& v1, const Math::Vector3& v2);
Math::Vector4 normalize(const Math::Vector4& v);
Math::Vector3 normalize(const Math::Vector3& v);
Math::Matrix4 GetInverseTransformMatrix(const Math::Vector3& right, const Math::Vector3& up, const Math::Vector3& forward, const Math::Vector3& position);