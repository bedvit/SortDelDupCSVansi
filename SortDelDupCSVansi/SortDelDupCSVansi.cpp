// SortDelDupCSVansi.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.


#include "pch.h"
#include <stdio.h>
#include <wchar.h>
#include <vector>
#include <string>
#include <iterator>
#include <array>
#include <iostream>    
#include <fstream>      
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <ppl.h>


int   wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{   ////////////////////переводим в Юникод
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
	////////////////////переводим в Юникод
	system("mode con cols=100 lines=50"); //размер окна, вывод количества строк в консоль
	HANDLE  hout = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD  size{ 100,100 };//символов строки, строк
	SetConsoleScreenBufferSize(hout, size);//размер буфера
	///////////////////////////////////
	std::wstring prog_name = argv[0];
	int Log = 0;
	int Caption = 0;
	int OnlySort = 0;
	std::wcout << L"Created by 2017 Bedvit (bedvit@mail.ru)\n";
	std::wcout << L"Delete duplicate rows in the .CSV file (With or without a title(choose zize), ANSI, text separator - (\\n)(&#10)(U+000A)\n";

	std::wstring FileIn;
	std::wstring FileOut;

	if (argc > 2) {
		FileIn = argv[1];
		std::wcout << L"\nEnter Full Name CSV In: ";
		std::wcout << FileIn;
		FileOut = argv[2];
		std::wcout << L"\nEnter Full Name CSV Out: ";
		std::wcout << FileOut;
		if (argc > 3) Caption = _wtoi(argv[3]);
		std::wcout << L"\nEnter the number of header lines: ";
		std::wcout << Caption;
		if (argc > 4) Log = _wtoi(argv[4]);
		std::wcout << L"\nEnter 0 - Sort and Delete duplicate rows, 1 - Sort: ";
		if (argc > 5) OnlySort = _wtoi(argv[5]);
		std::wcout << OnlySort;
	}
	else {
		std::wcout << L"\nEnter Full Name CSV In: ";
		std::wcin >> FileIn;

		wprintf(L"Enter Full Name CSV Out: ");
		std::wcin >> FileOut;

		std::wcout << L"Enter the number of header lines: ";
		std::wcin >> Caption;

		Log = 1;

		std::wcout << L"nEnter 0 - Sort and Delete duplicate rows, 1 - Sort: ";
		std::wcin >> OnlySort;
	}

	if (Caption < 0) {
		bool err = true;
		wprintf(L"\nERROR: the number of header lines\n\n");
		_wsystem(L"pause");
		return 1;
	}

	if (FileIn.length() < 5 || FileOut.length() < 5) {
		bool err = true;
		wprintf(L"\nERROR: Full Name CSV\n\n");
		_wsystem(L"pause");
		return 1;
	}

	std::ifstream file1(FileIn, std::ios::in | std::ios::binary);
	if (!file1.is_open()) {
		wprintf(L"\nERROR: Full Name CSV In\n\n");
		_wsystem(L"pause");
		return 1;
	}
	std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
	if (!file2.is_open()) {
		wprintf(L"\nERROR: Full Name CSV Out\n\n");
		_wsystem(L"pause");
		return 1;
	}

	unsigned long long time0 = clock(); // начальное время
	wprintf(L"\nStart...\n");

	std::vector<std::string> DataIn;
	std::istream_iterator<std::string> input_iterator(file1), end;
	std::copy(input_iterator, end, std::back_inserter(DataIn));
	file1.close();

	unsigned long long time1 = clock();
	wprintf(L"File: Open, rows = %zi, time = %6.3f\n", DataIn.size(), (double)(time1 - time0) / 1000);

	concurrency::parallel_buffered_sort(DataIn.begin() + Caption, DataIn.end());
	unsigned long long time2 = clock();
	wprintf(L"File: parallel_buffered_sort, time = %6.3f\n", (double)(time2 - time1) / 1000);

	unsigned long long time4 = time2;
	if (OnlySort != 1)
	{
		std::vector<std::string>::iterator it;
		it = std::unique(DataIn.begin() + Caption, DataIn.end());
		unsigned long long time3 = clock();
		wprintf(L"File: Unique, time = %6.3f\n", (double)(time3 - time2) / 1000);

		DataIn.resize(std::distance(DataIn.begin(), it));
		time4 = clock();
		wprintf(L"File: Resize, time = %6.3f\n", (double)(time4 - time3) / 1000);
	}
	std::ostream_iterator<std::string> output_iterator(file2, "\r\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(DataIn.begin(), DataIn.end(), output_iterator);//std::copy(example.begin(), example.end(), output_iterator);
	file2.close();

	unsigned long long time5 = clock();
	wprintf(L"File: Save, rows = %zi, time = %6.3f\n", DataIn.size(), (double)(time5 - time4) / 1000);
	wprintf(L"End\nTotal Time = %6.3f(sec)\n", (double)(time5 - time0) / 1000);

	if (Log != 0) _wsystem(L"pause");
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
