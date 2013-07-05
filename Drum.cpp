/*
 * Drum.cpp
 *
 *  Created on: 26.06.2013
 *      Author: andrej
 */

#include "Drum.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

namespace mad_n {

#define PORT_DATA_MAD 31000

Drum::Drum(int** buf, unsigned num, const sockaddr_in& bagAddr, int idMad,
		unsigned p_sec, unsigned p_usec) :
		bagAddr_(bagAddr), idMad_(idMad), count_(0), mode_(SILENCE), isRun_(
				false), timeTrans_(0) {
	sockaddr_in addr;
	sock_ = socket(AF_INET, SOCK_DGRAM, 0); //сокет передачи данных
	if (sock_ == -1) {
		perror("socket not create\n");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_DATA_MAD);
	addr.sin_addr.s_addr = htonl(INADDR_ANY );
	if (bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
		perror("socket not bind\n");
		exit(1);
	}
	if (p_usec > 1000000 || p_usec < 0 || p_sec < 0) {
		std::cout << "Неверно задан период следования пакетов\n";
		exit(1);
	} else {
		period_.tv_sec = p_sec;
		period_.tv_usec = p_usec;
	}
	dataUnit* pdat = new dataUnit;
	for (unsigned i = 0; i < num; i++) {
		memcpy(pdat->sampl, buf[i], sizeof(pdat->sampl));
		pdat->amountCount = SIZE_SAMPL;
		pdat->id_MAD = idMad_;
		pdat->ident = SIGNAL_SAMPL;
		pdat->mode = MODE;
		fifo_.push_back(*pdat);
	}
	delete pdat;
	return;
}

void Drum::passGoldPackage(const int** buf) {
	memcpy(packNeutrino_.sampl, *buf, sizeof(packNeutrino_.sampl));
	return;
}

void Drum::main(Timeval t) {
	//удар
	if (isRun_ && t >= timeTrans_) {
		if (!jobs_.empty() && t >= *jobs_.begin()) {
			//передача золотого пакета
			packNeutrino_.numFirstCount = count_;
			sendto(sock_, reinterpret_cast<void*>(&packNeutrino_),
					sizeof(packNeutrino_), 0,
					reinterpret_cast<sockaddr*>(&bagAddr_), sizeof(bagAddr_));
			do {
				jobs_.pop_front();
			} while (!jobs_.empty() && *jobs_.begin() < timeTrans_);
		} else {
			//передача простого пакета
			if (!fifo_.empty()) {
				fifo_.front().numFirstCount = count_;
				sendto(sock_, reinterpret_cast<void*>(&fifo_.front()),
						sizeof(dataUnit), 0,
						reinterpret_cast<sockaddr*>(&bagAddr_),
						sizeof(bagAddr_));
				fifo_.push_back(fifo_.front());
				fifo_.pop_front();
			}
		}
		count_ += SIZE_SAMPL;
		if (count_ >= LIMIT_COUNT_SAMPL)
			count_ = 0;
		timeTrans_ += period_;

	}
}

void Drum::putTimeStamp(Timeval& t) {
	if (t < timeTrans_)
		return;
	long long n_per = (t - timeTrans_) / period_;
	t = timeTrans_ + period_ * static_cast<int>(n_per);
	jobs_.push_back(t);
	jobs_.sort();
	return;
}

Drum::~Drum() {
	fifo_.clear();
	return;
}

} /* namespace mad_n */