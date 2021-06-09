/** @file TFTPclient.h
 *  @author Lukonin A. S.
 *  @version 1.0 
 *  @date 08.06.2021 
 *  @brief Заголовочный файл для модуля TFTPclient 
*/

#pragma once
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <cmath>
#include <chrono>

#define ASCII "netascii"
#define INTERFACE

/*
 * @brief TFTP client
 * @details В конструкторе передаётся адрес TFTP сервера в формате 127.0.0.1:69
 */
class TFTPclient
{
private:
	enum { RRQ = 1, WRQ, DATA, ACK, ERROR };
	struct timeval tm;
	int remoteAdress;
	int remotePort = 69;
	int soket;

	friend struct create_req;
	friend struct create_data;
	friend struct create_resp;
public:
	TFTPclient()=delete;
	TFTPclient(std::string address, double rcv_timeout = 2.0);
	~TFTPclient();
	void Upload(std::string const & file_name, std::ifstream & file);
	void Download(std::string const & name, std::ostream & file);
};

/*
 * @brief Класс обработки ошибок client_error
 */

class client_error: public std::invalid_argument
{
public:
	explicit client_error (const std::string& what_arg):
		std::invalid_argument(what_arg) {}
	explicit client_error (const char* what_arg):
		std::invalid_argument(what_arg) {}
};
