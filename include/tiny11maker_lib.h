#include <Windows.h>
#include <string>

#if defined(TEST)
#include <iostream>
#endif

#if defined(TINY11MAKER_LIB_EXPORTS)
#define TINY11MAKER_LIBAPI extern "C" __declspec(dllexport)
#else
#define TINY11MAKER_LIBAPI extern "C" __declspec(dllimport)
#endif

// mounts iso located in given path
// returns HANDLE object to that iso
// the drive is mounted under letter Z, but it can be changed via optional param
TINY11MAKER_LIBAPI HANDLE mountIso(const std::wstring& path, const std::wstring& driveLetter = L"Z:");

// dismounts iso with given handle 
// return value: true if function succeeded, otherwise false
// closes handle automatically, theres no need to call CloseHandle later
TINY11MAKER_LIBAPI bool dismountIso(HANDLE isoHandle);