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

bool CompareCharPtrEqual(char* lhs, char* rhs) {
	if (lhs == NULL && rhs == NULL) return true;
	if (lhs == NULL || rhs == NULL) return false;
	return strcmp(lhs, rhs) == 0;
}
bool CompareCharPtrAscending(char* lhs, char* rhs) {
	if (lhs == NULL) return false;
	if (rhs == NULL) return true;
	return strcmp(lhs, rhs) < 0;
}
bool CompareCharPtrDescending(char* lhs, char* rhs) {
	if (lhs == NULL) return false;
	if (rhs == NULL) return true;
}

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
	int HeaderRowsCount = 0;
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
		if (argc > 3) HeaderRowsCount = _wtoi(argv[3]);
		std::wcout << L"\nEnter the number of header lines: ";
		std::wcout << HeaderRowsCount;
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
		std::wcin >> HeaderRowsCount;

		Log = 1;

		std::wcout << L"nEnter 0 - Sort and Delete duplicate rows, 1 - Sort: ";
		std::wcin >> OnlySort;
	}

	if (HeaderRowsCount < 0) {
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

	unsigned long long time0 = clock(); // начальное время
	wprintf(L"\nStart...\n");
	   

	// создаем события с автоматическим сбросом
	HANDLE hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
	if (hEndRead == NULL) { return -1; }

	//std::string fileIn(FileIn.begin(), FileIn.end());
	// открываем файл для чтения
	HANDLE hFile = CreateFile(	// дескриптор файла
		FileIn.c_str(),   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED,// | (fileFlagNoBuffering != 0 ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		return -1;
	}

	DWORD sizeF = GetFileSize(hFile, NULL);
	const DWORD  nNumberOfBytesToRead = 16777216;//33554432; //16777216;//8388608;//читаем в буфер байты
	char* notAlignBuf = new char[sizeF + 4097 + 1 + nNumberOfBytesToRead]; //4096 - выравнивание, 1 - '\0', nNumberOfBytesToRead - для последнего буфера (остаток может быть меньше, но буфер мы должны выдилить полный)
	char* buf = notAlignBuf; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	DWORD dwBytesReadWork = 0;
	DWORD  dwError;
	bool errHandleEOF = false; //метка конца файла
	char* find;// указатель для поиска
	char* nextBuf = buf;
	std::vector<char*> charPtrArr;
	charPtrArr.push_back(buf);

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// читаем данные из файла

	for (;;)
	{
		DWORD  dwBytesRead;
		//find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			nextBuf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
				//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
			default: {
				goto return1;
			}// другие ошибки
			}
		}

		if (errHandleEOF) { goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
			default: {
				goto return1;
			}// другие ошибки
			}// конец процедуры switch (dwError = GetLastError())
		}

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		nextBuf += dwBytesRead; //добавляем смещение к указателю на буфер
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	return -1;

