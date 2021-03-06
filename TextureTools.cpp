// TextureTools.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include "dxtex.h"
#include "find_file/find_file.h"

#define TEXTURE_SIZE_MAX	512
static CDxtex tex;
void FindExtFile(const char* _szFile, const char* _ext)
{
	bool bGetFile = tex.OnOpenDDSFile(_szFile);
	if (!bGetFile)
	{
		std::cout << "-1" << _szFile << "open failed!";
	}
	else
	{
		if (tex.DwWidth()> TEXTURE_SIZE_MAX || tex.DwHeight()> TEXTURE_SIZE_MAX)
		{
			DWORD w = tex.DwWidth();
			DWORD h = tex.DwHeight();
			DWORD width = w> TEXTURE_SIZE_MAX ? TEXTURE_SIZE_MAX : w;
			DWORD height = h > TEXTURE_SIZE_MAX ? TEXTURE_SIZE_MAX : h;
			HRESULT hr = tex.Resize(width, height);
			if (SUCCEEDED(hr))
			{
				std::cout << "1 " << _szFile << " " << w << " " << h << " " << tex.DwWidth() << " " << tex.DwHeight() << "\n";
				tex.OnSaveDDSFile(_szFile);
			}
			else
			{
				std::cout << "2 " << _szFile << " " << w << " " << h << " " << tex.DwWidth() << " " << tex.DwHeight() << "\n";
			}
		}
	}
}
int main(int argc, const char* argv[])
{
	class MyFindFiles : public FindFiles
	{
		bool OnFindFile(const char* _szFile, const char* _ext)
		{
			FindExtFile(_szFile, _ext);
			return true;
		}
	};
	if (argc < 5)
	{
		std::cout << "TextureTools.exe [path] [ext] [outfile] [errfile]\n";
	}
	string path(argv[1]);
	string ext(argv[2]);
	string output_file(argv[3]);
	string err_file(argv[4]);

	std::cout << "开始...\n";
	{
		GX_TIMER("TextureTools");
		MyFindFiles mFinder;
		FILE *stream = freopen(err_file.c_str(), "w", stderr);
		ofstream fout(output_file);
		streambuf *oldbuf = cout.rdbuf(fout.rdbuf());//重定向到指定文件
		mFinder.Find(path.c_str(), ext.c_str());
		cout.rdbuf(oldbuf);//重新定向到标准输出
		fclose(stream);
		fout.close();
	}


	std::cout << "按回车键退出\n";
	std::cin.get();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
