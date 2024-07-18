#include "tiny11maker_lib.h"
#include <iostream>

std::wstring path;

int main(void) {
	std::wcout << L"Paste path to your image here: ";
	std::wcin >> path;
	std::wcout << L"\n";
	InstallerImage image(path);
	image.mount();
	system("pause");
	return 0;
}