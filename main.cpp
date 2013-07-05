/*
 * main.cpp
 *
 *  Created on: 26.06.2013
 *      Author: andrej
 */
#define PORT_DATA_BAG 31001
#define IP_BAG "192.168.203.30"
#define FREQUENCY 187500 //скорость дискретизации
#include "Drum.h"
#include <iostream>
#include <stdlib.h>

int main(int argc, char** argv) {
	int idMad = 1;
	if (argc > 1)
		idMad = atoi(argv[1]);
	unsigned p_sec = 0, p_usec = 1000000 / FREQUENCY;
	sockaddr_in addrBag;
	addrBag.sin_family = AF_INET;
	addrBag.sin_port = htons(PORT_DATA_BAG);
	char ipBag[15] = IP_BAG;
	if (inet_pton(AF_INET, ipBag,
			reinterpret_cast<void*>(&addrBag.sin_addr.s_addr)) != 1) {
		std::cerr << "name of address bag not create\n";
		exit(1);
	}
	return 0;
}

