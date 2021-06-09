/**
 * @file main.cpp
 */


#include "TFTPclient.h"

using namespace std;
using namespace std;
int main()
{

	try {
#ifdef INTERFACE
		bool repeat = true;
		string option;

		// Значения по умолчанию
		string addr = "127.0.0.1:9069";
		string uploadPath = "file.txt";
		string downloadPath = "file_return.txt";
		TFTPclient t(addr);

		do {
			cout << "Введите адрес сервера и порт в формате 127.0.0.1:9069" << endl;
			cout << "Значения по умолчанию: 127.0.0.1:9069.\n0 - адрес по умолчанию\nE для выхода" << endl;
			cin >> option;

			if (option == "E") 		break;
			else if	(option != "0") TFTPclient t(option);

			cout << "Загрузить файл введите 1, скачать файл 2" << endl;
			cout << "Чтобы выйти введите любой символ" << endl;
			cin >> option;
			// Загрузка файла на сервер
			if(option == "1") {
				cout << "Выполнить загрузку: 1" << endl;
				cin >> option;
					// Загрузка в ручную
				if(option == "1") {
					cout << "Укажите путь к загружаемому файлу" << endl;
					cin >> option;
					ifstream to_server(option);

					cout << "Укажите имя под которым файл будет записан на сервер" << endl;
					cin >> option;
					t.Upload(option, to_server);
				}

				cout << "Вернуться к выбору операции 1, выйти 0" << endl;
				cin >> repeat;

				// Скачивание файла с сервера
			} else if(option == "2") {
				cout << "Выполнить скачивание: 1" << endl;
				cin >> option;

				 if(option == "1") {
					cout << "Укажите путь к загружаемому файлу" << endl;
					cin >> option;
					ofstream from_server(option);

					cout << "Укажите имя скачиваемого файла" << endl;
					cin >> option;
					t.Download(option, from_server);
				}

				cout << "Вернуться к выбору операции 1, выйти 0" << endl;
				cin >> repeat;

			} else break;

		} while(repeat);
#endif

#ifndef INTERFACE

		ifstream to_server("file.txt");
		ofstream from_server("file_return.txt");

		TFTPclient t("127.0.0.1:9069");
		t.Upload("file.txt", to_server);
		t.Download("file.txt", from_server);

#endif

	} catch (const client_error & e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
