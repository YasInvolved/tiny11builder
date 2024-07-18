#include <map>
#include <algorithm>
#include "tiny11maker_lib.h"
#include <virtdisk.h>
#include <winioctl.h>
#include <optional>

#if defined(TEST)
#define LOG_TEST(X) std::wcout << L"DEBUG: " << X << L"\n";
#else
#define LOG_TEST(X)
#endif

#define LOG_ERROR(X) std::wcerr << L"ERROR: " << X << L"\n";
#define LOG_WARNING(X) std::wcerr << L"WARN: " << X << L"\n";

constexpr GUID VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT = { 0xEC984AEC, 0xA0F9, 0x47E9, { 0x90, 0x1F, 0x71, 0x41, 0x5A, 0x66, 0x34, 0x5B } };

constexpr VIRTUAL_STORAGE_TYPE virtual_storage_type_iso = {
	.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO,
	.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT
};

constexpr VIRTUAL_DISK_ACCESS_MASK iso_access_mask = VIRTUAL_DISK_ACCESS_ATTACH_RO | VIRTUAL_DISK_ACCESS_READ | VIRTUAL_DISK_ACCESS_GET_INFO | VIRTUAL_DISK_ACCESS_DETACH;

static std::wstring toLowercase(const std::wstring& input) {
	std::wstring result = input;
	std::transform(result.begin(), result.end(), result.begin(), ::towlower);
	return result;
}

static bool compareIgnorecase(const std::wstring& s1, const std::wstring& s2) {
	return toLowercase(s1) == toLowercase(s2);
}

static inline void removeNullChar(std::wstring& str) {
	str.erase(std::remove(str.begin(), str.end(), L'\0'), str.end());
}

InstallerImage::InstallerImage(const std::wstring& path) {
	DWORD opStatus = 0;
	HANDLE rawHandle = INVALID_HANDLE_VALUE;
	this->path = path;
	opStatus = OpenVirtualDisk((PVIRTUAL_STORAGE_TYPE)&virtual_storage_type_iso, path.c_str(), iso_access_mask, OPEN_VIRTUAL_DISK_FLAG_NONE, NULL, &rawHandle);
	if (opStatus != ERROR_SUCCESS) {
		LOG_ERROR(L"Failed to open " << path << ". Code: " << opStatus);
	}
	handle.reset(rawHandle);
}

InstallerImage::~InstallerImage() {
	if (mounted) {
		unmount();
	}
	handle.release();
}

void InstallerImage::mount() {
	DWORD opStatus = 0;
	opStatus = AttachVirtualDisk(handle.get(), NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY, NULL, NULL, NULL);
	if (opStatus != ERROR_SUCCESS) {
		LOG_ERROR(L"Failed to mount iso. Code: " << opStatus);
		mounted = false;
	}
	else mounted = true;

	// get physical path
	ULONG length = 0;
	GetVirtualDiskPhysicalPath(handle.get(), &length, nullptr); // get it's length first

	// allocate enough memory to contain it
	physicalPath.resize(length);

	// get physical path
	opStatus = GetVirtualDiskPhysicalPath(handle.get(), &length, physicalPath.data());
	if (opStatus != ERROR_SUCCESS) {
		LOG_ERROR(L"Failed to get image physical path. Code: " << opStatus);
		DetachVirtualDisk(handle.get(), DETACH_VIRTUAL_DISK_FLAG_NONE, NULL);
		mounted = false;
	}
	else mounted = true;

	// convert it to device path
	devicePath = convertToDevicePath(physicalPath);
	removeNullChar(devicePath);

	if (!searchForDriveLetter()) { 
		LOG_WARNING("Failed to automatically find drive letter");
		return;
	}
	
	LOG_TEST(L"Drive letter: " << driveLetter.value());
	return;
}

void InstallerImage::unmount() {
	DWORD opStatus = 0;
	opStatus = DetachVirtualDisk(handle.get(), DETACH_VIRTUAL_DISK_FLAG_NONE, NULL);
	return;
}

std::wstring InstallerImage::convertToDevicePath(std::wstring& physicalPath) {
	// erese unnecessary symbols
	physicalPath.erase(std::remove(physicalPath.begin(), physicalPath.end(), L'\\'), physicalPath.end());
	physicalPath.erase(std::remove(physicalPath.begin(), physicalPath.end(), L'.'), physicalPath.end());
	std::wstring devicePath = L"\\Device\\";
	devicePath += physicalPath;

	return devicePath;
}

// returns true if found
inline bool InstallerImage::searchForDriveLetter() {
	wchar_t drive[] = L" :";

	for (wchar_t letter = L'A'; letter < L'Z'; letter++) {
		std::wstring devicePath;
		devicePath.resize(MAX_PATH);
		drive[0] = letter;
		if (QueryDosDevice(drive, devicePath.data(), devicePath.size())) {
			removeNullChar(devicePath);
			LOG_TEST(drive << L"\\ is mapped to " << devicePath);
			LOG_TEST(toLowercase(this->devicePath) << " = " << toLowercase(devicePath));
			if (compareIgnorecase(this->devicePath, devicePath)) {
				driveLetter = letter;
				return true;
			}
		}
	}

	return false;
}