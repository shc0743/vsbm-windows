#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#define MyGDIPlusNoGDIPlus 1
//#define DEV 1
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <shellapi.h>
#include <timeapi.h>
#include <shlwapi.h>
#ifndef MyGDIPlusNoGDIPlus
#include <gdiplus.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <format>

#include "TrustCheck.hpp"
#include "resource.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Version.lib")
#ifndef MyGDIPlusNoGDIPlus
#pragma comment(lib, "gdiplus.lib")
#endif
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

wchar_t g_kWindowClass[] = L"yvep#iru#Zlqgrzv#^kwwsv=22jlwkxe1frp2vkf3:762yvep0zlqgrzv`";
wchar_t g_kProductUrl[] = L"nzzvy@55mozn{h4ius5yni6=:95|yhs3}otju}y";
wchar_t g_kOriginalUrl[] = L"p||x{B77k\x82v}tt6oq|p}j6qw7~{ju";
//wchar_t g_kOnlinePreviewUrl[] = L"dpplo6++n]s*cepdq^qoan_kjpajp*_ki+od_,30/+ro^i)sej`kso+nabo+da]`o+i]opan+ro^i*ljc";
int g_askUserWhenConflict = 0;

namespace {

using namespace std::string_literals;

constexpr wchar_t kWindowTitle[] = L"vsbm for Windows";
constexpr int kDefaultClientWidth = 1024;
constexpr int kDefaultClientHeight = 768;
constexpr int kDefaultResolutionWidth = 0;
constexpr int kDefaultResolutionHeight = 0;
constexpr int kBenchmarkResolutionWidth = 1024;
constexpr int kBenchmarkResolutionHeight = 1024;
constexpr float kDefaultLen = 1.6f;
constexpr float kDefaultAng1 = 2.8f;
constexpr float kDefaultAng2 = 0.4f;
constexpr float kDefaultCenX = 0.0f;
constexpr float kDefaultCenY = 0.0f;
constexpr float kDefaultCenZ = 0.0f;
constexpr float kPi = 3.14159265358979323846f;

constexpr float kAutoRotationStep = 0.01f;
constexpr UINT kMaxFrameRateLimit = 1000000;

constexpr UINT_PTR IDM_KERNEL = 0x1F00;
constexpr UINT_PTR IDM_HIDE_TO_TASKBAR = 0x1F01;
constexpr UINT_PTR IDM_HIDE_WHILE_WORKING = 0x1F02;
constexpr UINT_PTR IDM_HELP = 0x1F03;
constexpr UINT_PTR IDM_SETTINGS = 0x1F04;
constexpr UINT_PTR IDM_STATISTICS = 0x1F05;
constexpr UINT_PTR IDM_RENDER_PREVIEW = 0x1F06;
constexpr UINT_PTR IDM_RESET_CAMERA = 0x1F07;
constexpr UINT_PTR IDM_BENCHMARK_MODE = 0x1F08;

constexpr UINT_PTR IDM_TRAY_SHOW = 0x2F00;
constexpr UINT_PTR IDM_TRAY_EXIT = 0x2F01;
constexpr UINT WMAPP_TRAYICON = WM_APP + 1;
constexpr wchar_t kKernelWindowClass[] = L"D3D11RaymarchKernelWindow";
constexpr wchar_t kSettingsWindowClass[] = L"D3D11RaymarchSettingsWindow";
constexpr wchar_t kPreviewWindowClass[] = L"D3D11RaymarchPreviewWindow";
constexpr int IDC_KERNEL_EDIT = 2001;
constexpr int IDC_KERNEL_APPLY = IDOK;
constexpr int IDC_KERNEL_CANCEL = IDCANCEL;
constexpr int IDC_KERNEL_RESET = 2004;
constexpr int IDC_SETTINGS_FPS = 2201;
constexpr int IDC_SETTINGS_VSYNC = 2202;
constexpr int IDC_SETTINGS_OK = IDOK;
constexpr int IDC_SETTINGS_CANCEL = IDCANCEL;
constexpr int IDC_SETTINGS_RESOLUTION = 2205;
constexpr int IDC_SETTINGS_RESET = 2206;
constexpr int IDC_SETTINGS_ALLOW_ONLY_ONE_INSTANCE = 2207;

struct alignas(16) CameraConstants {
	float right[3];
	float pad0;
	float forward[3];
	float pad1;
	float up[3];
	float pad2;
	float origin[3];
	float x;
	float y;
	float len;
	float pad3;
};

static_assert(sizeof(CameraConstants) == 80, "CameraConstants must be 80 bytes for D3D11 constant-buffer packing");

struct TouchPoint {
	bool active = false;
	DWORD id = 0;
	float x = 0.0f;
	float y = 0.0f;
};

HWND g_hwnd = nullptr;
HWND g_kernelDialog = nullptr;
HWND g_kernelDialogEdit = nullptr;
HWND g_kernelDialogApply = nullptr;
HWND g_kernelDialogCancel = nullptr;
HWND g_kernelDialogReset = nullptr;
HWND g_settingsDialog = nullptr;
HWND g_settingsFpsLabel = nullptr;
HWND g_settingsFpsEdit = nullptr;
HWND g_settingsVsyncCheck = nullptr;
HWND g_settingsOk = nullptr;
HWND g_settingsCancel = nullptr;
HWND g_settingsReset = nullptr;
HWND g_settingsResolutionLabel = nullptr;
HWND g_settingsResolutionCombo = nullptr;
HWND g_settingsAllowOnlyOneInstanceCheck = nullptr;
HWND g_settingsAllowOnlyOneInstanceWarning = nullptr;
int g_settingsLastResolutionIndex = 0;
HWND g_previewWindow = nullptr;
#ifndef MyGDIPlusNoGDIPlus
Gdiplus::Bitmap* g_previewBitmap = nullptr;
#endif
IStream* g_previewStream = nullptr;
ULONG_PTR g_gdiplusToken = 0;

HICON g_hIcon = nullptr;
HICON g_hIconSmall = nullptr;
HFONT g_kernelDialogFont = nullptr;
HFONT g_settingsDialogFont = nullptr;

NOTIFYICONDATAW g_trayIcon{};
bool g_trayIconAdded = false;

bool g_paused = false;
bool g_needsRedraw = false;
bool g_pauseStateBeforeHideToTaskbar = false;
bool g_hiddenToTaskbar = false;
bool g_hiddenWhileWorking = false;
LONG_PTR g_exStyleBeforeHideWhileWorking = 0;
BYTE g_alpha = 255;

UINT g_taskbarCreatedMessage = 0;

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swapChain = nullptr;
ID3D11RenderTargetView* g_renderTarget = nullptr;
ID3D11VertexShader* g_vertexShader = nullptr;
ID3D11PixelShader* g_pixelShader = nullptr;
ID3D11Buffer* g_cameraBuffer = nullptr;
ID3D11RasterizerState* g_rasterizerState = nullptr;

UINT g_renderWidth = kDefaultClientWidth;
UINT g_renderHeight = kDefaultClientHeight;

float g_len = kDefaultLen;
float g_ang1 = kDefaultAng1;
float g_ang2 = kDefaultAng2;
float g_cenx = kDefaultCenX;
float g_ceny = kDefaultCenY;
float g_cenz = kDefaultCenZ;

bool g_leftDown = false;
bool g_rightDown = false;
bool g_mouseMoved = false;
int g_mouseX = 0;
int g_mouseY = 0;

TouchPoint g_touches[2];

// FPS is measured from successfully returned Present calls, so it reflects
// the actual frame rate of the presented application rather than the rate
// at which frames are requested. The title is updated periodically, FurMark-style.
LARGE_INTEGER g_fpsFrequency{};
LARGE_INTEGER g_fpsSampleStart{};
uint64_t g_fpsFrameCount = 0;
double g_fps = 0.0;
double g_maxFps = 0.0;
double g_maxFpsHistory = 0.0;
double g_maxFpsBenchmarkHistory = 0.0;

uint64_t g_totalPresentedFrames = 0;
LARGE_INTEGER g_activeSegmentStart{};
double g_activeSeconds = 0.0;

UINT g_frameRateLimit = 0;
bool g_vsyncEnabled = false;
bool g_allowOnlyOneInstance = true;
int g_resolutionWidth = kDefaultResolutionWidth;
int g_resolutionHeight = kDefaultResolutionHeight;
bool g_occluded = false;
bool g_renderFailed = false;
bool g_benchmarkMode = false;
bool g_benchmarkAutoPausedByMinimize = false;
bool g_benchmarkTransitioning = false;
bool g_firstRunPending = false;
bool g_iniDeletedByReset = false;
LARGE_INTEGER g_lastFrameTime{};

std::string g_kernel;
std::string g_defaultKernel;
std::wstring GetExecutableDirectory();

struct ResolutionOption {
	int width;
	int height;
};

// width == -1 marks a disabled separator entry; 0x0 is the unrestricted option.
constexpr ResolutionOption kResolutionOptions[] = {
	{0, 0}, {-1, 0},
	{320, 240}, {480, 360}, {640, 480}, {800, 600}, {1024, 768},
	{1152, 864}, {1280, 960}, {1400, 1050}, {1600, 1200}, {1920, 1440}, {2048, 1536},
	{-1, 0},
	{640, 360}, {854, 480}, {1280, 720}, {1366, 768}, {1600, 900},
	{1920, 1080}, {2560, 1440}, {3840, 2160}, {5120, 2880}, {7680, 4320},
	{-1, 0},
	{256, 256}, {320, 320}, {480, 480}, {512, 512}, {640, 640}, {720, 720},
	{768, 768}, {1024, 1024}, {1080, 1080}, {1280, 1280}, {1440, 1440}, {1536, 1536},
	{2048, 2048}, {2160, 2160}, {2560, 2560}, {2880, 2880}, {3072, 3072}, {3840, 3840},
	{4096, 4096}, {4320, 4320}, {5120, 5120}, {6144, 6144}, {7680, 7680},
};

bool IsResolutionSeparator(int index)
{
	return index >= 0 && index < static_cast<int>(std::size(kResolutionOptions)) &&
		kResolutionOptions[index].width == -1;
}

void FormatResolutionOptionLabel(int index, wchar_t* buffer, size_t bufferCount)
{
	if (index < 0 || index >= static_cast<int>(std::size(kResolutionOptions))) {
		buffer[0] = L'\0';
		return;
	}

	const ResolutionOption& option = kResolutionOptions[index];
	std::wstring label;
	if (option.width == -1) {
		label = L"--------";
	}
	else if (option.width == 0) {
		label = L"Any resolution";
	}
	else {
		label = std::format(L"{}x{}", option.width, option.height);
	}
	wcscpy_s(buffer, bufferCount, label.c_str());
}

int FindResolutionOption(int width, int height)
{
	for (int i = 0; i < static_cast<int>(std::size(kResolutionOptions)); ++i) {
		if (kResolutionOptions[i].width == width && kResolutionOptions[i].height == height) {
			return i;
		}
	}
	return 0;
}

std::wstring GetIniPath()
{
	return GetExecutableDirectory() + L"\\vsbm-windows.ini";
}

bool ReadIniInt(const wchar_t* section, const wchar_t* key, int& value)
{
	wchar_t buffer[64]{};
	const std::wstring ini = GetIniPath();
	if (!GetPrivateProfileStringW(section, key, L"", buffer, static_cast<DWORD>(std::size(buffer)), ini.c_str())) {
		return false;
	}

	wchar_t* end = nullptr;
	const long parsed = wcstol(buffer, &end, 10);
	if (end == buffer || *end != L'\0' ||
		parsed < static_cast<long>(std::numeric_limits<int>::min()) ||
		parsed > static_cast<long>(std::numeric_limits<int>::max())) {
		return false;
	}

	value = static_cast<int>(parsed);
	return true;
}

void WriteIniInt(const wchar_t* section, const wchar_t* key, int value)
{
	WritePrivateProfileStringW(section, key, std::to_wstring(value).c_str(), GetIniPath().c_str());
}

bool ReadIniStr(const wchar_t* section, const wchar_t* key, std::wstring& value,
	const std::wstring& defaultValue = L"")
{
	auto buffer = std::make_unique<WCHAR[]>(32768);
	const std::wstring ini = GetIniPath();
	if (!GetPrivateProfileStringW(section, key, L"", buffer.get(), 32768, ini.c_str())) {
		return false;
	}
	value = buffer.get();
	return true;
}

void WriteIniStr(const wchar_t* section, const wchar_t* key,
	const std::wstring& value)
{
	WritePrivateProfileStringW(section, key, value.c_str(), GetIniPath().c_str());
}

bool LoadResToString(
	DWORD dwResName, std::wstring lpResType, std::string& outData, HMODULE hInst
) {
	HMODULE hInstance = hInst ? hInst : GetModuleHandle(NULL);

	HRSRC hResID = ::FindResourceW(
		hInstance, MAKEINTRESOURCEW(dwResName), lpResType.c_str());
	if (!hResID) return false;

	HGLOBAL hRes = ::LoadResource(hInstance, hResID);
	if (!hRes) return false;

	LPVOID pRes = ::LockResource(hRes);
	if (pRes == NULL) return false;

	DWORD dwResSize = ::SizeofResource(hInstance, hResID);
	if (dwResSize == 0) {
		outData.clear();
		return true;
	}

	outData.assign(static_cast<const char*>(pRes), dwResSize);
	return true;
}

struct WindowSettings {
	bool hasPositionAndSize = false;
	int left = CW_USEDEFAULT;
	int top = CW_USEDEFAULT;
	int width = 0;
	int height = 0;
	BYTE alpha = 255;
};

WindowSettings g_windowSettings{};

void ResetCamera()
{
	g_len = kDefaultLen;
	g_ang1 = kDefaultAng1;
	g_ang2 = kDefaultAng2;
	g_cenx = kDefaultCenX;
	g_ceny = kDefaultCenY;
	g_cenz = kDefaultCenZ;
}

void ResetSessionStatistics()
{
	g_totalPresentedFrames = 0;
	g_activeSeconds = 0.0;
	g_maxFps = 0.0;
	g_fps = 0.0;
	if (g_fpsFrequency.QuadPart > 0) {
		QueryPerformanceCounter(&g_fpsSampleStart);
		g_activeSegmentStart = g_fpsSampleStart;
		g_lastFrameTime = g_fpsSampleStart;
		g_fpsFrameCount = 0;
	}
}

void ResetRuntimePreferencesToDefaults()
{
	g_windowSettings = WindowSettings{};
	g_frameRateLimit = 0;
	g_vsyncEnabled = false;
	g_allowOnlyOneInstance = true;
	g_resolutionWidth = kDefaultResolutionWidth;
	g_resolutionHeight = kDefaultResolutionHeight;
	g_alpha = 255;
	g_askUserWhenConflict = 0;
	g_maxFpsHistory = 0.0;
	g_maxFpsBenchmarkHistory = 0.0;
	g_benchmarkAutoPausedByMinimize = false;
	g_firstRunPending = false;
}

DECLSPEC_NOINLINE void LoadWindowSettings()
{
	ResetRuntimePreferencesToDefaults();

	WCHAR compName[64]{};
	DWORD compSize = 63;
	GetComputerNameW(compName, &compSize);

	if (g_benchmarkMode) {
		std::wstring benchmarkHistFps;
		if (ReadIniStr(L"Benchmark", std::format(L"Computer-{}_MaxFps", compName).c_str(), benchmarkHistFps, L"0.0")) {
			try {
				g_maxFpsBenchmarkHistory = std::stod(benchmarkHistFps);
			} catch (...) {
				g_maxFpsBenchmarkHistory = 0.0;
			}
		}
		return;
	}

	int left = 0, top = 0, width = 0, height = 0, opacity = 255;
	const bool haveLeft = ReadIniInt(L"Window", L"Left", left);
	const bool haveTop = ReadIniInt(L"Window", L"Top", top);
	const bool haveWidth = ReadIniInt(L"Window", L"Width", width);
	const bool haveHeight = ReadIniInt(L"Window", L"Height", height);

	if (haveLeft && haveTop && haveWidth && haveHeight &&
		width >= 256 && height >= 256 && width <= 16384 && height <= 16384) {
		g_windowSettings.hasPositionAndSize = true;
		g_windowSettings.left = left;
		g_windowSettings.top = top;
		g_windowSettings.width = width;
		g_windowSettings.height = height;
	}

	if (ReadIniInt(L"Window", L"Opacity", opacity)) {
		opacity = std::max(0, std::min(255, opacity));
		g_windowSettings.alpha = static_cast<BYTE>(opacity);
	}

	int isFirstRun = 1;
	if (ReadIniInt(L"Settings", L"IsFirstRun", isFirstRun)) {
		g_firstRunPending = isFirstRun != 0;
	} else {
		g_firstRunPending = true;
	}

	ReadIniInt(L"Window", L"AskUserWhenConflict", g_askUserWhenConflict);

	int frameRateLimit = static_cast<int>(g_frameRateLimit);
	if (ReadIniInt(L"Settings", L"FrameRateLimit", frameRateLimit) &&
		frameRateLimit >= 0 && frameRateLimit <= 1000000) {
		g_frameRateLimit = static_cast<UINT>(frameRateLimit);
	}

	int vsyncEnabled = g_vsyncEnabled ? 1 : 0;
	if (ReadIniInt(L"Settings", L"VsyncEnabled", vsyncEnabled)) {
		g_vsyncEnabled = vsyncEnabled != 0;
	}

	int allowOnlyOneInstance = g_allowOnlyOneInstance ? 1 : 0;
	if (ReadIniInt(L"Settings", L"AllowOnlyOneInstance", allowOnlyOneInstance)) {
		g_allowOnlyOneInstance = allowOnlyOneInstance != 0;
	}

	std::wstring resolution;
	if (ReadIniStr(L"Settings", L"Resolution", resolution) && !resolution.empty()) {
		if (resolution == L"Any") {
			g_resolutionWidth = 0;
			g_resolutionHeight = 0;
		} else {
			int parsedWidth = 0;
			int parsedHeight = 0;
			if (swscanf_s(resolution.c_str(), L"%dx%d", &parsedWidth, &parsedHeight) == 2 &&
				FindResolutionOption(parsedWidth, parsedHeight) != 0) {
				g_resolutionWidth = parsedWidth;
				g_resolutionHeight = parsedHeight;
			}
		}
	}

	std::wstring histFpsMax;
	if (ReadIniStr(L"Statistics", std::format(L"Computer-{}_MaxFps", compName).c_str(), histFpsMax, L"0.0")) {
		try {
			g_maxFpsHistory = std::stod(histFpsMax);
		} catch (...) {
			g_maxFpsHistory = 0.0;
		}
	}

	g_alpha = g_windowSettings.alpha;
}


bool GetNormalWindowRect(HWND hwnd, RECT& rect)
{
	WINDOWPLACEMENT placement{};
	placement.length = sizeof(placement);
	if (!GetWindowPlacement(hwnd, &placement)) {
		return false;
	}

	rect = placement.rcNormalPosition;
	return rect.right > rect.left && rect.bottom > rect.top;
}

void SaveWindowSettings()
{
	if (!g_hwnd || g_benchmarkMode || g_iniDeletedByReset) {
		return;
	}

	RECT rect{};
	if (GetNormalWindowRect(g_hwnd, rect)) {
		WriteIniInt(L"Window", L"Left", rect.left);
		WriteIniInt(L"Window", L"Top", rect.top);
		WriteIniInt(L"Window", L"Width", rect.right - rect.left);
		WriteIniInt(L"Window", L"Height", rect.bottom - rect.top);
	}

	WriteIniInt(L"Window", L"Opacity", static_cast<int>(g_alpha));
	WriteIniInt(L"Window", L"AskUserWhenConflict", static_cast<int>(g_askUserWhenConflict));

	WriteIniInt(L"Settings", L"FrameRateLimit", static_cast<int>(g_frameRateLimit));
	WriteIniInt(L"Settings", L"VsyncEnabled", g_vsyncEnabled ? 1 : 0);
	WriteIniInt(L"Settings", L"AllowOnlyOneInstance", g_allowOnlyOneInstance ? 1 : 0);
	if (g_resolutionWidth > 0 && g_resolutionHeight > 0) {
		WriteIniStr(L"Settings", L"Resolution",
			std::format(L"{}x{}", g_resolutionWidth, g_resolutionHeight));
	} else {
		WriteIniStr(L"Settings", L"Resolution", L"Any");
	}
	WriteIniInt(L"Settings", L"IsFirstRun", g_firstRunPending ? 1 : 0);

	WCHAR compName[64]{};
	DWORD compSize = 63;
	GetComputerNameW(compName, &compSize);

	if (g_maxFpsHistory < g_maxFps) g_maxFpsHistory = g_maxFps;
	WriteIniStr(L"Statistics", std::format(L"Computer-{}_MaxFps", compName).c_str(), std::to_wstring(g_maxFpsHistory).c_str());
}

void SaveBenchmarkStatistics()
{
	if (!g_benchmarkMode) {
		return;
	}

	WCHAR compName[64]{};
	DWORD compSize = 63;
	GetComputerNameW(compName, &compSize);

	if (g_maxFpsBenchmarkHistory < g_maxFps) {
		g_maxFpsBenchmarkHistory = g_maxFps;
	}
	WriteIniStr(L"Benchmark", std::format(L"Computer-{}_MaxFps", compName).c_str(),
		std::to_wstring(g_maxFpsBenchmarkHistory));
}

void MarkIniDirty()
{
	g_iniDeletedByReset = false;
}

bool DeleteUserIniFile()
{
	const std::wstring iniPath = GetIniPath();
	if (DeleteFileW(iniPath.c_str())) {
		return true;
	}
	return GetLastError() == ERROR_FILE_NOT_FOUND;
}

void ResetUserPreferences()
{
	ResetRuntimePreferencesToDefaults();
	ResetSessionStatistics();
	g_firstRunPending = true;
	g_iniDeletedByReset = true;
}

std::wstring GetExecutableLocation()
{
	auto buffer = std::make_unique<WCHAR[]>(32768);
	const DWORD length = GetModuleFileNameW(NULL, buffer.get(), 32768);
	if (length == 0 || length >= MAX_PATH) {
		return L".";
	}
	std::wstring path(buffer.get(), length);
	return path;
}

std::wstring GetExecutableDirectory()
{
	auto path = GetExecutableLocation();
	const size_t slash = path.find_last_of(L"\\/");
	if (slash == std::wstring::npos) {
		return L".";
	}
	return path.substr(0, slash);
}

void ClampSavedWindowRectToMonitor(RECT& rect)
{
	HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!monitor || !GetMonitorInfoW(monitor, &mi)) {
		return;
	}

	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;
	const RECT work = mi.rcWork;

