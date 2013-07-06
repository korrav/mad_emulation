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

Drum::Drum(std::vector<int*>& dat, const sockaddr_in& bagAddr, int idMad,
		int* gbuf, unsigned p_sec, unsigned p_usec) :
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
		std::cout << "Incorrectly specified period of packets\n";
		exit(1);
	} else {
		period_.tv_sec = p_sec;
		period_.tv_usec = p_usec;
	}
	dataUnit* pdat = new dataUnit;
	for (unsigned i = 0; i < dat.size(); i++) {
		memcpy(pdat->sampl, dat[i], sizeof(pdat->sampl));
		delete[] dat[i];
		pdat->amountCount = SIZE_SAMPL;
		pdat->id_MAD = idMad_;
		pdat->ident = SIGNAL_SAMPL;
		pdat->mode = MODE;
		fifo_.push_back(*pdat);
	}
	dat.clear();
	if (gbuf)
		passGoldPackage(gbuf);
	delete pdat;
	return;
}

void Drum::passGoldPackage(const int* buf) {
	packNeutrino_ = new dataUnit;
	memcpy(packNeutrino_->sampl, buf, sizeof(packNeutrino_->sampl));
	delete[] buf;
	return;
}

void Drum::main(Timeval t) {
	//удар
	if (isRun_ && t >= timeTrans_) {
		if (!jobs_.empty() && t >= *jobs_.begin()) {
			//передача золотого пакета
			transGoldPack();
			do {
				jobs_.pop_front();
			} while (!jobs_.empty() && *jobs_.begin() < timeTrans_);
		} else
			//передача простого пакета
			transSimplePack();
		timeTrans_ += period_;

	}
	return;
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

void Drum::oneShot(void) {
	transSimplePack();
	return;
}

void Drum::transSimplePack(void) {
	if (!fifo_.empty()) {
		fifo_.front().numFirstCount = count_;
		sendto(sock_, reinterpret_cast<void*>(&fifo_.front()), sizeof(dataUnit),
				0, reinterpret_cast<sockaddr*>(&bagAddr_), sizeof(bagAddr_));
		fifo_.push_back(fifo_.front());
		fifo_.pop_front();
	}

	count_ += SIZE_SAMPL;
	if (count_ >= LIMIT_COUNT_SAMPL)
		count_ = 0;
	return;
}

void Drum::oneShotGold(void) {
	transGoldPack();
}

void Drum::transGoldPack(void) {
	if (packNeutrino_) {
		packNeutrino_->numFirstCount = count_;
		sendto(sock_, reinterpret_cast<void*>(&packNeutrino_),
				sizeof(packNeutrino_), 0,
				reinterpret_cast<sockaddr*>(&bagAddr_), sizeof(bagAddr_));
		count_ += SIZE_SAMPL;
		if (count_ >= LIMIT_COUNT_SAMPL)
			count_ = 0;
	}
	return;
}

Drum::~Drum() {
	fifo_.clear();
	delete packNeutrino_;
	return;
}

} /* namespace mad_n */
