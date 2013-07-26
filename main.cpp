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
#define IP_BAG "192.168.127.30" //ip БЭГ
#define FREQUENCY 187500 //скорость дискретизации
#define PREFIX "data" //префикс для файлов данных
#define SIZE_DATA_FILE (SIZE_SAMPL * 4 * sizeof(int)) //размер файла данных
//команды оператора
static const std::string SHOT = "shot"; //передача одного пакета данных
static const std::string SHOTG = "shotg"; //единичная передача золотого пакета
static const std::string GET_NUM_PACK = "g_num_pack"; //определить сколько обычных пакетов данных находится в барабане
/*Опции командной строки: id идентификатор МАД; nf количество файлов с данными отсчётов; dir - папка с файлами;
 * gold файл с золотым пакетом; pref префикс файлов данных
 */
void handlCom(mad_n::Drum& drum); //обработка команд оператора
int main(int argc, char** argv) {
	unsigned p_sec = 0, p_usec = 1000000 / FREQUENCY;
	int idMad = 1;
	int numF = -1;
	std::string prefix = ""; //префикс файлов данных

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
		else if (!strcmp("--pref", argv[i]))
			prefix = argv[i + 1];
		else
			std::cout << argv[i] << " - неизвестный параметр\n";
	}
	//инициализация адреса БЭГ
	sockaddr_in addrBag;
	addrBag.sin_family = AF_INET;
	addrBag.sin_port = htons(PORT_DATA_BAG);
	char ipBag[16] = IP_BAG;
	if (inet_pton(AF_INET, ipBag,
			reinterpret_cast<void*>(&addrBag.sin_addr.s_addr)) != 1) {
		std::cerr << "name of address bag not create\n";
		exit(1);
	}
	//заправить барабан
	DIR* sd;
	dirent *fileD;
	int countP = 0;
	std::string file; //имя файла данных
	struct stat statF;
	std::ifstream streamF;
	std::vector<int*> data;
	sd = opendir(dir.c_str());
	if (!sd) {
		perror("There is no way to open a directory\n");
		exit(1);
	}
	for (; (countP == -1 || countP < numF) && (fileD = readdir(sd));) {
		file = fileD->d_name;
		if (!file.compare(0, strlen(prefix.c_str()), prefix)) {
			if (stat((file = dir + "/" + file).c_str(), &statF) == -1) {
				std::cout << "Для файла " << file << std::endl;
				perror("stat");
				exit(1);
			}
			if (statF.st_size != SIZE_DATA_FILE) {
				std::cout << "Размер файла " << file << " не равен "
						<< SIZE_DATA_FILE << " байт, а равен " << statF.st_size
						<< " байт\n";
				continue;
			}
			streamF.open(file.c_str(), std::ios::in | std::ios::binary);
			if (!streamF.is_open()) {
				std::cout << "Невозможно открыть файл " << file << std::endl;
				continue;
			}
			int* buf = new int[SIZE_SAMPL * 4];
			streamF.read((char*) buf, SIZE_DATA_FILE);
			data.push_back(buf);
			countP++;
			streamF.close();
		}
	}
	std::cout << "In the drum fueled " << countP << " packets\n";
	int* gbuf = NULL; //указатель на золотой пакет
	if (!gF.empty()) {
		streamF.open(gF.c_str(), std::ios::in | std::ios::binary);
		if (streamF.is_open()) {
			gbuf = new int[SIZE_SAMPL * 4];
			streamF.read((char*) gbuf, SIZE_DATA_FILE);
			streamF.close();
			std::cout << "Получен золотой пакет из файла " << gF << std::endl;
		} else
			std::cout << "Невозможно открыть файл " << gF
					<< " , содержащий золотой пакет\n";
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
	if (comlin == SHOT)
		drum.oneShot();
	else if (comlin == SHOTG)
		drum.oneShotGold();
	else if (comlin == GET_NUM_PACK) {
		std::cout << "Всего в барабан заправлено " << drum.getPackageInDrum()
				<< " пакетов данных\n";
	} else
		std::cout << "Неизвестная команда\n";
	return;
}