	if (rect.right <= work.left + 40 || rect.left >= work.right - 40) {
		rect.left = work.left + (work.right - work.left - width) / 2;
	}
	if (rect.bottom <= work.top + 40 || rect.top >= work.bottom - 40) {
		rect.top = work.top + (work.bottom - work.top - height) / 2;
	}
}

SIZE GetClientSizeForWindowRect(HWND hwnd, const RECT& windowRect)
{
	RECT currentClient{};
	GetClientRect(hwnd, &currentClient);
	POINT clientOrigin{0, 0};
	ClientToScreen(hwnd, &clientOrigin);
	RECT currentWindow{};
	GetWindowRect(hwnd, &currentWindow);

	const int currentClientWidth = currentClient.right - currentClient.left;
	const int currentClientHeight = currentClient.bottom - currentClient.top;
	const int borderLeft = clientOrigin.x - currentWindow.left;
	const int borderTop = clientOrigin.y - currentWindow.top;
	const int borderRight = currentWindow.right - (clientOrigin.x + currentClientWidth);
	const int borderBottom = currentWindow.bottom - (clientOrigin.y + currentClientHeight);

	SIZE clientSize{};
	clientSize.cx = std::max<LONG>(0,
		(windowRect.right - windowRect.left) - borderLeft - borderRight);
	clientSize.cy = std::max<LONG>(0,
		(windowRect.bottom - windowRect.top) - borderTop - borderBottom);
	return clientSize;
}

std::wstring QueryVersionString(const std::vector<BYTE>& data, std::wstring_view field)
{
	struct LangCp { WORD lang, cp; } *tr = nullptr;
	UINT cb = 0;
	if (!::VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation",
		reinterpret_cast<LPVOID*>(&tr), &cb) || cb < sizeof(*tr))
		return {};

	for (UINT i = 0; i < cb / sizeof(*tr); ++i)
	{
		auto sub = std::format(L"\\StringFileInfo\\{:04X}{:04X}\\{}", tr[i].lang, tr[i].cp, field);
		LPWSTR p = nullptr; UINT len = 0;
		if (::VerQueryValueW(data.data(), sub.c_str(),
			reinterpret_cast<LPVOID*>(&p), &len) && len > 0)
			return std::wstring(p, len - 1);
	}
	return {};
}

struct AppVersion {
	bool created{};
	WORD major{}, minor{}, build{}, revision{};
};

