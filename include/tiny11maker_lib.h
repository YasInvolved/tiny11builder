#include <Windows.h>
#include <optional>
#include <string>
#include <iostream>

#if defined(TINY11MAKER_LIB_EXPORTS)
#define TINY11MAKER_LIBAPI __declspec(dllexport)
#else
#define TINY11MAKER_LIBAPI __declspec(dllimport)
#endif

struct HandleDeleter {
	void operator()(HANDLE handle) const {
		if (handle && handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
		}
	}
};

using UniqueHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleDeleter>;

// installer image class
class TINY11MAKER_LIBAPI InstallerImage {
public:
	InstallerImage(const std::wstring& path);
	~InstallerImage();
	void mount();
	void unmount();
	static inline std::wstring convertToDevicePath(std::wstring& physicalPath);
private:
	UniqueHandle handle;
	bool mounted = false;
	std::wstring path;
	std::wstring physicalPath;
	std::wstring devicePath;
	std::optional<wchar_t> driveLetter;

	inline bool searchForDriveLetter();
};