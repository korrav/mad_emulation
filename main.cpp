/*
 * main.cpp
 *
 *  Created on: 26.06.2013
 *      Author: andrej
 */
#include "Drum.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <vector>
#include "dirent.h"

#define PORT_DATA_BAG 31001
#define IP_BAG "192.168.203.30" //ip БЭГ
#define FREQUENCY 187500 //скорость дискретизации
#define PREFIX "data" //префикс для файлов данных
#define SIZE_DATA_FILE (SIZE_SAMPL * 4 * sizeof(int)) //размер файла данных
//команды оператора
static const std::string SHOT = "shot"; //передача одного пакета данных
static const std::string SHOTG = "shotg"; //единичная передача золотого пакета
/*Опции командной строки: id идентификатор МАД; nf количество файлов с данными отсчётов; dir - папка с файлами;
 * gold файл с золотым пакетом
 */
void handlCom(mad_n::Drum& drum); //обработка команд оператора
int main(int argc, char** argv) {
	unsigned p_sec = 0, p_usec = 1000000 / FREQUENCY;
	int idMad = 1;
	int numF = -1;

	std::string dir = "./";
	std::string gF = "";
	//чтение командной строки
	for (int i = 1; i < argc; i += 2) {
		if (!strcmp("--id", argv[i]))
			idMad = atoi(argv[i + 1]);
		else if (!strcmp("--nf", argv[i]))
			numF = atoi(argv[i + 1]);
		else if (!strcmp("--dir", argv[i]))
			dir = argv[i + 1];
		else if (!strcmp("--gold", argv[i]))
			gF = argv[i + 1];
	}
	//инициализация адреса БЭГ
	sockaddr_in addrBag;
	addrBag.sin_family = AF_INET;
	addrBag.sin_port = htons(PORT_DATA_BAG);
	char ipBag[15] = IP_BAG;
	if (inet_pton(AF_INET, ipBag,
			reinterpret_cast<void*>(&addrBag.sin_addr.s_addr)) != 1) {
		std::cerr << "name of address bag not create\n";
		exit(1);
	}
	//заправить барабан
	DIR* sd;
	dirent *fileD;
	int countP = 0;
	std::string file;
	std::string prefix;
	struct stat statF;
	std::ifstream streamF;
	std::vector<int*> data;
	sd = opendir(dir.c_str());
	if (!sd) {
		perror("There is no way to open a directory\n");
		exit(1);
	}
	for (; (countP == -1 || countP < numF) && (fileD = readdir(sd)); countP++) {
		file = fileD->d_name;
		if (!file.compare(0, strlen(prefix.c_str()), prefix)) {
			stat(file.c_str(), &statF);
			if (statF.st_size != SIZE_DATA_FILE)
				continue;
			streamF.open(file.c_str(), std::ios::in | std::ios::binary);
			if (!streamF.is_open())
				continue;
			int* buf = new int[SIZE_SAMPL * 4];
			streamF.read((char*) buf, SIZE_DATA_FILE);
			data.push_back(buf);
			streamF.close();
		}
	}
	std::cout << "In the drum fueled " << countP << " packets\n";
	int* gbuf = NULL; //указатель на золотой пакет
	if (!gF.empty()) {
		streamF.open(gF.c_str(), std::ios::in | std::ios::binary);
		if (!streamF.is_open()) {
			gbuf = new int[SIZE_SAMPL * 4];
			streamF.read((char*) gbuf, SIZE_DATA_FILE);
			streamF.close();
		}
	}
	//создание Drum
	mad_n::Drum drum(data, addrBag, idMad, gbuf, p_sec, p_usec);
	//ГЛАВНЫЙ ЦИКЛ
	fd_set fdin;
	timeval nullpause = { 0, 0 };
	mad_n::Timeval loctime;
	for (;;) {
		FD_ZERO(&fdin);
		FD_SET(STDIN_FILENO, &fdin);
		select(STDIN_FILENO + 1, &fdin, NULL, NULL, &nullpause);
		if (FD_ISSET(STDIN_FILENO, &fdin))
			handlCom(drum);
		gettimeofday(&loctime, NULL);
		drum.main(loctime);
	}
	return 0;
}

void handlCom(mad_n::Drum& drum) {
	std::string comlin;
	std::getline(std::cin, comlin);
	if (comlin == SHOT) {
		drum.oneShot();
		std::cout << "Передан один пакет данных\n";
	} else if (comlin == SHOTG) {
		drum.oneShotGold();
		std::cout << "Передан один золотой пакет данных\n";
	} else
		std::cout << "Неизвестная команда\n";
	return;
}
