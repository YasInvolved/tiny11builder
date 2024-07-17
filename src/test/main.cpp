#include "tiny11maker_lib.h"
#include <iostream>

std::wstring path;

int main(void) {
	std::wcout << L"Paste path to your image here: ";
	std::wcin >> path;
	std::wcout << L"\n";
	HANDLE handle = mountIso(path);
	if (handle == nullptr) return -1;
	std::wcout << L"Check if iso is mounted and files are visible\n";
	system("pause");
	dismountIso(handle);
	return 0;
}