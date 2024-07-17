#include <map>
#include <algorithm>
#include "tiny11maker_lib.h"
#include <virtdisk.h>
#include <winioctl.h>

#if defined(TEST)
#define LOG_TEST(X) std::wcout << L"DEBUG: " << X << L"\n";
#else
#define LOG_TEST(X)
#endif

typedef wchar_t wchar;

constexpr GUID VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT = { 0xEC984AEC, 0xA0F9, 0x47E9, { 0x90, 0x1F, 0x71, 0x41, 0x5A, 0x66, 0x34, 0x5B } };

const VIRTUAL_STORAGE_TYPE virtual_storage_type_iso = {
	.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO,
	.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT
};

// map contains entries like:
// physical path - drive letter
static std::map<std::wstring, std::wstring> mountedLetters;

static inline std::wstring convertToDevicePath(std::wstring physicalPath) {
	// erese unnecessary symbols
	physicalPath.erase(std::remove(physicalPath.begin(), physicalPath.end(), L'\\'), physicalPath.end());
	physicalPath.erase(std::remove(physicalPath.begin(), physicalPath.end(), L'.'), physicalPath.end());
	std::wstring devicePath = L"\\Device\\";
	devicePath += physicalPath;

	return devicePath;
}

static inline std::wstring getPhysicalPath(HANDLE isoHandle) {
	DWORD status = 0;
	
	std::wstring physicalPath;
	ULONG size = 0;
	status = GetVirtualDiskPhysicalPath(isoHandle, &size, nullptr); // get its size first

	physicalPath.resize(size);
	status = GetVirtualDiskPhysicalPath(isoHandle, &size, physicalPath.data());
	if (status != ERROR_SUCCESS) {
		std::wcout << L"Failed to get physical path with code " << status << "\n";
		return L"";
	}

	return physicalPath;
}

static bool assignDriveLetterForPhysicalPath(std::wstring path, const std::wstring& letter) {
	std::wstring devicePath = convertToDevicePath(path);
	devicePath.shrink_to_fit();
	LOG_TEST(L"Mapping " << devicePath << L" to " << letter << "\\\n");
	if (DefineDosDevice(DDD_RAW_TARGET_PATH, letter.c_str(), devicePath.c_str()) == 0) {
		LOG_TEST(L"Failed to map " << devicePath << " to " << letter << "\\\n");
		return false;
	}

	mountedLetters[devicePath] = letter;

	return true;
}

static inline std::wstring toLowercase(const std::wstring& input) {
	std::wstring result = input;
	std::transform(result.begin(), result.end(), result.begin(), ::towlower);
	return result;
}

HANDLE mountIso(const std::wstring& path, const std::wstring& driveLetter) {
	DWORD status = 0;
	HANDLE handle = nullptr;

	// tbh im too lazy to make concatenating function
	LOG_TEST(L"Opening iso located in:");
	LOG_TEST(path);

	if (!path.ends_with(L".iso")) {
		std::wcerr << path << "is not an iso file.\n";
		return nullptr;
	}

	VIRTUAL_STORAGE_TYPE type = virtual_storage_type_iso;
	VIRTUAL_DISK_ACCESS_MASK accessMask = VIRTUAL_DISK_ACCESS_ATTACH_RO | VIRTUAL_DISK_ACCESS_DETACH | VIRTUAL_DISK_ACCESS_GET_INFO;

	status = OpenVirtualDisk(&type, path.c_str(), accessMask, OPEN_VIRTUAL_DISK_FLAG_NONE, NULL, &handle);
	if (status != ERROR_SUCCESS) {
		std::wcout << L"Failed to open " << path << L" with code: " << status;
		return nullptr;
	}

	status = AttachVirtualDisk(handle, NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY | ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER, NULL, nullptr, NULL);
	if (status != ERROR_SUCCESS) {
		std::wcout << L"Failed to open " << path << L" with code: " << status;
		return nullptr;
	}

	// get cdrom number (for now might be buggy)
	bool result = assignDriveLetterForPhysicalPath(getPhysicalPath(handle), driveLetter);

	return handle;
}

bool dismountIso(HANDLE isoHandle) {
	DWORD opStatus = 0;
	std::wstring devicePath = convertToDevicePath(getPhysicalPath(isoHandle));
	devicePath.shrink_to_fit();

	opStatus = DetachVirtualDisk(isoHandle, DETACH_VIRTUAL_DISK_FLAG_NONE, 0);
	if (opStatus != ERROR_SUCCESS) {
		std::wcerr << L"Failed to detach iso. Code: " << opStatus << "L\n";
	}

	MessageBox(
		NULL, 
		L"Don't worry, the dummy CD Drive you may see in the Explorer will disappear after reboot.",
		L"Tiny11Maker Information", 
		MB_ICONINFORMATION | MB_OK
	);

	return true;
}