AppVersion GetSelfVersion() {
	std::wstring path = GetExecutableLocation();

	DWORD dummy = 0;
	DWORD size = ::GetFileVersionInfoSizeW(path.c_str(), &dummy);
	if (size == 0) throw std::runtime_error("no VERSIONINFO resource");

	std::vector<BYTE> data(size);
	if (!::GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
		throw std::runtime_error("GetFileVersionInfoW failed");

	AppVersion v;
	v.created = true;

	VS_FIXEDFILEINFO* ffi = nullptr; UINT ffiLen = 0;
	if (::VerQueryValueW(data.data(), L"\\", reinterpret_cast<LPVOID*>(&ffi), &ffiLen)
		&& ffiLen >= sizeof(VS_FIXEDFILEINFO))
	{
		v.major = HIWORD(ffi->dwFileVersionMS);
		v.minor = LOWORD(ffi->dwFileVersionMS);
		v.build = HIWORD(ffi->dwFileVersionLS);
		v.revision = LOWORD(ffi->dwFileVersionLS);
	}
	return v;
}

void UpdateWindowTitle(int clientWidth, int clientHeight)
{
	if (!g_hwnd) {
		return;
	}
	static AppVersion ver{};
	static std::wstring vertext;
	if (!ver.created) {
		ver = GetSelfVersion();
		vertext = std::format(L"{}.{}.{}", ver.major, ver.minor, ver.build);
	}

	const unsigned displayedFps = g_fps > 0.0
		? static_cast<unsigned>(std::lround(g_fps))
		: 0u;

	const wchar_t* benchmarkPrefix = g_benchmarkMode ? L"[Benchmark] " : L"";
	const wchar_t* modifiedPrefix = (!g_benchmarkMode && g_kernel != g_defaultKernel) ? L"* " : L"";
	std::wstring title = std::format(L"{}{}{} {} {}- {} FPS @ {}x{}",
		benchmarkPrefix, modifiedPrefix, kWindowTitle,
		vertext, (g_paused ? L"- Paused " : L""),
		displayedFps, clientWidth, clientHeight);
	SetWindowTextW(g_hwnd, title.c_str());
}

void UpdateWindowTitle()
{
	RECT client{};
	GetClientRect(g_hwnd, &client);
	UpdateWindowTitle(
		std::max<LONG>(0, client.right - client.left),
		std::max<LONG>(0, client.bottom - client.top));
}

void ApplyPausedTitle()
{
	UpdateWindowTitle();
}

void ApplyResolutionStyleAndSize()
{
	if (!g_hwnd) {
		return;
	}

	LONG_PTR style = GetWindowLongPtrW(g_hwnd, GWL_STYLE);
	const bool fixedResolution = g_resolutionWidth > 0 && g_resolutionHeight > 0;
	if (fixedResolution) {
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	} else {
		style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	}
	SetWindowLongPtrW(g_hwnd, GWL_STYLE, style);
	SetWindowPos(g_hwnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	if (!fixedResolution) {
		return;
	}

	if (IsZoomed(g_hwnd)) {
		ShowWindow(g_hwnd, SW_RESTORE);
	}

	RECT clientRect{0, 0, g_resolutionWidth, g_resolutionHeight};
	const LONG_PTR exStyle = GetWindowLongPtrW(g_hwnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&clientRect, static_cast<DWORD>(style), FALSE, static_cast<DWORD>(exStyle));

	RECT current{};
	GetWindowRect(g_hwnd, &current);
	SetWindowPos(g_hwnd, nullptr,
		current.left, current.top,
		clientRect.right - clientRect.left,
		clientRect.bottom - clientRect.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

double GetElapsedSeconds(LARGE_INTEGER start, LARGE_INTEGER end)
{
	if (g_fpsFrequency.QuadPart <= 0) {
		return 0.0;
	}
	return static_cast<double>(end.QuadPart - start.QuadPart) /
		static_cast<double>(g_fpsFrequency.QuadPart);
}

void InitializeFpsCounter()
{
	QueryPerformanceFrequency(&g_fpsFrequency);
	QueryPerformanceCounter(&g_fpsSampleStart);
	g_activeSegmentStart = g_fpsSampleStart;
	g_lastFrameTime = g_fpsSampleStart;
	g_fpsFrameCount = 0;
	g_fps = 0.0;
	g_maxFps = 0.0;
}

void RecordPresentedFrame()
{
	// Frames drawn on demand while paused are not part of the continuous
	// rendering rate and would otherwise deflate the reported FPS.
	if (g_fpsFrequency.QuadPart <= 0 || g_paused) {
		return;
	}

	++g_totalPresentedFrames;
	++g_fpsFrameCount;

	LARGE_INTEGER now{};
	QueryPerformanceCounter(&now);
	const double elapsed = GetElapsedSeconds(g_fpsSampleStart, now);

	// Update the title twice per second. This avoids changing the caption on
	// every frame while still making the displayed FPS responsive.
	if (elapsed >= 0.5) {
		g_fps = static_cast<double>(g_fpsFrameCount) / elapsed;
		if (g_fps > g_maxFps) {
			g_maxFps = g_fps;
		}
		g_fpsFrameCount = 0;
		g_fpsSampleStart = now;
		UpdateWindowTitle();
	}
}

double GetActiveSeconds()
{
	LARGE_INTEGER now{};
	QueryPerformanceCounter(&now);

	double result = g_activeSeconds;
	if (!g_paused) {
		result += GetElapsedSeconds(g_activeSegmentStart, now);
	}
	return result;
}

void SetPaused(bool paused)
{
	if (g_paused == paused) {
		ApplyPausedTitle();
		return;
	}

	LARGE_INTEGER now{};
	QueryPerformanceCounter(&now);
	if (paused) {
		g_activeSeconds += GetElapsedSeconds(g_activeSegmentStart, now);
	} else {
		g_activeSegmentStart = now;
		g_fpsSampleStart = now;
		g_fpsFrameCount = 0;
	}

	g_paused = paused;
	ApplyPausedTitle();
}

void SetMainWindowAlpha(BYTE alpha)
{
	g_alpha = alpha;
	if (g_hwnd) {
		SetLayeredWindowAttributes(g_hwnd, 0, g_alpha, LWA_ALPHA);
	}
}

bool LaunchSelf(const std::wstring& arguments, bool suspended, PROCESS_INFORMATION& processInfo);

bool SaveKernelSourceToDisk(const std::string& source, std::wstring* errorText = nullptr)
{
	const std::wstring path = GetExecutableDirectory() + L"\\vsbm-windows.hlsl";
	const std::wstring temporaryPath = path + L".tmp";

	HANDLE file = CreateFileW(
		temporaryPath.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (file == INVALID_HANDLE_VALUE) {
		if (errorText) {
			*errorText = L"Cannot create temporary kernel file. Win32 error " +
				std::to_wstring(GetLastError()) + L".";
		}
		return false;
	}

	bool ok = true;
	const char* data = source.data();
	size_t remaining = source.size();

	while (remaining > 0) {
		const DWORD chunk = static_cast<DWORD>(std::min<size_t>(remaining, 1024 * 1024));
		DWORD written = 0;
		if (!WriteFile(file, data, chunk, &written, nullptr) || written != chunk) {
			ok = false;
			break;
		}
		data += written;
		remaining -= written;
	}

	if (ok && !FlushFileBuffers(file)) {
		ok = false;
	}

	DWORD writeError = ERROR_SUCCESS;
	if (!ok) {
		writeError = GetLastError();
		if (writeError == ERROR_SUCCESS) {
			writeError = ERROR_WRITE_FAULT;
		}
	}
	CloseHandle(file);

	if (!ok) {
		DeleteFileW(temporaryPath.c_str());
		if (errorText) {
			*errorText = L"Cannot write vsbm-windows.hlsl. Win32 error " +
				std::to_wstring(writeError) + L".";
		}
		return false;
	}

	if (!MoveFileExW(
			temporaryPath.c_str(),
			path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		const DWORD moveError = GetLastError();
		DeleteFileW(temporaryPath.c_str());
		if (errorText) {
			*errorText = L"Cannot replace vsbm-windows.hlsl. Win32 error " +
				std::to_wstring(moveError) + L".";
		}
		return false;
	}

	return true;
}

void SafeRelease(IUnknown*& object)
{
	if (object) {
		object->Release();
		object = nullptr;
	}
}

template <typename T>
void SafeReleaseT(T*& object)
{
	if (object) {
		object->Release();
		object = nullptr;
	}
}

std::wstring Utf8ToWide(const std::string& input)
{
	if (input.empty()) {
		return {};
	}
	const int count = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
	if (count <= 0) {
		return {};
	}
	std::wstring output(static_cast<size_t>(count), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), &output[0], count);
	return output;
}

std::string WideToUtf8(const std::wstring& input)
{
	if (input.empty()) {
		return {};
	}
	const int count = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
	if (count <= 0) {
		return {};
	}
	std::string output(static_cast<size_t>(count), '\0');
	WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), &output[0], count, nullptr, nullptr);
	return output;
}

std::wstring NormalizeNewlinesToLF(const std::wstring& input)
{
	std::wstring output;
	output.reserve(input.size());
	for (size_t i = 0; i < input.size(); ++i) {
		const wchar_t ch = input[i];
		if (ch == L'\r') {
			output.push_back(L'\n');
			if (i + 1 < input.size() && input[i + 1] == L'\n') {
				++i;
			}
		} else {
			output.push_back(ch);
		}
	}
	return output;
}

std::wstring NormalizeNewlinesToCRLF(const std::wstring& input)
{
	std::wstring output;
	output.reserve(input.size() + input.size() / 16 + 2);
	for (size_t i = 0; i < input.size(); ++i) {
		const wchar_t ch = input[i];
		if (ch == L'\r') {
			output.push_back(L'\r');
			if (i + 1 < input.size() && input[i + 1] == L'\n') {
				output.push_back(L'\n');
				++i;
			} else {
				output.push_back(L'\n');
			}
		} else if (ch == L'\n') {
			output.push_back(L'\r');
			output.push_back(L'\n');
		} else {
			output.push_back(ch);
		}
	}
	return output;
}

UINT GetWindowDpiSafe(HWND hwnd)
{
	const UINT dpi = GetDpiForWindow(hwnd);
	return dpi ? dpi : 96u;
}

int ScaleForDpi(int value, UINT dpi)
{
	return MulDiv(value, static_cast<int>(dpi), 96);
}

void centerWindow(HWND hwnd, HWND parent) {
	RECT rcParent{};
	if (parent) GetWindowRect(parent, &rcParent);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	auto w = rect.right - rect.left, h = rect.bottom - rect.top;
	if (parent) {
		auto w2 = rcParent.right - rcParent.left, h2 = rcParent.bottom - rcParent.top;
		rect.left = rcParent.left + w2 / 2 - w / 2;
		rect.top = rcParent.top + h2 / 2 - h / 2;
	}
	else {
		rect.left = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
		rect.top = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
	}
	UINT flags = SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER;
	flags |= (IsWindowVisible(hwnd) ? 0 : SWP_HIDEWINDOW);
	SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 1, 1, flags);
}

HFONT CreateKernelDialogFont(UINT dpi)
{
	const int height = -MulDiv(9, static_cast<int>(dpi), 72);
	return CreateFontW(
		height, 0, 0, 0,
		FW_NORMAL,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"Consolas");
}

void ApplyKernelDialogFont(HWND hwnd)
{
	if (!hwnd) {
		return;
	}

	const HFONT newFont = CreateKernelDialogFont(GetWindowDpiSafe(hwnd));
	if (!newFont) {
		return;
	}

	if (g_kernelDialogEdit) {
		SendMessageW(g_kernelDialogEdit, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
	}
	if (g_kernelDialogApply) {
		SendMessageW(g_kernelDialogApply, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
	}
	if (g_kernelDialogCancel) {
		SendMessageW(g_kernelDialogCancel, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
	}
	if (g_kernelDialogReset) {
		SendMessageW(g_kernelDialogReset, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
	}

	if (g_kernelDialogFont) {
		DeleteObject(g_kernelDialogFont);
	}
	g_kernelDialogFont = newFont;
}

void LayoutKernelDialog(HWND hwnd)
{
	RECT client{};
	GetClientRect(hwnd, &client);

	const UINT dpi = GetWindowDpiSafe(hwnd);
	const int margin = ScaleForDpi(8, dpi);
	const int gap = ScaleForDpi(8, dpi);
	const int buttonWidth = ScaleForDpi(86, dpi);
	const int buttonHeight = ScaleForDpi(28, dpi);

	const int width = client.right - client.left;
	const int height = client.bottom - client.top;
	const int editHeight = std::max(1, height - margin * 3 - buttonHeight);

	if (g_kernelDialogReset) {
		SetWindowPos(g_kernelDialogReset, nullptr,
			margin,
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}

	if (g_kernelDialogEdit) {
		SetWindowPos(g_kernelDialogEdit, nullptr,
			margin, margin,
			std::max(1, width - margin * 2), editHeight,
			SWP_NOZORDER);
	}

	if (g_kernelDialogApply) {
		SetWindowPos(g_kernelDialogApply, nullptr,
			std::max(margin, width - margin - buttonWidth * 2 - gap),
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}

	if (g_kernelDialogCancel) {
		SetWindowPos(g_kernelDialogCancel, nullptr,
			std::max(margin, width - margin - buttonWidth),
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}
}

std::string LoadTextFile(const std::wstring& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		return {};
	}
	std::ostringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

bool ReplaceAll(std::string& value, const std::string& from, const std::string& to)
{
	if (from.empty()) {
		return false;
	}
	bool changed = false;
	size_t pos = 0;
	while ((pos = value.find(from, pos)) != std::string::npos) {
		value.replace(pos, from.size(), to);
		pos += to.size();
		changed = true;
	}
	return changed;
}

std::string NormalizeNewlinesToLF(const std::string& input)
{
	std::string output;
	output.reserve(input.size());

	for (size_t i = 0; i < input.size(); ++i) {
		const char ch = input[i];
		if (ch == '\r') {
			output.push_back('\n');
			if (i + 1 < input.size() && input[i + 1] == '\n') {
				++i;
			}
		} else {
			output.push_back(ch);
		}
	}

	return output;
}

std::string GetDefaultKernelSource()
{
	std::string str;
	LoadResToString(IDR_BIN_KERNEL, L"BIN", str, 0);
	return str;
}

std::string GetKernelSource()
{
	const std::wstring path = GetExecutableDirectory() + L"\\vsbm-windows.hlsl";
	std::string loaded = LoadTextFile(path);
	if (!loaded.empty()) {
		return NormalizeNewlinesToLF(loaded);
	}

	return GetDefaultKernelSource();
}

std::string BuildPixelShaderSource(const std::string& kernelSource)
{
	std::string tmp, result;
	LoadResToString(IDR_BIN_PIXELSHADER1, L"BIN", result, 0);
	LoadResToString(IDR_BIN_PIXELSHADER2, L"BIN", tmp, 0);
	result += (kernelSource);
	result += tmp;
	return result;
}

void ShowCompileError(const wchar_t* title, ID3DBlob* errors)
{
	std::string text;
	if (errors && errors->GetBufferPointer() && errors->GetBufferSize()) {
		text.assign(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
	} else {
		text = "Unknown shader compilation error.";
	}
	const std::wstring wide = Utf8ToWide(text);
	MessageBoxW(g_hwnd, wide.c_str(), title, MB_OK | MB_ICONERROR);
}

bool CompileVertexShader()
{
	ID3DBlob* shader = nullptr;
	ID3DBlob* errors = nullptr;
	std::string vs;
	LoadResToString(IDR_BIN_VERTEXSHADER, L"BIN", vs, 0);
	const HRESULT hr = D3DCompile(
		vs.c_str(),
		vs.size(),
		"vertex.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shader,
		&errors);

	if (FAILED(hr)) {
		ShowCompileError(L"Vertex Shader Compilation Failed", errors);
		SafeReleaseT(errors);
		SafeReleaseT(shader);
		return false;
	}

	HRESULT createHr = g_device->CreateVertexShader(
		shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &g_vertexShader);
	if (FAILED(createHr)) {
		SafeReleaseT(errors);
		SafeReleaseT(shader);
		return false;
	}

	SafeReleaseT(errors);
	SafeReleaseT(shader);

	return true;
}

bool CompileKernelShader(const std::string& kernelSource, bool showError)
{
	const std::string pixelSource = BuildPixelShaderSource(kernelSource);
	ID3DBlob* shader = nullptr;
	ID3DBlob* errors = nullptr;

	const HRESULT hr = D3DCompile(
		pixelSource.data(),
		pixelSource.size(),
		"kernel.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shader,
		&errors);

	if (FAILED(hr)) {
		if (showError) {
			ShowCompileError(L"Pixel Shader Compilation Failed", errors);
		}
		SafeReleaseT(errors);
		SafeReleaseT(shader);
		return false;
	}

	ID3D11PixelShader* newShader = nullptr;
	const HRESULT createHr = g_device->CreatePixelShader(
		shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &newShader);

	if (FAILED(createHr)) {
		if (showError) {
			MessageBoxW(g_hwnd, L"D3D11 could not create the compiled Pixel Shader.", L"Pixel Shader Error", MB_OK | MB_ICONERROR);
		}
		SafeReleaseT(errors);
		SafeReleaseT(shader);
		return false;
	}

	SafeReleaseT(errors);
	SafeReleaseT(shader);

	SafeReleaseT(g_pixelShader);
	g_pixelShader = newShader;
	g_kernel = kernelSource;
	g_needsRedraw = true;
	UpdateWindowTitle();
	return true;
}

void ReleaseRenderTarget()
{
	SafeReleaseT(g_renderTarget);
}

bool CreateRenderTarget()
{
	ID3D11Texture2D* backBuffer = nullptr;
	const HRESULT hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (FAILED(hr)) {
		return false;
	}
	const HRESULT createHr = g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
	SafeReleaseT(backBuffer);
	return SUCCEEDED(createHr);
}

void UpdateRenderViewport()
{
	RECT client{};
	GetClientRect(g_hwnd, &client);

	g_renderWidth = static_cast<UINT>(std::max<LONG>(1, client.right - client.left));
	g_renderHeight = static_cast<UINT>(std::max<LONG>(1, client.bottom - client.top));

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(g_renderWidth);
	viewport.Height = static_cast<float>(g_renderHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_context->RSSetViewports(1, &viewport);
}

bool ResizeSwapChain(UINT width, UINT height)
{
	if (!g_swapChain || !g_context) {
		return false;
	}

	ReleaseRenderTarget();
	g_context->OMSetRenderTargets(0, nullptr, nullptr);

	const HRESULT hr = g_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) {
		return false;
	}

	if (!CreateRenderTarget()) {
		return false;
	}

	UpdateRenderViewport();
	return true;
}

void OpenUrl(std::wstring url, HWND hwnd) {
	STARTUPINFOW si{ sizeof(si) }; PROCESS_INFORMATION pi{};
	WCHAR s32[256]{}; GetSystemDirectoryW(s32, 256);
	url = std::format(L"RunDLL \"{}/url.dll\",FileProtocolHandler \"{}\"", s32, url);
	if (!CreateProcessW((s32 + L"/rundll32.exe"s).c_str(), url.data(), 0, 0, 0, 0, 0, 0, &si, &pi)) {
		MessageBoxW(hwnd, L"Cannot open the page.", NULL, MB_ICONHAND);
	}
	else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

bool InitD3D()
{
	RECT rect{};
	GetClientRect(g_hwnd, &rect);
	const UINT width = static_cast<UINT>(std::max<LONG>(1, rect.right - rect.left));
	const UINT height = static_cast<UINT>(std::max<LONG>(1, rect.bottom - rect.top));

	const D3D_FEATURE_LEVEL requestedLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL actualLevel = D3D_FEATURE_LEVEL_11_0;

	UINT flags = 0;
#if defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC swapDesc{};
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferCount = 1;
	swapDesc.OutputWindow = g_hwnd;
	swapDesc.Windowed = TRUE;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		requestedLevels,
		static_cast<UINT>(std::size(requestedLevels)),
		D3D11_SDK_VERSION,
		&swapDesc,
		&g_swapChain,
		&g_device,
		&actualLevel,
		&g_context);

	if (hr == E_INVALIDARG) {
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			flags,
			requestedLevels + 1,
			1,
			D3D11_SDK_VERSION,
			&swapDesc,
			&g_swapChain,
			&g_device,
			&actualLevel,
			&g_context);
	}

	if (FAILED(hr)) {
		const std::wstring text = std::format(
			L"D3D11CreateDeviceAndSwapChain failed: 0x{:08X}", static_cast<unsigned>(hr));
		MessageBoxW(g_hwnd, text.c_str(), L"Direct3D 11 Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!CreateRenderTarget()) {
		MessageBoxW(g_hwnd, L"Failed to create the Direct3D 11 render target.", L"Direct3D 11 Error", MB_OK | MB_ICONERROR);
		return false;
	}

	D3D11_BUFFER_DESC cameraDesc{};
	cameraDesc.ByteWidth = sizeof(CameraConstants);
	cameraDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = g_device->CreateBuffer(&cameraDesc, nullptr, &g_cameraBuffer);
	if (FAILED(hr)) {
		MessageBoxW(g_hwnd, L"Failed to create the camera constant buffer.", L"Direct3D 11 Error", MB_OK | MB_ICONERROR);
		return false;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = TRUE;
	hr = g_device->CreateRasterizerState(&rasterizerDesc, &g_rasterizerState);
	if (FAILED(hr)) {
		MessageBoxW(g_hwnd, L"Failed to create the Direct3D 11 rasterizer state.", L"Direct3D 11 Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!CompileVertexShader()) {
		return false;
	}

	g_defaultKernel = GetDefaultKernelSource();
	g_kernel = g_benchmarkMode ? g_defaultKernel : GetKernelSource();
	if (!CompileKernelShader(g_kernel, true)) {
		g_kernel = g_defaultKernel;
		if (!CompileKernelShader(g_kernel, true)) {
			return false;
		}
	}

	UpdateRenderViewport();
	return true;
}

void ShutdownD3D()
{
	if (g_context) {
		g_context->ClearState();
		g_context->Flush();
	}

	SafeReleaseT(g_rasterizerState);
	SafeReleaseT(g_cameraBuffer);
	SafeReleaseT(g_pixelShader);
	SafeReleaseT(g_vertexShader);
	ReleaseRenderTarget();
	SafeReleaseT(g_swapChain);
	SafeReleaseT(g_context);
	SafeReleaseT(g_device);
}

void UpdateCameraBuffer()
{
	if (!g_context || !g_cameraBuffer) {
		return;
	}

	const float cx = static_cast<float>(g_renderWidth);
	const float cy = static_cast<float>(g_renderHeight);
	const float shortSide = std::max(1.0f, std::min(cx, cy));

	CameraConstants constants{};
	const float cos1 = std::cos(g_ang1);
	const float sin1 = std::sin(g_ang1);
	const float cos2 = std::cos(g_ang2);
	const float sin2 = std::sin(g_ang2);

	constants.x = cx / shortSide;
	constants.y = cy / shortSide;
	constants.len = g_len;

	constants.origin[0] = g_len * cos1 * cos2 + g_cenx;
	constants.origin[1] = g_len * sin2 + g_ceny;
	constants.origin[2] = g_len * sin1 * cos2 + g_cenz;

	constants.right[0] = sin1;
	constants.right[1] = 0.0f;
	constants.right[2] = -cos1;

	constants.up[0] = -sin2 * cos1;
	constants.up[1] = cos2;
	constants.up[2] = -sin2 * sin1;

	constants.forward[0] = -cos1 * cos2;
	constants.forward[1] = -sin2;
	constants.forward[2] = -sin1 * cos2;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(g_context->Map(g_cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
		std::memcpy(mapped.pData, &constants, sizeof(constants));
		g_context->Unmap(g_cameraBuffer, 0);
	}
}

void Render()
{
	if (!g_context || !g_renderTarget || !g_vertexShader || !g_pixelShader || !g_swapChain) {
		return;
	}

	UpdateRenderViewport();
	UpdateCameraBuffer();

	const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
	g_context->ClearRenderTargetView(g_renderTarget, clearColor);

	// Fullscreen triangle generated from SV_VertexID; no VB/IA state required.
	g_context->IASetInputLayout(nullptr);
	g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_context->RSSetState(g_rasterizerState);
	g_context->VSSetShader(g_vertexShader, nullptr, 0);
	g_context->PSSetShader(g_pixelShader, nullptr, 0);
	g_context->VSSetConstantBuffers(0, 1, &g_cameraBuffer);
	g_context->PSSetConstantBuffers(0, 1, &g_cameraBuffer);
	g_context->Draw(3, 0);

	const UINT syncInterval = g_vsyncEnabled ? 1u : 0u;
	const HRESULT presentHr = g_swapChain->Present(syncInterval, 0);
	if (presentHr == DXGI_STATUS_OCCLUDED) {
		g_occluded = true;
		return;
	}
	g_occluded = false;

	if (SUCCEEDED(presentHr)) {
		RecordPresentedFrame();
	} else {
		g_renderFailed = true;
		HRESULT reason = g_device ? g_device->GetDeviceRemovedReason() : E_FAIL;
		const std::wstring text = std::format(
			L"Present failed: 0x{:08X}\nDevice removed reason: 0x{:08X}",
			static_cast<unsigned>(presentHr), static_cast<unsigned>(reason));
		MessageBoxW(g_hwnd, text.c_str(), L"Direct3D 11 Present Error", MB_OK | MB_ICONERROR);
	}
}

void WaitForInput(DWORD timeoutMs)
{
	MsgWaitForMultipleObjects(0, nullptr, FALSE, timeoutMs, QS_ALLINPUT);
}

void PumpRenderFrame()
{
	// Pausing only stops the auto-rotation; frames requested by user input are
	// still rendered so the camera stays responsive while paused.
	if (g_renderFailed || (g_paused && !g_needsRedraw)) {
		WaitForInput(INFINITE);
		return;
	}

	if (g_occluded) {
		WaitForInput(100);
	}

	LARGE_INTEGER now{};
	QueryPerformanceCounter(&now);

	if (g_frameRateLimit > 0 && !g_occluded) {
		const double frameInterval = 1.0 / static_cast<double>(g_frameRateLimit);
		const double elapsed = GetElapsedSeconds(g_lastFrameTime, now);
		if (elapsed < frameInterval) {
			const double remainingMs = (frameInterval - elapsed) * 1000.0;
			// Sleep through the coarse wait, then spin on subsequent iterations
			// for the final ~1.5 ms so scheduler granularity cannot cap FPS.
			if (remainingMs > 1.5) {
				Sleep(static_cast<DWORD>(remainingMs - 1.0));
			}
			return;
		}
	}

	g_lastFrameTime = now;
	if (!g_paused) {
		g_ang1 += kAutoRotationStep;
	}
	g_needsRedraw = false;
	Render();
}


void RemoveTrayIcon()
{
	if (!g_trayIconAdded) {
		return;
	}

	Shell_NotifyIconW(NIM_DELETE, &g_trayIcon);
	g_trayIconAdded = false;
	std::memset(&g_trayIcon, 0, sizeof(g_trayIcon));
}

bool AddTrayIcon()
{
	if (!g_hwnd || !g_hIconSmall) {
		return false;
	}

	std::memset(&g_trayIcon, 0, sizeof(g_trayIcon));
	g_trayIcon.cbSize = sizeof(g_trayIcon);
	g_trayIcon.hWnd = g_hwnd;
	g_trayIcon.uID = 1;
	g_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	g_trayIcon.uCallbackMessage = WMAPP_TRAYICON;
	g_trayIcon.hIcon = g_hIconSmall;
	wcscpy_s(g_trayIcon.szTip, L"vsbm for Windows");

	if (!Shell_NotifyIconW(NIM_ADD, &g_trayIcon)) {
		return false;
	}

	// Keep the classic callback-message semantics so lParam is the mouse
	// message (WM_LBUTTONUP, WM_RBUTTONUP, etc.) used below.
	g_trayIconAdded = true;
	return true;
}

void RestoreFromHideWhileWorking()
{
	if (!g_hiddenWhileWorking || !g_hwnd) {
		return;
	}

	LONG_PTR exStyle = g_exStyleBeforeHideWhileWorking;
	exStyle &= ~(static_cast<LONG_PTR>(WS_EX_TRANSPARENT) |
				 static_cast<LONG_PTR>(WS_EX_TOOLWINDOW));
	exStyle |= WS_EX_APPWINDOW;

	SetWindowLongPtrW(g_hwnd, GWL_EXSTYLE, exStyle);
	SetMainWindowAlpha(g_alpha);

	SetWindowPos(
		g_hwnd, nullptr,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
		SWP_NOZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);

	g_hiddenWhileWorking = false;
	ShowWindow(g_hwnd, SW_RESTORE);
	SetForegroundWindow(g_hwnd);
}

void HideWhileWorking()
{
	if (!g_hwnd || g_hiddenWhileWorking) {
		return;
	}

	if (g_hiddenToTaskbar) {
		return;
	}

	g_exStyleBeforeHideWhileWorking = GetWindowLongPtrW(g_hwnd, GWL_EXSTYLE);

	LONG_PTR exStyle = g_exStyleBeforeHideWhileWorking;
	exStyle |= WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
	exStyle &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);

	SetWindowLongPtrW(g_hwnd, GWL_EXSTYLE, exStyle);
	SetWindowPos(
		g_hwnd, nullptr,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

	SetLayeredWindowAttributes(g_hwnd, 0, 0, LWA_ALPHA);
	g_hiddenWhileWorking = true;
}

void RestoreFromHideToTaskbar()
{
	if (!g_hiddenToTaskbar || !g_hwnd) {
		return;
	}

	// Keep it paused during the visual restore animation so no new frame is
	// generated between the unhide/minimize/restore steps.
	const bool savedPauseState = g_pauseStateBeforeHideToTaskbar;
	g_paused = true;
	ApplyPausedTitle();

	// This intentionally mirrors the requested Windows animation sequence.
	ShowWindow(g_hwnd, SW_MINIMIZE);
	ShowWindow(g_hwnd, SW_RESTORE);

	g_hiddenToTaskbar = false;
	SetPaused(savedPauseState);
	SetMainWindowAlpha(g_alpha);
	SetForegroundWindow(g_hwnd);
}

void HideToTaskbar()
{
	if (!g_hwnd || g_hiddenToTaskbar) {
		return;
	}

	if (g_hiddenWhileWorking) {
		RestoreFromHideWhileWorking();
	}

	g_pauseStateBeforeHideToTaskbar = g_paused;
	SetPaused(true);

	// First minimize so Windows performs the normal taskbar animation,
	// then hide the already-minimized window.
	ShowWindow(g_hwnd, SW_MINIMIZE);
	ShowWindow(g_hwnd, SW_HIDE);
	g_hiddenToTaskbar = true;
}

void ShowTrayMenu()
{
	if (!g_hwnd) {
		return;
	}

	HMENU menu = CreatePopupMenu();
	if (!menu) {
		return;
	}

	AppendMenuW(menu, MF_STRING, IDM_TRAY_SHOW, L"&Show");
	AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, L"&Exit");

	POINT point{};
	GetCursorPos(&point);

	SetForegroundWindow(g_hwnd);
	const UINT command = TrackPopupMenuEx(
		menu,
		TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
		point.x,
		point.y,
		g_hwnd,
		nullptr);

	DestroyMenu(menu);
	PostMessageW(g_hwnd, WM_NULL, 0, 0);

	switch (command) {
	case IDM_TRAY_SHOW:
		if (g_hiddenWhileWorking) {
			RestoreFromHideWhileWorking();
		} else if (g_hiddenToTaskbar) {
			RestoreFromHideToTaskbar();
		} else {
			ShowWindow(g_hwnd, SW_RESTORE);
			SetForegroundWindow(g_hwnd);
		}
		break;

	case IDM_TRAY_EXIT:
		DestroyWindow(g_hwnd);
		break;

	default:
		break;
	}
}

LRESULT CALLBACK KernelDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE: {
		const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		const HINSTANCE instance = create ? create->hInstance : GetModuleHandleW(nullptr);
		const UINT dpi = GetWindowDpiSafe(hwnd);
		const int margin = ScaleForDpi(8, dpi);
		const int buttonWidth = ScaleForDpi(86, dpi);
		const int buttonHeight = ScaleForDpi(28, dpi);

		g_kernelDialogEdit = CreateWindowExW(
			WS_EX_CLIENTEDGE,
			L"EDIT",
			L"",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP |
				ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
				WS_VSCROLL | WS_HSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
			margin, margin, 680, 400,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_KERNEL_EDIT)),
			instance,
			nullptr);

		if (!g_kernelDialogEdit) {
			return -1;
		}

		g_kernelDialogApply = CreateWindowExW(
			0, L"BUTTON", L"&Apply",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_KERNEL_APPLY)),
			instance,
			nullptr);

		g_kernelDialogCancel = CreateWindowExW(
			0, L"BUTTON", L"&Cancel",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_KERNEL_CANCEL)),
			instance,
			nullptr);

		g_kernelDialogReset = CreateWindowExW(
			0, L"BUTTON", L"&Reset",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_KERNEL_RESET)),
			instance,
			nullptr);

		if (!g_kernelDialogApply || !g_kernelDialogCancel || !g_kernelDialogReset) {
			return -1;
		}

		SendMessageW(g_kernelDialogEdit, EM_SETLIMITTEXT, static_cast<WPARAM>(1024 * 1024), 0);

		// Win32's multiline EDIT expects CRLF for line breaks. Keep LF as the
		// program's canonical Kernel representation and convert only at the UI boundary.
		const std::wstring initialText = NormalizeNewlinesToCRLF(Utf8ToWide(g_kernel));
		SetWindowTextW(g_kernelDialogEdit, initialText.c_str());

		ApplyKernelDialogFont(hwnd);
		LayoutKernelDialog(hwnd);
		SetFocus(g_kernelDialogEdit);
		return 0;
	}

	case WM_SIZE:
		LayoutKernelDialog(hwnd);
		return 0;

	case WM_DPICHANGED: {
		const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
		if (suggested) {
			SetWindowPos(hwnd, nullptr,
				suggested->left,
				suggested->top,
				suggested->right - suggested->left,
				suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		ApplyKernelDialogFont(hwnd);
		LayoutKernelDialog(hwnd);
		return 0;
	}

	case WM_GETMINMAXINFO: {
		auto* minMax = reinterpret_cast<MINMAXINFO*>(lParam);
		const UINT dpi = GetWindowDpiSafe(hwnd);
		minMax->ptMinTrackSize.x = ScaleForDpi(560, dpi);
		minMax->ptMinTrackSize.y = ScaleForDpi(360, dpi);
		return 0;
	}

	case WM_SETFOCUS:
		if (g_kernelDialogEdit) {
			SetFocus(g_kernelDialogEdit);
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_KERNEL_APPLY: {
			const int length = g_kernelDialogEdit ? GetWindowTextLengthW(g_kernelDialogEdit) : 0;
			std::wstring text(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
			if (length > 0) {
				GetWindowTextW(g_kernelDialogEdit, &text[0], length + 1);
				text.resize(static_cast<size_t>(length));
			} else {
				text.clear();
			}

			text = NormalizeNewlinesToLF(text);
			const std::string newKernel = WideToUtf8(text);
			if (CompileKernelShader(newKernel, true)) {
				std::wstring saveError;
				if (!SaveKernelSourceToDisk(newKernel, &saveError)) {
					MessageBoxW(
						hwnd,
						(L"Kernel was compiled, but saving vsbm-windows.hlsl failed.\r\n\r\n" + saveError).c_str(),
						L"Kernel Save Error",
						MB_OK | MB_ICONERROR);
					return 0;
				}
				DestroyWindow(hwnd);
			}
			return 0;
		}

		case IDC_KERNEL_RESET: {
			const std::string defaultKernel = GetDefaultKernelSource();
			if (!CompileKernelShader(defaultKernel, true)) {
				return 0;
			}

			const std::wstring resetText = NormalizeNewlinesToCRLF(Utf8ToWide(defaultKernel));
			SetWindowTextW(g_kernelDialogEdit, resetText.c_str());
			SetFocus(g_kernelDialogEdit);

			const std::wstring kernelPath = GetExecutableDirectory() + L"\\vsbm-windows.hlsl";
			if (!DeleteFileW(kernelPath.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) {
				MessageBoxW(
					hwnd,
					L"The default Kernel was restored, but vsbm-windows.hlsl could not be deleted.",
					L"Kernel Reset",
					MB_OK | MB_ICONWARNING);
			}
			return 0;
		}

		case IDC_KERNEL_CANCEL:
			DestroyWindow(hwnd);
			return 0;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		g_kernelDialogEdit = nullptr;
		g_kernelDialogApply = nullptr;
		g_kernelDialogCancel = nullptr;
		g_kernelDialogReset = nullptr;
		if (g_kernelDialogFont) {
			DeleteObject(g_kernelDialogFont);
			g_kernelDialogFont = nullptr;
		}
		g_kernelDialog = nullptr;
		return 0;
	}

	return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterKernelDialogClass(HINSTANCE instance)
{
	static bool registered = false;
	if (registered) {
		return true;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = KernelDialogProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	wc.hIcon = g_hIcon;
	wc.hIconSm = g_hIconSmall;
	wc.lpszClassName = kKernelWindowClass;

	if (!RegisterClassExW(&wc)) {
		if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			return false;
		}
	}

	registered = true;
	return true;
}

void OpenKernelDialog()
{
	if (g_kernelDialog) {
		ShowWindow(g_kernelDialog, SW_SHOWNORMAL);
		SetForegroundWindow(g_kernelDialog);
		SetFocus(g_kernelDialogEdit);
		return;
	}

	const HINSTANCE instance = GetModuleHandleW(nullptr);
	if (!RegisterKernelDialogClass(instance)) {
		MessageBoxW(g_hwnd, L"Failed to register the Kernel editor window class.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	RECT owner{};
	GetWindowRect(g_hwnd, &owner);

	const UINT dpi = GetWindowDpiSafe(g_hwnd);
	const int width = ScaleForDpi(760, dpi);
	const int height = ScaleForDpi(560, dpi);

	g_kernelDialog = CreateWindowExW(
		0,
		kKernelWindowClass,
		L"Kernel - HLSL Editor",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		0, 0, width, height,
		g_hwnd,
		nullptr,
		instance,
		nullptr);

	if (!g_kernelDialog) {
		MessageBoxW(g_hwnd, L"Failed to create the Kernel editor window.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	centerWindow(g_kernelDialog, g_hwnd);
	ShowWindow(g_kernelDialog, SW_SHOWNORMAL);
	UpdateWindow(g_kernelDialog);
	SetForegroundWindow(g_kernelDialog);
	SetFocus(g_kernelDialogEdit);
}

HFONT CreateSettingsDialogFont(UINT dpi)
{
	const int height = -MulDiv(9, static_cast<int>(dpi), 72);
	return CreateFontW(
		height, 0, 0, 0,
		FW_NORMAL,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"Segoe UI");
}

void ApplySettingsDialogFont(HWND hwnd)
{
	if (!hwnd) {
		return;
	}

	const HFONT newFont = CreateSettingsDialogFont(GetWindowDpiSafe(hwnd));
	if (!newFont) {
		return;
	}

	HWND controls[] = {
		g_settingsFpsLabel,
		g_settingsFpsEdit,
		g_settingsVsyncCheck,
		g_settingsResolutionLabel,
		g_settingsResolutionCombo,
		g_settingsAllowOnlyOneInstanceCheck,
		g_settingsAllowOnlyOneInstanceWarning,
		g_settingsOk,
		g_settingsCancel,
		g_settingsReset,
	};
	for (HWND control : controls) {
		if (control) {
			SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
		}
	}

	if (g_settingsDialogFont) {
		DeleteObject(g_settingsDialogFont);
	}
	g_settingsDialogFont = newFont;
}

void LayoutSettingsDialog(HWND hwnd)
{
	RECT client{};
	GetClientRect(hwnd, &client);

	const UINT dpi = GetWindowDpiSafe(hwnd);
	const int margin = ScaleForDpi(12, dpi);
	const int gap = ScaleForDpi(8, dpi);
	const int rowHeight = ScaleForDpi(24, dpi);
	const int buttonWidth = ScaleForDpi(86, dpi);
	const int buttonHeight = ScaleForDpi(28, dpi);

	const int width = client.right - client.left;
	const int height = client.bottom - client.top;

	if (g_settingsFpsLabel) {
		SetWindowPos(g_settingsFpsLabel, nullptr,
			margin, margin,
			std::max(1, width - margin * 2), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsFpsEdit) {
		SetWindowPos(g_settingsFpsEdit, nullptr,
			margin, ScaleForDpi(42, dpi),
			ScaleForDpi(140, dpi), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsVsyncCheck) {
		SetWindowPos(g_settingsVsyncCheck, nullptr,
			margin, ScaleForDpi(82, dpi),
			std::max(1, width - margin * 2), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsResolutionLabel) {
		SetWindowPos(g_settingsResolutionLabel, nullptr,
			margin, ScaleForDpi(118, dpi),
			ScaleForDpi(70, dpi), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsResolutionCombo) {
		SetWindowPos(g_settingsResolutionCombo, nullptr,
			margin + ScaleForDpi(70, dpi) + ScaleForDpi(8, dpi), ScaleForDpi(118, dpi),
			ScaleForDpi(200, dpi), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsAllowOnlyOneInstanceCheck) {
		SetWindowPos(g_settingsAllowOnlyOneInstanceCheck, nullptr,
			margin, ScaleForDpi(152, dpi),
			std::max(1, width - margin * 2), rowHeight,
			SWP_NOZORDER);
	}
	if (g_settingsAllowOnlyOneInstanceWarning) {
		SetWindowPos(g_settingsAllowOnlyOneInstanceWarning, nullptr,
			margin, ScaleForDpi(180, dpi),
			std::max(1, width - margin * 2), ScaleForDpi(34, dpi),
			SWP_NOZORDER);
	}
	if (g_settingsOk) {
		SetWindowPos(g_settingsOk, nullptr,
			std::max(margin, width - margin - buttonWidth * 2 - gap),
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}
	if (g_settingsCancel) {
		SetWindowPos(g_settingsCancel, nullptr,
			std::max(margin, width - margin - buttonWidth),
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}
	if (g_settingsReset) {
		SetWindowPos(g_settingsReset, nullptr,
			margin,
			std::max(margin, height - margin - buttonHeight),
			buttonWidth, buttonHeight,
			SWP_NOZORDER);
	}
}

LRESULT CALLBACK SettingsDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE: {
		const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		const HINSTANCE instance = create ? create->hInstance : GetModuleHandleW(nullptr);
		const UINT dpi = GetWindowDpiSafe(hwnd);
		const int margin = ScaleForDpi(12, dpi);
		const int rowHeight = ScaleForDpi(24, dpi);
		const int buttonWidth = ScaleForDpi(86, dpi);
		const int buttonHeight = ScaleForDpi(28, dpi);

		g_settingsFpsLabel = CreateWindowExW(
			0, WC_STATICW, L"Frame rate (0 = unlimited):",
			WS_CHILD | WS_VISIBLE,
			margin, margin, ScaleForDpi(260, dpi), rowHeight,
			hwnd, nullptr, instance, nullptr);

		g_settingsFpsEdit = CreateWindowExW(
			WS_EX_CLIENTEDGE, L"EDIT", L"",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
			0, 0, ScaleForDpi(140, dpi), rowHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_FPS)),
			instance,
			nullptr);

		g_settingsVsyncCheck = CreateWindowExW(
			0, WC_BUTTONW, L"Enable &vertical sync",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
			0, 0, ScaleForDpi(240, dpi), rowHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_VSYNC)),
			instance,
			nullptr);

		g_settingsResolutionLabel = CreateWindowExW(
			0, WC_STATICW, L"Resolution:",
			WS_CHILD | WS_VISIBLE,
			0, 0, ScaleForDpi(260, dpi), rowHeight,
			hwnd, nullptr, instance, nullptr);

		g_settingsResolutionCombo = CreateWindowExW(
			0, WC_COMBOBOXW, nullptr,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
			CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
			0, 0, ScaleForDpi(200, dpi), rowHeight + ScaleForDpi(170, dpi),
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_RESOLUTION)),
			instance,
			nullptr);

		g_settingsAllowOnlyOneInstanceCheck = CreateWindowExW(
			0, WC_BUTTONW, L"Allow only one instance",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
			0, 0, ScaleForDpi(280, dpi), rowHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_ALLOW_ONLY_ONE_INSTANCE)),
			instance,
			nullptr);

		g_settingsAllowOnlyOneInstanceWarning = CreateWindowExW(
			0, WC_STATICW, L"Warning: Running multiple instances may seriously\nstress your hardware!",
			WS_CHILD | WS_VISIBLE,
			0, 0, ScaleForDpi(340, dpi), ScaleForDpi(34, dpi),
			hwnd, nullptr, instance, nullptr);

		g_settingsOk = CreateWindowExW(
			0, WC_BUTTONW, L"&OK",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_OK)),
			instance,
			nullptr);

		g_settingsCancel = CreateWindowExW(
			0, WC_BUTTONW, L"&Cancel",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_CANCEL)),
			instance,
			nullptr);

		g_settingsReset = CreateWindowExW(
			0, WC_BUTTONW, L"Reset...",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, buttonWidth, buttonHeight,
			hwnd,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SETTINGS_RESET)),
			instance,
			nullptr);

		if (!g_settingsFpsLabel || !g_settingsFpsEdit || !g_settingsVsyncCheck ||
			!g_settingsResolutionLabel || !g_settingsResolutionCombo ||
			!g_settingsAllowOnlyOneInstanceCheck || !g_settingsAllowOnlyOneInstanceWarning ||
			!g_settingsOk || !g_settingsCancel || !g_settingsReset) {
			return -1;
		}

		SetWindowTextW(g_settingsFpsEdit, std::format(L"{}", g_frameRateLimit).c_str());
		Button_SetCheck(g_settingsVsyncCheck, g_vsyncEnabled ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(g_settingsAllowOnlyOneInstanceCheck, g_allowOnlyOneInstance ? BST_CHECKED : BST_UNCHECKED);

		for (int i = 0; i < static_cast<int>(std::size(kResolutionOptions)); ++i) {
			wchar_t optionLabel[32]{};
			FormatResolutionOptionLabel(i, optionLabel, std::size(optionLabel));
			SendMessageW(g_settingsResolutionCombo, CB_ADDSTRING, 0,
				reinterpret_cast<LPARAM>(optionLabel));
		}
		const int initialResolution = FindResolutionOption(g_resolutionWidth, g_resolutionHeight);
		SendMessageW(g_settingsResolutionCombo, CB_SETCURSEL, initialResolution, 0);
		g_settingsLastResolutionIndex = initialResolution;

		ApplySettingsDialogFont(hwnd);
		LayoutSettingsDialog(hwnd);
		SetFocus(g_settingsFpsEdit);
		return 0;
	}

	case WM_MEASUREITEM: {
		auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
		if (measure && measure->CtlType == ODT_COMBOBOX) {
			measure->itemHeight = ScaleForDpi(20, GetWindowDpiSafe(hwnd));
			return TRUE;
		}
		break;
	}

	case WM_DRAWITEM: {
		const auto* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
		if (!draw || draw->CtlType != ODT_COMBOBOX) {
			break;
		}

		const int originalItem = static_cast<int>(draw->itemID);
		int item = originalItem;
		const bool separator = IsResolutionSeparator(item);
		if (item < 0) {
			item = static_cast<int>(SendMessageW(draw->hwndItem, CB_GETCURSEL, 0, 0));
		}

		wchar_t optionLabel[32]{};
		if (item >= 0) {
			SendMessageW(draw->hwndItem, CB_GETLBTEXT, static_cast<WPARAM>(item),
				reinterpret_cast<LPARAM>(optionLabel));
		}

		COLORREF textColor = GetSysColor(COLOR_WINDOWTEXT);
		HBRUSH background = GetSysColorBrush(COLOR_WINDOW);
		if (separator) {
			textColor = GetSysColor(COLOR_GRAYTEXT);
			background = GetSysColorBrush(COLOR_BTNFACE);
		} else if (originalItem >= 0 && (draw->itemState & ODS_SELECTED)) {
			textColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
			background = GetSysColorBrush(COLOR_HIGHLIGHT);
		}

		FillRect(draw->hDC, &draw->rcItem, background);
		SetBkMode(draw->hDC, TRANSPARENT);
		SetTextColor(draw->hDC, textColor);
		RECT textRect = draw->rcItem;
		const int padding = ScaleForDpi(6, GetWindowDpiSafe(hwnd));
		textRect.left += padding;
		textRect.right -= padding;
		DrawTextW(draw->hDC, optionLabel, -1, &textRect,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		return TRUE;
	}

	case WM_SIZE:
		LayoutSettingsDialog(hwnd);
		return 0;

	case WM_DPICHANGED: {
		const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
		if (suggested) {
			SetWindowPos(hwnd, nullptr,
				suggested->left,
				suggested->top,
				suggested->right - suggested->left,
				suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		ApplySettingsDialogFont(hwnd);
		LayoutSettingsDialog(hwnd);
		return 0;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_RESOLUTION:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const int selected = static_cast<int>(
					SendMessageW(g_settingsResolutionCombo, CB_GETCURSEL, 0, 0));
				if (IsResolutionSeparator(selected)) {
					SendMessageW(g_settingsResolutionCombo, CB_SETCURSEL,
						g_settingsLastResolutionIndex, 0);
				} else {
					g_settingsLastResolutionIndex = selected;
				}
			}
			return 0;

		case IDC_SETTINGS_RESET: {
			int user = IDNO;
			TaskDialog(
				hwnd,
				nullptr,
				L"Reset settings",
				L"Are you sure you want to reset everything?",
				L"All preferences and statistics data will be reset to their initial state. Kernel shader file will not be affected.",
				TDCBF_YES_BUTTON | TDCBF_CANCEL_BUTTON,
				TD_WARNING_ICON,
				&user);
			if (user == IDYES) {
				if (!DeleteUserIniFile()) {
					MessageBoxW(hwnd, L"Cannot delete vsbm-windows.ini.", L"Reset settings", MB_OK | MB_ICONERROR);
					return 0;
				}
				ResetUserPreferences();
				SetMainWindowAlpha(g_alpha);
				ApplyResolutionStyleAndSize();
				if (g_hwnd) {
					centerWindow(g_hwnd, 0);
				}
				//DestroyWindow(hwnd);
				PROCESS_INFORMATION pi{};
				if (LaunchSelf(L"", false, pi) && pi.hProcess && pi.hThread) {
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
				}
				ExitProcess(0);
			}
			return 0;
		}

		case IDC_SETTINGS_OK: {
			wchar_t buffer[16]{};
			GetWindowTextW(g_settingsFpsEdit, buffer, static_cast<int>(std::size(buffer)));

			wchar_t* end = nullptr;
			const unsigned long parsed = wcstoul(buffer, &end, 10);
			if (end == buffer || *end != L'\0' || parsed > kMaxFrameRateLimit) {
				MessageBoxW(
					hwnd,
					L"Enter an integer between 0 and 1000000. 0 means unlimited.",
					L"Invalid Frame Rate",
					MB_OK | MB_ICONWARNING);
				SetFocus(g_settingsFpsEdit);
				return 0;
			}

			g_frameRateLimit = static_cast<UINT>(parsed);
			g_vsyncEnabled = Button_GetCheck(g_settingsVsyncCheck) == BST_CHECKED;
			g_allowOnlyOneInstance = Button_GetCheck(g_settingsAllowOnlyOneInstanceCheck) == BST_CHECKED;

			const int resolutionIndex = static_cast<int>(
				SendMessageW(g_settingsResolutionCombo, CB_GETCURSEL, 0, 0));
			if (resolutionIndex > 0 && !IsResolutionSeparator(resolutionIndex)) {
				const ResolutionOption& option = kResolutionOptions[resolutionIndex];
				g_resolutionWidth = option.width;
				g_resolutionHeight = option.height;
			} else {
				g_resolutionWidth = 0;
				g_resolutionHeight = 0;
			}

			ApplyResolutionStyleAndSize();
			MarkIniDirty();
			SaveWindowSettings();
			DestroyWindow(hwnd);
			return 0;
		}

		case IDC_SETTINGS_CANCEL:
			DestroyWindow(hwnd);
			return 0;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		g_settingsFpsLabel = nullptr;
		g_settingsFpsEdit = nullptr;
		g_settingsVsyncCheck = nullptr;
		g_settingsResolutionLabel = nullptr;
		g_settingsResolutionCombo = nullptr;
		g_settingsAllowOnlyOneInstanceCheck = nullptr;
		g_settingsAllowOnlyOneInstanceWarning = nullptr;
		g_settingsOk = nullptr;
		g_settingsCancel = nullptr;
		g_settingsReset = nullptr;
		if (g_settingsDialogFont) {
			DeleteObject(g_settingsDialogFont);
			g_settingsDialogFont = nullptr;
		}
		g_settingsDialog = nullptr;
		return 0;
	}

	return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterSettingsDialogClass(HINSTANCE instance)
{
	static bool registered = false;
	if (registered) {
		return true;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = SettingsDialogProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	wc.hIcon = g_hIcon;
	wc.hIconSm = g_hIconSmall;
	wc.lpszClassName = kSettingsWindowClass;

	if (!RegisterClassExW(&wc)) {
		if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			return false;
		}
	}

	registered = true;
	return true;
}

void OpenSettingsDialog()
{
	if (g_settingsDialog) {
		ShowWindow(g_settingsDialog, SW_SHOWNORMAL);
		SetForegroundWindow(g_settingsDialog);
		SetFocus(g_settingsFpsEdit);
		return;
	}

	const HINSTANCE instance = GetModuleHandleW(nullptr);
	if (!RegisterSettingsDialogClass(instance)) {
		MessageBoxW(g_hwnd, L"Failed to register the Settings window class.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	const UINT dpi = GetWindowDpiSafe(g_hwnd);
	const int width = ScaleForDpi(380, dpi);
	const int height = ScaleForDpi(320, dpi);

	g_settingsDialog = CreateWindowExW(
		0,
		kSettingsWindowClass,
		L"Settings",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
		0, 0, width, height,
		g_hwnd,
		nullptr,
		instance,
		nullptr);

	if (!g_settingsDialog) {
		MessageBoxW(g_hwnd, L"Failed to create the Settings window.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	centerWindow(g_settingsDialog, g_hwnd);
	ShowWindow(g_settingsDialog, SW_SHOWNORMAL);
	UpdateWindow(g_settingsDialog);
	SetForegroundWindow(g_settingsDialog);
	SetFocus(g_settingsFpsEdit);
}

bool DoesUserUsesChinese() {
	static auto langID = PRIMARYLANGID(GetUserDefaultUILanguage());
#pragma warning(push)
#pragma warning(disable: 6287)
	return langID == LANG_CHINESE ||
		langID == LANG_CHINESE_SIMPLIFIED ||
		langID == LANG_CHINESE_TRADITIONAL;
#pragma warning(pop)
}

void ShowFirstRunWelcome(HWND hwnd) {
	TaskDialog(
		hwnd,
		nullptr,
		L"vsbm for Windows",
		L"Welcome to this application!",
		DoesUserUsesChinese() ? L"提示：右键单击标题栏或按 Alt + Space 即可显示更多选项或调整渲染分辨率。" :
		L"Tip: Right-click the title bar or press Alt+Space to show more options or adjust the resolution.",
		TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON,
		TD_INFORMATION_ICON,
		nullptr);
}

void ShowStatistics(HWND hwnd)
{
	const double activeSeconds = GetActiveSeconds();
	const double averageFps = activeSeconds > 0.0
		? static_cast<double>(g_totalPresentedFrames) / activeSeconds
		: 0.0;

	const bool benchmark = g_benchmarkMode;
	const double historicalMax = benchmark
		? std::max(g_maxFpsBenchmarkHistory, g_maxFps)
		: std::max(g_maxFpsHistory, g_maxFps);

	std::wstring text;
	if (benchmark) {
		text = std::format(
			L"Total frames rendered: {}\r\n"
			L"Average frame rate: {:.2f} FPS\r\n"
			L"Session maximum frame rate: {:.2f} FPS\r\n"
			L"Historical maximum frame rate (benchmark mode): {:.2f} FPS\r\n",
			static_cast<unsigned long long>(g_totalPresentedFrames),
			averageFps,
			g_maxFps,
			historicalMax);
		TaskDialog(hwnd, nullptr, L"Benchmark statistics - vsbm for Windows",
			L"Benchmark statistics for this session.",
			text.c_str(), TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, nullptr);
		return;
	}

	text = std::format(
		L"Total frames rendered: {}\r\n"
		L"Average frame rate: {:.2f} FPS\r\n"
		L"Session maximum frame rate: {:.2f} FPS\r\n"
		L"Historical maximum frame rate: {:.2f} FPS\r\n",
		static_cast<unsigned long long>(g_totalPresentedFrames),
		averageFps,
		g_maxFps,
		historicalMax);

	TaskDialog(hwnd, nullptr, L"Statistics - vsbm for Windows",
		L"Rendering statistics since application startup.",
		text.c_str(), TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, nullptr);
}

void ReleasePreviewBitmap()
{
#ifndef MyGDIPlusNoGDIPlus
	if (g_previewBitmap) {
		delete g_previewBitmap;
		g_previewBitmap = nullptr;
	}
#endif
	if (g_previewStream) {
		g_previewStream->Release();
		g_previewStream = nullptr;
	}
}

bool LoadPreviewBitmap()
{
	ReleasePreviewBitmap();

	const HMODULE module = GetModuleHandleW(nullptr);
	const HRSRC resource = 0;//FindResourceW(module, MAKEINTRESOURCEW(IDB_PNG1), L"PNG");
	if (!resource) {
		return false;
	}

	const DWORD resourceSize = SizeofResource(module, resource);
	const HGLOBAL loaded = LoadResource(module, resource);
	if (!loaded || resourceSize == 0) {
		return false;
	}

	const void* resourceData = LockResource(loaded);
	if (!resourceData) {
		return false;
	}

	IStream* stream = SHCreateMemStream(static_cast<const BYTE*>(resourceData), resourceSize);
	if (!stream) {
		return false;
	}

#ifndef MyGDIPlusNoGDIPlus
	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(stream, FALSE);
	if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
		delete bitmap;
		stream->Release();
		return false;
	}
#endif

	g_previewStream = stream;
#ifndef MyGDIPlusNoGDIPlus
	g_previewBitmap = bitmap;
#endif
	return true;
}

LRESULT CALLBACK RenderPreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DPICHANGED: {
		const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
		if (suggested) {
			SetWindowPos(hwnd, nullptr,
				suggested->left,
				suggested->top,
				suggested->right - suggested->left,
				suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		return 0;
	}

	case WM_SIZE:
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT: {
		PAINTSTRUCT paint{};
		HDC hdc = BeginPaint(hwnd, &paint);

		RECT client{};
		GetClientRect(hwnd, &client);
		FillRect(hdc, &client, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

#ifndef MyGDIPlusNoGDIPlus
		if (g_previewBitmap) {
			const int clientWidth = client.right - client.left;
			const int clientHeight = client.bottom - client.top;
			const UINT imageWidth = g_previewBitmap->GetWidth();
			const UINT imageHeight = g_previewBitmap->GetHeight();
			if (imageWidth > 0 && imageHeight > 0) {
				const double scale = std::min(
					static_cast<double>(clientWidth) / imageWidth,
					static_cast<double>(clientHeight) / imageHeight);
				const int drawWidth = std::max(1, static_cast<int>(imageWidth * scale));
				const int drawHeight = std::max(1, static_cast<int>(imageHeight * scale));

				Gdiplus::Graphics graphics(hdc);
				graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
				graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
				graphics.DrawImage(
					g_previewBitmap,
					(clientWidth - drawWidth) / 2,
					(clientHeight - drawHeight) / 2,
					drawWidth,
					drawHeight);
			}
		}
#endif

		EndPaint(hwnd, &paint);
		return 0;
	}

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		ReleasePreviewBitmap();
		g_previewWindow = nullptr;
		return 0;
	}

	return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterRenderPreviewClass(HINSTANCE instance)
{
	static bool registered = false;
	if (registered) {
		return true;
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RenderPreviewProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hIcon = g_hIcon;
	wc.hIconSm = g_hIconSmall;
	wc.lpszClassName = kPreviewWindowClass;

	if (!RegisterClassExW(&wc)) {
		if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			return false;
		}
	}

	registered = true;
	return true;
}

void OpenRenderPreview()
{
	if (g_previewWindow) {
		ShowWindow(g_previewWindow, SW_SHOWNORMAL);
		SetForegroundWindow(g_previewWindow);
		return;
	}

#if 0
	int u = MessageBoxW(g_hwnd, L"Render preview has been removed because the high entropy PNG is always recognized"
		L" as malicious payload.\nIf you want to preview the render effect, you can open the online resource.\n"
		L"Do you want to open the online preview?", NULL, MB_ICONERROR | MB_OKCANCEL);
#else
	int u = IDCANCEL;
#endif
	if (u == IDCANCEL) return;
	else if (u == IDOK) {
		//OpenUrl(g_kOnlinePreviewUrl, g_hwnd);
		return;
	}

	if (!LoadPreviewBitmap()) {
		MessageBoxW(g_hwnd, L"Failed to load the embedded preview image.", L"Render preview", MB_OK | MB_ICONERROR);
		return;
	}

	const HINSTANCE instance = GetModuleHandleW(nullptr);
	if (!RegisterRenderPreviewClass(instance)) {
		ReleasePreviewBitmap();
		MessageBoxW(g_hwnd, L"Failed to register the Render preview window class.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

#ifndef MyGDIPlusNoGDIPlus
	int clientWidth = static_cast<int>(g_previewBitmap->GetWidth());
	int clientHeight = static_cast<int>(g_previewBitmap->GetHeight());
#else
	int clientWidth = 100;
	int clientHeight = 100;
#endif

	// Cap the initial window to 80% of the monitor work area so a large bitmap still opens on screen.
	const HMONITOR monitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo{};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (monitor && GetMonitorInfoW(monitor, &monitorInfo)) {
		const int maxWidth = (monitorInfo.rcWork.right - monitorInfo.rcWork.left) * 4 / 5;
		const int maxHeight = (monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) * 4 / 5;
		if (clientWidth > maxWidth || clientHeight > maxHeight) {
			const double scale = std::min(
				static_cast<double>(maxWidth) / std::max(1, clientWidth),
				static_cast<double>(maxHeight) / std::max(1, clientHeight));
			clientWidth = static_cast<int>(clientWidth * scale);
			clientHeight = static_cast<int>(clientHeight * scale);
		}
	}

	RECT windowRect{0, 0, clientWidth, clientHeight};
	AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);

	g_previewWindow = CreateWindowExW(
		0,
		kPreviewWindowClass,
		L"Render preview",
		WS_OVERLAPPEDWINDOW,
		0, 0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		g_hwnd,
		nullptr,
		instance,
		nullptr);

	if (!g_previewWindow) {
		ReleasePreviewBitmap();
		MessageBoxW(g_hwnd, L"Failed to create the Render preview window.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	centerWindow(g_previewWindow, g_hwnd);
	ShowWindow(g_previewWindow, SW_SHOWNORMAL);
	UpdateWindow(g_previewWindow);
	SetForegroundWindow(g_previewWindow);
}

void AddKernelMenuItem(HWND hwnd)
{
	HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
	if (!systemMenu) {
		return;
	}

	AppendMenuW(systemMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(systemMenu, g_benchmarkMode ? MF_GRAYED : MF_STRING, IDM_KERNEL, L"&Kernel...");
	AppendMenuW(systemMenu, g_benchmarkMode ? MF_GRAYED : MF_STRING, IDM_RESET_CAMERA, L"Reset camera (&F)");
	AppendMenuW(systemMenu, MF_STRING, IDM_BENCHMARK_MODE,
		g_benchmarkMode ? L"Leave &benchmark mode" : L"&Benchmark mode");
	AppendMenuW(systemMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(systemMenu, MF_STRING, IDM_HIDE_TO_TASKBAR, L"Hide to t&askbar");
	AppendMenuW(systemMenu, g_benchmarkMode ? MF_GRAYED : MF_STRING, IDM_HIDE_WHILE_WORKING, L"Hide while &working");
	AppendMenuW(systemMenu, MF_SEPARATOR, 0, nullptr);
	//AppendMenuW(systemMenu, MF_STRING, IDM_RENDER_PREVIEW, L"Render preview");
	AppendMenuW(systemMenu, MF_STRING, IDM_STATISTICS, L"S&tatistics");
	AppendMenuW(systemMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(systemMenu, g_benchmarkMode ? MF_GRAYED : MF_STRING, IDM_SETTINGS, L"S&ettings...");
	AppendMenuW(systemMenu, MF_STRING, IDM_HELP, L"&Help...");
}


bool LaunchSelf(const std::wstring& arguments, bool suspended, PROCESS_INFORMATION& processInfo)
{
	processInfo = {};
	std::vector<wchar_t> path(32768, L'\0');
	const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
	if (length == 0 || length >= path.size()) {
		return false;
	}

	std::wstring commandLine = L"-";
	if (!arguments.empty()) {
		commandLine += L" ";
		commandLine += arguments;
	}

	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	DWORD creationFlags = CREATE_DEFAULT_ERROR_MODE;
	if (suspended) {
		creationFlags |= CREATE_SUSPENDED;
	}

	return CreateProcessW(
		path.data(), commandLine.data(), nullptr, nullptr, FALSE, creationFlags,
		nullptr, nullptr, &startupInfo, &processInfo) != FALSE;
}

void ResetCameraAndRefresh()
{
	if (g_benchmarkMode) {
		return;
	}
	ResetCamera();
	UpdateWindowTitle();
	g_needsRedraw = true;
}

void StartBenchmarkMode(HWND hwnd)
{
	const int user = MessageBoxW(
		hwnd,
		L"Switch to benchmark mode? This will reset the current session's statistics data.",
		L"Benchmark mode",
		MB_ICONQUESTION | MB_OKCANCEL);
	if (user != IDOK) {
		return;
	}

	PROCESS_INFORMATION pi{};
	if (!LaunchSelf(L"--benchmark", false, pi) || !pi.hThread || !pi.hProcess) {
		MessageBoxW(hwnd, L"Cannot start benchmark mode.", L"Benchmark mode", MB_OK | MB_ICONERROR);
		return;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	DestroyWindow(hwnd);
}

void LeaveBenchmarkMode(HWND hwnd)
{
	if (!g_benchmarkMode || g_benchmarkTransitioning) {
		return;
	}

	SetPaused(true);
	ShowStatistics(hwnd);
	SaveBenchmarkStatistics();

	PROCESS_INFORMATION pi{};
	if (!LaunchSelf(L"", true, pi) || !pi.hThread || !pi.hProcess) {
		MessageBoxW(hwnd,
			L"Cannot leave benchmark mode because the normal application could not be started.",
			L"Benchmark mode", MB_OK | MB_ICONERROR);
		SetPaused(false);
		return;
	}

	g_benchmarkTransitioning = true;
	DestroyWindow(hwnd);
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

void UpdateMouseButtonsFromCapture()
{
	if (!g_leftDown && !g_rightDown) {
		ReleaseCapture();
	}
}

int FindTouchIndex(DWORD id)
{
	for (int i = 0; i < 2; ++i) {
		if (g_touches[i].active && g_touches[i].id == id) {
			return i;
		}
	}
	return -1;
}

int FindFreeTouchIndex()
{
	for (int i = 0; i < 2; ++i) {
		if (!g_touches[i].active) {
			return i;
		}
	}
	return -1;
}

int ActiveTouchCount() {
	return static_cast<int>(g_touches[0].active) + static_cast<int>(g_touches[1].active);
}

void GetTouchPair(std::array<TouchPoint, 2>& out, int& count)
{
	count = 0;
	for (const TouchPoint& touch : g_touches) {
		if (touch.active && count < 2) {
			out[static_cast<size_t>(count++)] = touch;
		}
	}
}

void HandleTouch(HWND hwnd, LPARAM lParam)
{
	const UINT count = LOWORD(lParam);
	std::vector<TOUCHINPUT> inputs(count);
	HTOUCHINPUT touchHandle = reinterpret_cast<HTOUCHINPUT>(lParam);
	if (!GetTouchInputInfo(touchHandle, count, inputs.data(), sizeof(TOUCHINPUT))) {
		return;
	}

	std::array<TouchPoint, 2> oldTouches{};
	int oldCount = 0;
	GetTouchPair(oldTouches, oldCount);

	bool moved = false;

	for (const TOUCHINPUT& ti : inputs) {
		POINT point{
			static_cast<LONG>(ti.x / 100),
			static_cast<LONG>(ti.y / 100)
		};
		ScreenToClient(hwnd, &point);

		if (ti.dwFlags & TOUCHEVENTF_DOWN) {
			int index = FindTouchIndex(ti.dwID);
			if (index < 0) {
				index = FindFreeTouchIndex();
			}
			if (index >= 0) {
				g_touches[index].active = true;
				g_touches[index].id = ti.dwID;
				g_touches[index].x = static_cast<float>(point.x);
				g_touches[index].y = static_cast<float>(point.y);
			}
		}

		if (ti.dwFlags & TOUCHEVENTF_MOVE) {
			const int index = FindTouchIndex(ti.dwID);
			if (index >= 0) {
				if (std::fabs(g_touches[index].x - point.x) > 0.001f ||
					std::fabs(g_touches[index].y - point.y) > 0.001f) {
					moved = true;
				}
				g_touches[index].x = static_cast<float>(point.x);
				g_touches[index].y = static_cast<float>(point.y);
			}
		}
	}

	const int newCountBeforeUp = ActiveTouchCount();
	std::array<TouchPoint, 2> newTouches{};
	int newCount = 0;
	GetTouchPair(newTouches, newCount);

	if (oldCount == 1 && newCount == 1 && moved) {
		const float dx = newTouches[0].x - oldTouches[0].x;
		const float dy = newTouches[0].y - oldTouches[0].y;
		g_ang1 += dx * 0.002f;
		g_ang2 += dy * 0.002f;
	} else if (oldCount == 2 && newCount == 2 && moved) {
		const float oldSumX = oldTouches[0].x + oldTouches[1].x;
		const float oldSumY = oldTouches[0].y + oldTouches[1].y;
		const float newSumX = newTouches[0].x + newTouches[1].x;
		const float newSumY = newTouches[0].y + newTouches[1].y;
		const float deltaX = newSumX - oldSumX;
		const float deltaY = newSumY - oldSumY;

		const float cx = static_cast<float>(g_renderWidth);
		const float cy = static_cast<float>(g_renderHeight);
		const float l = g_len * 2.0f / std::max(1.0f, cx + cy);

		g_cenx += l * (-deltaX * std::sin(g_ang1) - deltaY * std::sin(g_ang2) * std::cos(g_ang1));
		g_ceny += l * ( deltaY * std::cos(g_ang2));
		g_cenz += l * ( deltaX * std::cos(g_ang1) - deltaY * std::sin(g_ang2) * std::sin(g_ang1));

		const float oldDist = std::sqrt(
			(oldTouches[0].x - oldTouches[1].x) * (oldTouches[0].x - oldTouches[1].x) +
			(oldTouches[0].y - oldTouches[1].y) * (oldTouches[0].y - oldTouches[1].y) + 1.0f);
		const float newDist = std::sqrt(
			(newTouches[0].x - newTouches[1].x) * (newTouches[0].x - newTouches[1].x) +
			(newTouches[0].y - newTouches[1].y) * (newTouches[0].y - newTouches[1].y) + 1.0f);
		if (newDist > 0.001f) {
			g_len *= oldDist / newDist;
		}
	}

	for (const TOUCHINPUT& ti : inputs) {
		if (ti.dwFlags & TOUCHEVENTF_UP) {
			const int index = FindTouchIndex(ti.dwID);
			if (index >= 0) {
				g_touches[index] = {};
			}
		}
	}

	(void)newCountBeforeUp;
	CloseTouchInputHandle(touchHandle);
}

LRESULT CALLBACK VerySimpleLicenseViewerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_taskbarCreatedMessage && message == g_taskbarCreatedMessage) {
		AddTrayIcon();
		return 0;
	}

	switch (message) {
	case WM_CREATE:
		RegisterTouchWindow(hwnd, 0);
		SetLayeredWindowAttributes(hwnd, 0, g_alpha, LWA_ALPHA);
		return 0;

	case WM_DPICHANGED:
		if (g_resolutionWidth > 0 && g_resolutionHeight > 0) {
			ApplyResolutionStyleAndSize();
		} else if (!IsZoomed(hwnd) && !IsIconic(hwnd)) {
			const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
			if (suggested) {
				SetWindowPos(hwnd, nullptr,
					suggested->left,
					suggested->top,
					suggested->right - suggested->left,
					suggested->bottom - suggested->top,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
		MarkIniDirty();
		SaveWindowSettings();
		return 0;

	case WM_SIZING: {
		const auto* proposed = reinterpret_cast<const RECT*>(lParam);
		const SIZE clientSize = GetClientSizeForWindowRect(hwnd, *proposed);
		UpdateWindowTitle(clientSize.cx, clientSize.cy);
		break;
	}

	case WM_SIZE:
		if (g_benchmarkMode) {
			if (wParam == SIZE_MINIMIZED) {
				if (!g_paused) {
					g_benchmarkAutoPausedByMinimize = true;
					SetPaused(true);
				}
			} else if (g_benchmarkAutoPausedByMinimize) {
				g_benchmarkAutoPausedByMinimize = false;
				SetPaused(false);
			}
		}

		if (g_device && wParam != SIZE_MINIMIZED) {
			const UINT width = static_cast<UINT>(std::max<LONG>(1, LOWORD(lParam)));
			const UINT height = static_cast<UINT>(std::max<LONG>(1, HIWORD(lParam)));
			if (ResizeSwapChain(width, height)) {
				Render();
			}
			UpdateWindowTitle(static_cast<int>(width), static_cast<int>(height));
		}
		return 0;

	case WM_SYSCOMMAND: {
		const UINT_PTR command = static_cast<UINT_PTR>(wParam);
		if (command == IDM_KERNEL) {
			if (g_benchmarkMode) return 0;
			OpenKernelDialog();
			return 0;
		}
		if (command == IDM_RESET_CAMERA) {
			ResetCameraAndRefresh();
			return 0;
		}
		if (command == IDM_BENCHMARK_MODE) {
			if (g_benchmarkMode) {
				LeaveBenchmarkMode(hwnd);
			} else {
				StartBenchmarkMode(hwnd);
			}
			return 0;
		}
		if (command == IDM_HIDE_TO_TASKBAR) {
			HideToTaskbar();
			return 0;
		}
		if (command == IDM_HIDE_WHILE_WORKING) {
			if (g_benchmarkMode) return 0;
			HideWhileWorking();
			return 0;
		}
		if (command == IDM_RENDER_PREVIEW) {
			OpenRenderPreview();
			return 0;
		}
		if (command == IDM_STATISTICS) {
			ShowStatistics(hwnd);
			return 0;
		}
		if (command == IDM_SETTINGS) {
			if (g_benchmarkMode) return 0;
			OpenSettingsDialog();
			return 0;
		}
		if (command == IDM_HELP) {
			TASKDIALOGCONFIG cfg{};
			AppVersion ver = GetSelfVersion();
			std::wstring content = (
				L"Press Space to pause/resume animation.\r\n"
				L"Press left button and move to rotate.\r\n"
				L"Press right button to move the view.\r\n"
				L"Scroll the wheel to zoom.\r\n"
				L"Press Up to decrease opacity, or Down to increase it.\r\n"
				L"Use the system menu to edit the Kernel, open Settings, view statistics, or hide the window.\r\n"
				L"The notification-area icon can restore the window or exit the application.\r\n"
				L"\r\nThanks for using this application!"
				L"\r\nOriginal webpage: " + std::wstring(g_kOriginalUrl) +
				L"\r\nWindows version " + std::format(L"{}.{}.{}.{}", ver.major, ver.minor, ver.build, ver.revision) +
				L" by: " + std::wstring(g_kProductUrl) +
				L" , GPL-3.0 License."
#ifndef _WIN64
				+ L"\r\nYou're currently using the 32 bit version of the application."
#endif
			).c_str();
			const TASKDIALOG_BUTTON buttons[] = {
				{0x1001, L"Open repository"},
				{0x1002, L"Open original webpage"},
				{0x1003, L"Show license"},
			};
			cfg.cbSize = sizeof(TASKDIALOGCONFIG);
			cfg.pszWindowTitle = L"Help - vsbm for Windows";
			cfg.pszMainInstruction = L"Here is the help document.";
			cfg.pszContent = content.c_str();
			cfg.pszMainIcon = TD_INFORMATION_ICON;
			cfg.cButtons = 3;
			cfg.pButtons = buttons;
			cfg.nDefaultButton = 1;
			cfg.dwCommonButtons = TDCBF_CANCEL_BUTTON;
			cfg.hwndParent = hwnd;
			int user = 0;
			HRESULT hr = TaskDialogIndirect(&cfg, &user, nullptr, nullptr);
			if (!SUCCEEDED(hr)) user = 1;
			if (user >= 0x1001 && user <= 0x1002) {
				std::wstring url;
				if (user == 0x1001) url = g_kProductUrl;
				if (user == 0x1002) url = g_kOriginalUrl;
				OpenUrl(url, hwnd);
				return 0;
			}
			if (user == 0x1003) {
				std::string u8LicenseText;
				if (!LoadResToString(IDR_BIN_LICENSE, L"BIN", u8LicenseText, 0)) {
					MessageBoxW(hwnd, L"Cannot get content", NULL, MB_ICONHAND);
					return 0;
				}
				ReplaceAll(u8LicenseText, "\n", "\r\n");
				std::wstring LicenseText = Utf8ToWide(u8LicenseText);
				const UINT dpi = GetWindowDpiSafe(g_hwnd);
				const int width = ScaleForDpi(640, dpi);
				const int height = ScaleForDpi(480, dpi);
				RECT pr{}; GetWindowRect(hwnd, &pr);
				int x = pr.left + ((pr.right - pr.left) - width) / 2;
				int y = pr.top + ((pr.bottom - pr.top) - height) / 2;
				HWND hWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"#32770", L"License - vsbm for Windows",
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_DLGFRAME, x, y, width, height, hwnd, NULL, NULL, NULL);
				if (!hWnd) {
					MessageBoxW(hwnd, L"Cannot open the page.", NULL, MB_ICONHAND);
					return 0;
				}
				RECT rc{}; GetClientRect(hWnd, &rc);
				HWND hEdit = CreateWindowExW(0, WC_EDITW, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
					ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_MULTILINE | ES_READONLY,
					0, 0, rc.right - rc.left, rc.bottom - rc.top, hWnd, (HMENU)1, NULL, NULL);
				SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)VerySimpleLicenseViewerProc);
				SendMessageW(hEdit, WM_SETTEXT, 0, (LPARAM)LicenseText.c_str());
				SendMessageW(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
				ShowWindow(hWnd, SW_SHOWNORMAL);
				SetForegroundWindow(hWnd);
				return 0;
			}
			return 0;
		}
		break;
	}

	case WMAPP_TRAYICON:
		if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
			if (g_hiddenWhileWorking) {
				RestoreFromHideWhileWorking();
			} else if (g_hiddenToTaskbar) {
				RestoreFromHideToTaskbar();
			} else {
				ShowWindow(hwnd, SW_RESTORE);
				SetForegroundWindow(hwnd);
			}
			return 0;
		}
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu();
			return 0;
		}
		break;

	case WM_LBUTTONDOWN:
		if (g_benchmarkMode) return 0;
		g_leftDown = true;
		g_mouseMoved = false;
		g_mouseX = GET_X_LPARAM(lParam);
		g_mouseY = GET_Y_LPARAM(lParam);
		SetCapture(hwnd);
		return 0;

	case WM_LBUTTONUP:
		g_leftDown = false;
		UpdateMouseButtonsFromCapture();
		return 0;

	case WM_RBUTTONDOWN:
		if (g_benchmarkMode) return 0;
		g_rightDown = true;
		g_mouseMoved = false;
		g_mouseX = GET_X_LPARAM(lParam);
		g_mouseY = GET_Y_LPARAM(lParam);
		SetCapture(hwnd);
		return 0;

	case WM_RBUTTONUP:
		g_rightDown = false;
		UpdateMouseButtonsFromCapture();
		return 0;

	case WM_MOUSEMOVE: {
		if (g_benchmarkMode) return 0;
		const int x = GET_X_LPARAM(lParam);
		const int y = GET_Y_LPARAM(lParam);

		if (g_leftDown) {
			g_ang1 += static_cast<float>(x - g_mouseX) * 0.002f;
			g_ang2 += static_cast<float>(y - g_mouseY) * 0.002f;
			if (x != g_mouseX || y != g_mouseY) {
				g_mouseMoved = true;
				g_needsRedraw = true;
			}
		}

		if (g_rightDown) {
			const float cx = static_cast<float>(g_renderWidth);
			const float cy = static_cast<float>(g_renderHeight);
			const float l = g_len * 4.0f / std::max(1.0f, cx + cy);
			const float dx = static_cast<float>(x - g_mouseX);
			const float dy = static_cast<float>(y - g_mouseY);

			g_cenx += l * (-dx * std::sin(g_ang1) - dy * std::sin(g_ang2) * std::cos(g_ang1));
			g_ceny += l * (dy * std::cos(g_ang2));
			g_cenz += l * (dx * std::cos(g_ang1) - dy * std::sin(g_ang2) * std::sin(g_ang1));
			if (x != g_mouseX || y != g_mouseY) {
				g_mouseMoved = true;
				g_needsRedraw = true;
			}
		}

		g_mouseX = x;
		g_mouseY = y;
		return 0;
	}

	case WM_MOUSEWHEEL: {
		if (g_benchmarkMode) return 0;
		const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
		g_len *= std::exp(-0.001f * static_cast<float>(delta));
		g_len = std::max(0.01f, std::min(g_len, 1000.0f));
		g_needsRedraw = true;
		return 0;
	}

	case WM_TOUCH:
		if (g_benchmarkMode) return 0;
		HandleTouch(hwnd, lParam);
		g_needsRedraw = true;
		return 0;

	case WM_GETMINMAXINFO: {
		auto* minMax = reinterpret_cast<MINMAXINFO*>(lParam);
		minMax->ptMinTrackSize.x = 256;
		minMax->ptMinTrackSize.y = 256;
		return 0;
	}

	case WM_ERASEBKGND:
		return 1;

	case WM_KEYDOWN:
		if (g_hiddenWhileWorking) {
			return 0;
		}
		if (g_benchmarkMode && wParam != VK_SPACE) {
			return 0;
		}

		switch (wParam) {
		case VK_SPACE:
			if ((lParam & (1u << 30)) == 0) {
				SetPaused(!g_paused);
			}
			break;

		case VK_DOWN:
			if (g_alpha < 255) {
				++g_alpha;
				SetMainWindowAlpha(g_alpha);
				MarkIniDirty();
				SaveWindowSettings();
			}
			break;

		case VK_UP:
			if (g_alpha > 0) {
				--g_alpha;
				SetMainWindowAlpha(g_alpha);
				MarkIniDirty();
				SaveWindowSettings();
			}
			break;

		default:
			break;
		}
		return 0;

	case WM_EXITSIZEMOVE:
		MarkIniDirty();
		SaveWindowSettings();
		return 0;

	case WM_NCHITTEST:
		if (g_hiddenWhileWorking) {
			return HTTRANSPARENT;
		}
		break;

	case WM_MOUSEACTIVATE:
		if (g_hiddenWhileWorking) {
			return MA_NOACTIVATE;
		}
		break;

	case WM_CLOSE:
		if (g_benchmarkMode && !g_benchmarkTransitioning) {
			SetPaused(true);
			ShowStatistics(hwnd);
			SaveBenchmarkStatistics();
		}
		SaveWindowSettings();
		RemoveTrayIcon();
		SetWindowLongPtrW(hwnd, GWL_EXSTYLE, GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & (~static_cast<LONG_PTR>(WS_EX_LAYERED)));
		break;

	case WM_DESTROY:
		UnregisterTouchWindow(hwnd);
		RemoveTrayIcon();
		SaveWindowSettings();
		if (g_kernelDialog) {
			DestroyWindow(g_kernelDialog);
			g_kernelDialog = nullptr;
			g_kernelDialogEdit = nullptr;
		}
		if (g_settingsDialog) {
			DestroyWindow(g_settingsDialog);
			g_settingsDialog = nullptr;
		}
		if (g_previewWindow) {
			DestroyWindow(g_previewWindow);
			g_previewWindow = nullptr;
		}
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace


static bool LoadApplicationIcons(HINSTANCE instance)
{
	const int largeWidth = GetSystemMetrics(SM_CXICON);
	const int largeHeight = GetSystemMetrics(SM_CYICON);
	const int smallWidth = GetSystemMetrics(SM_CXSMICON);
	const int smallHeight = GetSystemMetrics(SM_CYSMICON);

	g_hIcon = static_cast<HICON>(LoadImageW(
		instance,
		MAKEINTRESOURCEW(IDI_ICON1),
		IMAGE_ICON,
		largeWidth,
		largeHeight,
		LR_DEFAULTCOLOR));

	g_hIconSmall = static_cast<HICON>(LoadImageW(
		instance,
		MAKEINTRESOURCEW(IDI_ICON1),
		IMAGE_ICON,
		smallWidth,
		smallHeight,
		LR_DEFAULTCOLOR));

	if (!g_hIcon && g_hIconSmall) {
		g_hIcon = g_hIconSmall;
	}
	if (!g_hIconSmall && g_hIcon) {
		g_hIconSmall = g_hIcon;
	}

	if (!g_hIconSmall && g_hIcon) {
		g_hIconSmall = g_hIcon;
	}

	return g_hIcon != nullptr;
}

static void DestroyApplicationIcons()
{
	if (g_hIconSmall && g_hIconSmall != g_hIcon) {
		DestroyIcon(g_hIconSmall);
	}

	if (g_hIcon) {
		DestroyIcon(g_hIcon);
	}

	g_hIconSmall = nullptr;
	g_hIcon = nullptr;
}

DECLSPEC_NOINLINE static void DecryptGlobalStrings() {
	for (size_t i = 0, l = std::size(g_kWindowClass) - 1; i < l; ++i) {
		g_kWindowClass[i] -= 3;
	}
	for (size_t i = 0, l = std::size(g_kProductUrl) - 1; i < l; ++i) {
		g_kProductUrl[i] -= 6;
	}
	for (size_t i = 0, l = std::size(g_kOriginalUrl) - 1; i < l; ++i) {
		g_kOriginalUrl[i] -= 8;
	}
#if 0
	for (size_t i = 0, l = std::size(g_kOnlinePreviewUrl) - 1; i < l; ++i) {
		g_kOnlinePreviewUrl[i] -= -4;
	}
#endif
}

#include "./sig.txt"

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
) {
#ifndef DEV
	if (int r = SafeCheck(RootCrtDer, sizeof(RootCrtDer), true)) {
		return r;
	}
#endif

#ifndef MyGDIPlusNoGDIPlus
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
		g_gdiplusToken = 0;
	}
#endif

	(void)SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	g_benchmarkMode = lpCmdLine && std::wstring(lpCmdLine) == L"--benchmark";
	LoadWindowSettings();

	g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

	LoadApplicationIcons(hInstance);
	DecryptGlobalStrings();

	if (g_benchmarkMode) {
		g_frameRateLimit = 0;
		g_vsyncEnabled = false;
		g_resolutionWidth = kBenchmarkResolutionWidth;
		g_resolutionHeight = kBenchmarkResolutionHeight;
		ResetCamera();
	}

	if (!g_benchmarkMode && g_allowOnlyOneInstance) if (HWND h = FindWindowW(g_kWindowClass, NULL)) {
		int user = IDYES;
		if (g_askUserWhenConflict) 
			TaskDialog(NULL, hInstance, L"vsbm for Windows", L"It seems that you've running another instance of "
			L"the application.", L"Do you want to switch to the running instance (recommended), "
			L"or open a new instance (not recommended)?", TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON,
			TD_INFORMATION_ICON, &user);
		if (user == IDCANCEL) return ERROR_CANCELLED;
		else if (user == IDYES) {
			AllowSetForegroundWindow(ASFW_ANY);
			if (SendMessageTimeoutW(h, WMAPP_TRAYICON, 0, WM_LBUTTONUP, 
				SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 5000, 0))
				return ERROR_SUCCESS;
			TaskDialog(NULL, hInstance, L"vsbm for Windows", L"Cannot activate the running instance!",
				L"Do you want to open a new instance or give up?",
				TDCBF_YES_BUTTON | TDCBF_CANCEL_BUTTON, TD_WARNING_ICON, &user);
			if (user == IDCANCEL) return 0xcfffffff;
		}
	}

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wc.hIcon = wc.hIconSm = g_hIcon;
	wc.lpszClassName = g_kWindowClass;

	if (!RegisterClassExW(&wc)) {
		return 1;
	}

	int windowX = CW_USEDEFAULT;
	int windowY = CW_USEDEFAULT;
	int windowWidth = 0;
	int windowHeight = 0;

	const bool fixedResolution = g_resolutionWidth > 0 && g_resolutionHeight > 0;
	DWORD windowStyle = WS_OVERLAPPEDWINDOW;
	if (fixedResolution) {
		windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}

	if (g_windowSettings.hasPositionAndSize) {
		RECT saved{
			g_windowSettings.left,
			g_windowSettings.top,
			g_windowSettings.left + g_windowSettings.width,
			g_windowSettings.top + g_windowSettings.height
		};
		ClampSavedWindowRectToMonitor(saved);
		windowX = saved.left;
		windowY = saved.top;
		if (!fixedResolution) {
			windowWidth = saved.right - saved.left;
			windowHeight = saved.bottom - saved.top;
		}
	}

	if (fixedResolution) {
		RECT clientRect{0, 0, g_resolutionWidth, g_resolutionHeight};
		AdjustWindowRectEx(&clientRect, windowStyle, FALSE, WS_EX_LAYERED);
		windowWidth = clientRect.right - clientRect.left;
		windowHeight = clientRect.bottom - clientRect.top;
	} else if (!g_windowSettings.hasPositionAndSize) {
		// The default resolution mode is unrestricted, but the initial window
		// should still have a 1024x768 client area.
		RECT clientRect{0, 0, kDefaultClientWidth, kDefaultClientHeight};
		AdjustWindowRectEx(&clientRect, windowStyle, FALSE, WS_EX_LAYERED);
		windowWidth = clientRect.right - clientRect.left;
		windowHeight = clientRect.bottom - clientRect.top;
	}

	g_hwnd = CreateWindowExW(
		WS_EX_LAYERED,
		g_kWindowClass,
		kWindowTitle,
		windowStyle,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!g_hwnd) {
		return 2;
	}

	AddKernelMenuItem(g_hwnd);

	if (!g_windowSettings.hasPositionAndSize) {
		centerWindow(g_hwnd, 0);
	}

	SetMainWindowAlpha(g_alpha);
	InitializeFpsCounter();
	ApplyPausedTitle();

	ShowWindow(g_hwnd, nShowCmd);
	UpdateWindow(g_hwnd);

	if (!InitD3D()) {
		ShutdownD3D();
		DestroyWindow(g_hwnd);
		return 3;
	}

	Render();
	AddTrayIcon();

	const bool openSettingsOnStartup = !g_benchmarkMode &&
		((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
	if (!g_benchmarkMode && g_firstRunPending) {
		ShowFirstRunWelcome(g_hwnd);
		g_firstRunPending = false;
		MarkIniDirty();
		SaveWindowSettings();
	}
	if (openSettingsOnStartup) {
		OpenSettingsDialog();
	}

	timeBeginPeriod(1);

	MSG message{};
	for (;;) {
		if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				break;
			}
			if (g_settingsDialog && IsDialogMessageW(g_settingsDialog, &message)) {
				continue;
			}
			TranslateMessage(&message);
			DispatchMessageW(&message);
			continue;
		}
		PumpRenderFrame();
	}

	timeEndPeriod(1);

	ShutdownD3D();
	UnregisterClassW(g_kWindowClass, hInstance);
	DestroyApplicationIcons();
	if (g_gdiplusToken) {
#ifndef MyGDIPlusNoGDIPlus
		Gdiplus::GdiplusShutdown(g_gdiplusToken);
#endif
	}
	return static_cast<int>(message.wParam);
}