return0:
	//unsigned long long time1 = clock();
	//wprintf(L"File: Open, rows = %zi, time = %6.3f\n", charPtrArr.size(), (double)(time1 - time0) / 1000);
	nextBuf[0] = '\0'; //конец буфера
	find = buf;
	find = strchr(find, '\n');
	while (find != NULL)
	{
		find[0] = '\0';
		charPtrArr.push_back(++find);
		find = strchr(find, '\n');
	}

	if (strlen(charPtrArr[charPtrArr.size() - 1]) == 0) {	charPtrArr.pop_back();	}

	unsigned long long time1 = clock();
	wprintf(L"File: Open, rows = %zi, time = %6.3f\n", charPtrArr.size(), (double)(time1 - time0) / 1000);
	concurrency::parallel_buffered_sort(charPtrArr.begin()+ HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscending);
	unsigned long long time2 = clock();
	wprintf(L"File: parallel_buffered_sort, time = %6.3f\n", (double)(time2 - time1) / 1000);


	unsigned long long t33 = clock();
	if (OnlySort == 0)
	{
		std::vector<char*>::iterator it;
		it = std::unique(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrEqual);
		charPtrArr.resize(std::distance(charPtrArr.begin(), it));
		t33 = clock();
		printf("dell: Time - %f\n", (t33 - time2 + .0) / CLOCKS_PER_SEC); // время отработки
	}
	CloseHandle(hFile);

	//вар1
	std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
	if (!file2.is_open()) {
		CloseHandle(hEndRead);
		delete[] notAlignBuf;
		return -1;
	}
	std::ostream_iterator<char*> output_iterator(file2, "\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(charPtrArr.begin(), charPtrArr.end(), output_iterator);
	file2.close();
	//

	CloseHandle(hEndRead);
	delete[] notAlignBuf;

	unsigned long long time5 = clock();
	wprintf(L"File: Save, rows = %zi, time = %6.3f\n", charPtrArr.size(), (double)(time5 - t33) / 1000);
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

//int   wmain(int argc, wchar_t *argv[], wchar_t *envp[])
//{   ////////////////////переводим в Юникод
//	_setmode(_fileno(stdout), _O_U16TEXT);
//	_setmode(_fileno(stdin), _O_U16TEXT);
//	_setmode(_fileno(stderr), _O_U16TEXT);
//	////////////////////переводим в Юникод
//	system("mode con cols=100 lines=50"); //размер окна, вывод количества строк в консоль
//	HANDLE  hout = GetStdHandle(STD_OUTPUT_HANDLE);
//	COORD  size{ 100,100 };//символов строки, строк
//	SetConsoleScreenBufferSize(hout, size);//размер буфера
//	///////////////////////////////////
//	std::wstring prog_name = argv[0];
//	int Log = 0;
//	int Caption = 0;
//	int OnlySort = 0;
//	std::wcout << L"Created by 2017 Bedvit (bedvit@mail.ru)\n";
//	std::wcout << L"Delete duplicate rows in the .CSV file (With or without a title(choose zize), ANSI, text separator - (\\n)(&#10)(U+000A)\n";
//
//	std::wstring FileIn;
//	std::wstring FileOut;
//
//	if (argc > 2) {
//		FileIn = argv[1];
//		std::wcout << L"\nEnter Full Name CSV In: ";
//		std::wcout << FileIn;
//		FileOut = argv[2];
//		std::wcout << L"\nEnter Full Name CSV Out: ";
//		std::wcout << FileOut;
//		if (argc > 3) Caption = _wtoi(argv[3]);
//		std::wcout << L"\nEnter the number of header lines: ";
//		std::wcout << Caption;
//		if (argc > 4) Log = _wtoi(argv[4]);
//		std::wcout << L"\nEnter 0 - Sort and Delete duplicate rows, 1 - Sort: ";
//		if (argc > 5) OnlySort = _wtoi(argv[5]);
//		std::wcout << OnlySort;
//	}
//	else {
//		std::wcout << L"\nEnter Full Name CSV In: ";
//		std::wcin >> FileIn;
//
//		wprintf(L"Enter Full Name CSV Out: ");
//		std::wcin >> FileOut;
//
//		std::wcout << L"Enter the number of header lines: ";
//		std::wcin >> Caption;
//
//		Log = 1;
//
//		std::wcout << L"nEnter 0 - Sort and Delete duplicate rows, 1 - Sort: ";
//		std::wcin >> OnlySort;
//	}
//
//	if (Caption < 0) {
//		bool err = true;
//		wprintf(L"\nERROR: the number of header lines\n\n");
//		_wsystem(L"pause");
//		return 1;
//	}
//
//	if (FileIn.length() < 5 || FileOut.length() < 5) {
//		bool err = true;
//		wprintf(L"\nERROR: Full Name CSV\n\n");
//		_wsystem(L"pause");
//		return 1;
//	}
//
//	std::ifstream file1(FileIn, std::ios::in | std::ios::binary);
//	if (!file1.is_open()) {
//		wprintf(L"\nERROR: Full Name CSV In\n\n");
//		_wsystem(L"pause");
//		return 1;
//	}
//	std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
//	if (!file2.is_open()) {
//		wprintf(L"\nERROR: Full Name CSV Out\n\n");
//		_wsystem(L"pause");
//		return 1;
//	}
//
//	unsigned long long time0 = clock(); // начальное время
//	wprintf(L"\nStart...\n");
//
//	std::vector<std::string> DataIn;
//	std::string line;
//	while (std::getline(file1, line, '\n'))
//	{
//		DataIn.push_back(line);
//	}
//	if (line.length() != 0 && line[line.length() - 1] != '\r')
//	{
//		DataIn[DataIn.size() - 1] = line + '\r';
//	}
//
//	//std::istream_iterator<std::string> input_iterator(file1), end;
//	//std::copy(input_iterator, end, std::back_inserter(DataIn));
//
//	file1.close();
//
//	unsigned long long time1 = clock();
//	wprintf(L"File: Open, rows = %zi, time = %6.3f\n", DataIn.size(), (double)(time1 - time0) / 1000);
//
//	concurrency::parallel_buffered_sort(DataIn.begin() + Caption, DataIn.end());
//	unsigned long long time2 = clock();
//	wprintf(L"File: parallel_buffered_sort, time = %6.3f\n", (double)(time2 - time1) / 1000);
//
//	unsigned long long time4 = time2;
//	if (OnlySort != 1)
//	{
//		std::vector<std::string>::iterator it;
//		it = std::unique(DataIn.begin() + Caption, DataIn.end());
//		unsigned long long time3 = clock();
//		wprintf(L"File: Unique, time = %6.3f\n", (double)(time3 - time2) / 1000);
//
//		DataIn.resize(std::distance(DataIn.begin(), it));
//		time4 = clock();
//		wprintf(L"File: Resize, time = %6.3f\n", (double)(time4 - time3) / 1000);
//	}
//	std::ostream_iterator<std::string> output_iterator(file2, "\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
//	std::copy(DataIn.begin(), DataIn.end(), output_iterator);//std::copy(example.begin(), example.end(), output_iterator);
//	file2.close();
//
//	unsigned long long time5 = clock();
//	wprintf(L"File: Save, rows = %zi, time = %6.3f\n", DataIn.size(), (double)(time5 - time4) / 1000);
//	wprintf(L"End\nTotal Time = %6.3f(sec)\n", (double)(time5 - time0) / 1000);
//
//	if (Log != 0) _wsystem(L"pause");
//	return 0;
//}