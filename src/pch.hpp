#pragma once
#include "../resource.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3dx12.h>
#include <dxgi1_6.h>
#include "DirectXMath.h"
#include <atlbase.h>
#include <d3dcompiler.h>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <algorithm>
#include <cassert>
#include <chrono>

inline void ThrowIfFailed(HRESULT result) {
	if (FAILED(result)) {
		throw std::exception();
	}
}