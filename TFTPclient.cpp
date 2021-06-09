/**
 * @file TFTPclient.cpp
 */

#include "TFTPclient.h"

struct create_req{
	uint16_t type = htons(TFTPclient::RRQ);
	char filename[512];
	uint16_t size = 2;
	
	create_req(std::string const & name, int type = TFTPclient::RRQ) : type(htons(type)) 
    {
        std::strcpy(filename, name.c_str());
        std::strcpy(filename + name.size() + 1, ASCII);
        size = 2 + name.size() + sizeof(ASCII) + 1;
    }	
};

struct create_data{
	uint16_t type = htons(TFTPclient::DATA);
	uint16_t block_number;
	char data[512];
	
	size_t size;
	
	bool read_block(std::ifstream &file){
		file.read(data, 512);
		size = 4 + file.gcount();
        return file.gcount() == 512;
	}
	
	uint16_t get_block(void) { return ntohs(block_number); }
	void set_block(uint16_t block_number){this->block_number = htons(block_number);}
};

struct create_resp{
	uint16_t type = htons(TFTPclient::ACK);
	union{
        uint16_t block;
        uint16_t errcode;
    };
	char err[128];
	
	void set_block(uint16_t val) { block = htons(val); }
	uint16_t get_block(void) { return ntohs(block); }
};

/**
* @brief Конструктор класса TFTPclient.
* @param address адрес TFTP сервера в формате 127.0.0.1:69.
* @param rcv_timeout время ожидания от сервера, по умлочанию 2.0 сек.
* @details Создаёт сетевой сокет и привязывает его к локальному адресу.
* @warning При передаче неверного адреса в верном формате подключение всё равно произойдёт.
* @throw client_error, если произошла ошибка.
*/

TFTPclient::TFTPclient(std::string address, double rcv_timeout){

	// Если адрес передан в формате 127.0.0.1:9069 считываем всё что после :
	int sp = address.find(":");	
	if(sp != -1) remotePort = htons(stoi(address.substr(sp + 1, address.length())));
	
	// Преобразуем адресс из вида 127.0.0.1 в 16777343
	struct in_addr addr;	
	int ad_addr = inet_aton(address.substr(0, sp).c_str(), &addr);	
	if(ad_addr != 1) throw client_error(std::string("Ошибка преобразования адреса"));
	remoteAdress = addr.s_addr;
	
	// Таймаут на прием
    tm.tv_sec = int(rcv_timeout);
    tm.tv_usec = int(std::fmod(rcv_timeout, 1.0) * 1000000);

	// Сокет
	soket = socket(AF_INET, SOCK_DGRAM, 0);
	if(soket == -1) throw client_error(std::string("Ошибка создания сокета"));
	#ifndef INTERFACE
	else std::cout << "Сокет успешно создан" << std::endl;
	#endif
	if(setsockopt(soket, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm)) == -1)
		throw client_error(std::string("Время истекло"));
	
	sockaddr_in selfAddr;
	selfAddr.sin_family = AF_INET;
	selfAddr.sin_port = 0;
	selfAddr.sin_addr.s_addr = 0;

	// Бинд
	if(bind(soket, (struct sockaddr *) &selfAddr, sizeof(selfAddr)) == -1)
		throw client_error(std::string("Ошибка бинда"));
	#ifndef INTERFACE
	else std::cout << "Сокет привязан к локальному адресу" << std::endl;
	#endif
}

/**
* @brief Деструктор класса TFTPclient.
* @details Закрывает сокет, открытый в конструкторе.
*/

TFTPclient::~TFTPclient(){
	close(soket);
}

/**
* @brief Загрузка файла на сервер.
* @param file_name имя файла на сервере в который будет записана загружаема информация. Создаётся автоматически.
* @param file файловый поток для чтения. Из него будет считана информация, загружаемая на сервер.
* @throw client_error, если произошла ошибка.
*/

void TFTPclient::Upload(std::string const & file_name, std::ifstream & file){
	
	create_req req(file_name, WRQ);
	create_resp resp;	
	
	// Подготовка структуры с адресом программы
	sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = remotePort;
	remoteAddr.sin_addr.s_addr = remoteAdress;
	
	// Отправка запроса на запись
	if (sendto(soket, &req, req.size, 0, (struct sockaddr *) &remoteAddr, sizeof(remoteAddr)) == -1)
		throw client_error(std::string("Ошибка отправки запроса на запись"));
	#ifndef INTERFACE
	else std::cout << "Запрос на запись отправлен" << std::endl;
	#endif
	
	// Получение ответа от сервера
	socklen_t rlen = sizeof(rlen);
	if (recvfrom(soket, &resp, sizeof(resp), 0, (struct sockaddr *) &remoteAddr, &rlen) == -1)
		throw client_error(std::string("Ошибка получения ответа от сервера"));
	#ifndef INTERFACE
	else std::cout << "Ответ сервера принят" << std::endl;
	#endif
	

	create_data chunk;
	chunk.set_block(1);
	bool go;
	do{
		go = chunk.read_block(file);
		sendto(soket, &chunk, chunk.size, 0, (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
		if (recvfrom(soket, &resp, sizeof(resp), 0, (struct sockaddr *) &remoteAddr, &rlen) == -1)
			throw client_error(std::string("Ошибка отправки пакета серверу"));
		#ifndef INTERFACE
		else std::cout << "Пакет отправлен серверу" << std::endl;
		#endif
		chunk.set_block(chunk.get_block() + 1);
		
	}while(go);
}

/**
* @brief Скачивание файла с сервера.
* @param file_name имя файла на сервере, который необходимо скачать.
* @param file файловый поток для записи принятой информации в файл.
* @throw client_error, если произошла ошибка.
*/

void TFTPclient::Download(std::string const & file_name, std::ostream & file){
	
	// Подготовка структуры с адресом программы
	sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = remotePort;
	remoteAddr.sin_addr.s_addr = remoteAdress;
	
    create_req req(file_name);    
    create_data dat;    
    create_resp ack;
	
	if (sendto(soket, &req, req.size, 0, (struct sockaddr *) &remoteAddr, sizeof(remoteAddr)) == -1)
        throw client_error(std::string("Ошибка отправки запроса на чтение"));
    #ifndef INTERFACE
	else std::cout << "Запрос на чтение отправлен" << std::endl;
	#endif
	
    socklen_t rlen = sizeof(rlen);
    ssize_t len;
	do{
		// Читаем блок. Из длины вычитаем длину заголовка.
		len = recvfrom(soket, &dat, sizeof(dat), 0, (struct sockaddr *) &remoteAddr, &rlen) - 4;
		if(len == -1) throw client_error(std::string("Ошибка получения пакета"));
		#ifndef INTERFACE
		else std::cout << "Пакет получен" << std::endl;
		#endif
		
		// Подтвердить прием блока
        ack.set_block(dat.get_block());
        sendto(soket, &ack, 4, 0, (struct sockaddr *) &remoteAddr, sizeof(remoteAddr));
		
        // Запись в файл
        if (len) file.write(dat.data, len);
		
	}while(len == 512);
}

