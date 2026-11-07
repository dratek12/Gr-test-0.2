#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>
#include <imgui.h>
#include <getrekt/GetRektUI.hpp>
#include <getrekt/Win32Input.hpp>
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;