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

inline float XM_CALLCONV CombineVector3(const XMVECTOR& v)
{
	return v.m128_f32[0] + v.m128_f32[1] + v.m128_f32[2];
}

inline Math::Vector4 XM_CALLCONV mul(Math::Matrix4& mat, const Math::Vector4& vec)
{
	return {
		CombineVector4(mat[0] * vec),
		CombineVector4(mat[1] * vec),
		CombineVector4(mat[2] * vec),
		CombineVector4(mat[3] * vec)
	};
}

inline Math::Vector4 XM_CALLCONV mul(Math::Matrix4&& mat, const Math::Vector4& vec)
{
	return mul(mat, vec);
}

inline Math::Vector3 XM_CALLCONV mul(Math::Matrix3& mat, const  Math::Vector3& vec)
{
	return {
		CombineVector3(mat[0] * vec),
		CombineVector3(mat[1] * vec),
		CombineVector3(mat[2] * vec)
	};
}

inline Math::Vector3 XM_CALLCONV mul(Math::Matrix3&& mat, const Math::Vector3& vec)
{
	return mul(mat, vec);
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


inline Math::Matrix4 XM_CALLCONV mul
(
	const Math::Matrix4& m1,
	const Math::Matrix4& m2){
	XMMATRIX& M1 = (XMMATRIX&)m1;
	XMMATRIX& M2 = (XMMATRIX&)m2;
	// Splat the component X,Y,Z then W
#if defined(_XM_AVX_INTRINSICS_)
	XMVECTOR vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 0);
	XMVECTOR vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 1);
	XMVECTOR vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 2);
	XMVECTOR vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[0]) + 3);
#else
	// Use vW to hold the original row
	XMVECTOR vW = M1.r[0];
	XMVECTOR vX = XM_PERMUTE_PS(vW, _MM_SHUFFLE(0, 0, 0, 0));
	XMVECTOR vY = XM_PERMUTE_PS(vW, _MM_SHUFFLE(1, 1, 1, 1));
	XMVECTOR vZ = XM_PERMUTE_PS(vW, _MM_SHUFFLE(2, 2, 2, 2));
	vW = XM_PERMUTE_PS(vW, _MM_SHUFFLE(3, 3, 3, 3));
#endif
	// Perform the operation on the first row
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	// Perform a binary add to reduce cumulative errors
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	XMVECTOR r0 = vX;
	// Repeat for the other 3 rows
#if defined(_XM_AVX_INTRINSICS_)
	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[1]) + 3);
#else
	vW = M1.r[1];
	vX = XM_PERMUTE_PS(vW, _MM_SHUFFLE(0, 0, 0, 0));
	vY = XM_PERMUTE_PS(vW, _MM_SHUFFLE(1, 1, 1, 1));
	vZ = XM_PERMUTE_PS(vW, _MM_SHUFFLE(2, 2, 2, 2));
	vW = XM_PERMUTE_PS(vW, _MM_SHUFFLE(3, 3, 3, 3));
#endif
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	XMVECTOR r1 = vX;
#if defined(_XM_AVX_INTRINSICS_)
	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[2]) + 3);
#else
	vW = M1.r[2];
	vX = XM_PERMUTE_PS(vW, _MM_SHUFFLE(0, 0, 0, 0));
	vY = XM_PERMUTE_PS(vW, _MM_SHUFFLE(1, 1, 1, 1));
	vZ = XM_PERMUTE_PS(vW, _MM_SHUFFLE(2, 2, 2, 2));
	vW = XM_PERMUTE_PS(vW, _MM_SHUFFLE(3, 3, 3, 3));
#endif
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	XMVECTOR r2 = vX;
#if defined(_XM_AVX_INTRINSICS_)
	vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 0);
	vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 1);
	vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 2);
	vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&M1.r[3]) + 3);
#else
	vW = M1.r[3];
	vX = XM_PERMUTE_PS(vW, _MM_SHUFFLE(0, 0, 0, 0));
	vY = XM_PERMUTE_PS(vW, _MM_SHUFFLE(1, 1, 1, 1));
	vZ = XM_PERMUTE_PS(vW, _MM_SHUFFLE(2, 2, 2, 2));
	vW = XM_PERMUTE_PS(vW, _MM_SHUFFLE(3, 3, 3, 3));
#endif
	vX = _mm_mul_ps(vX, M2.r[0]);
	vY = _mm_mul_ps(vY, M2.r[1]);
	vZ = _mm_mul_ps(vZ, M2.r[2]);
	vW = _mm_mul_ps(vW, M2.r[3]);
	vX = _mm_add_ps(vX, vZ);
	vY = _mm_add_ps(vY, vW);
	vX = _mm_add_ps(vX, vY);
	XMVECTOR r3 = vX;

	// x.x,x.y,y.x,y.y
	XMVECTOR vTemp1 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(1, 0, 1, 0));
	// x.z,x.w,y.z,y.w
	XMVECTOR vTemp3 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(3, 2, 3, 2));
	// z.x,z.y,w.x,w.y
	XMVECTOR vTemp2 = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(1, 0, 1, 0));
	// z.z,z.w,w.z,w.w
	XMVECTOR vTemp4 = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(3, 2, 3, 2));

	XMMATRIX mResult;
	// x.x,y.x,z.x,w.x
	mResult.r[0] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(2, 0, 2, 0));
	// x.y,y.y,z.y,w.y
	mResult.r[1] = _mm_shuffle_ps(vTemp1, vTemp2, _MM_SHUFFLE(3, 1, 3, 1));
	// x.z,y.z,z.z,w.z
	mResult.r[2] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(2, 0, 2, 0));
	// x.w,y.w,z.w,w.w
	mResult.r[3] = _mm_shuffle_ps(vTemp3, vTemp4, _MM_SHUFFLE(3, 1, 3, 1));
	return mResult;
}

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