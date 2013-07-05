/*
 * Drum.h
 *
 *  Created on: 26.06.2013
 *      Author: andrej
 */

#ifndef DRUM_H_
#define DRUM_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>
#include <list>

namespace mad_n {

//СТРУКТУРА БЛОКА ДАННЫХ, КОТОРАЯ ПЕРЕДАЁТСЯ В БЭГ
#define SIGNAL_SAMPL 3	 //код, идентифицирующий блок данных сигнала
#define SIZE_SAMPL 1000  //количество отсчётов на канал в пакете
#define MODE 0 //текущий режим работы МАД (CONTINIOUS)
#define LIMIT_COUNT_SAMPL 2000000000 //масимальный номер первого отсчёта пакета. После его достижения счёт начинается с нуля
/*
 *
 */

class Timeval: public timeval {
public:
	Timeval(unsigned sec = 0, unsigned usec = 0) {
		tv_sec = sec;
		tv_usec = usec;
	}
	bool operator ==(Timeval t) {
		return (tv_sec == t.tv_sec && tv_usec == t.tv_usec);
	}
	bool operator !=(Timeval t) {
		return (tv_sec != t.tv_sec || tv_usec != t.tv_usec);
	}
	bool operator >(Timeval t) {
		if (*this == t)
			return false;
		if (tv_sec != t.tv_sec)
			return (tv_sec > t.tv_sec);
		else
			return (tv_usec > t.tv_usec);
	}
	bool operator <(Timeval t) {
		return (t > *this);
	}
	bool operator >=(Timeval t) {
		if (*this == t)
			return true;
		else
			return (*this > t);
	}
	bool operator <=(Timeval t) {
		return (t >= *this);
	}
	operator bool() {
		return (tv_usec != 0 || tv_sec != 0);
	}
	Timeval operator +(Timeval t) {
		Timeval temp;
		__suseconds_t sum_usec = t.tv_usec + tv_usec;
		temp.tv_sec = tv_sec + t.tv_sec + sum_usec / 1000000;
		temp.tv_usec = sum_usec % 1000000;
		return temp;
	}
	Timeval operator +=(Timeval t) {
		*this = *this + t;
		return *this;
	}
	long long operator /(Timeval t) {
		long long temp;
		temp = (tv_sec * 1000000 + tv_usec) / (t.tv_sec * 1000000 + t.tv_usec);
		return temp;
	}
	long long operator %(Timeval t) {
		long long temp;
		temp = (tv_sec * 1000000 + tv_usec) % (t.tv_sec * 1000000 + t.tv_usec);
		return temp;
	}
	Timeval operator *(int num) {
		Timeval temp;
		long long mul_u = tv_usec * num;
		temp.tv_usec = mul_u % 1000000;
		temp.tv_sec = tv_sec * num + mul_u / 1000000;
		return temp;
	}
};
class Drum {
	struct dataUnit {
		int ident; //идентификатор блока данных
		int mode; //режим сбора данных
		unsigned int numFirstCount; //номер первого отсчёта
		unsigned int amountCount; //количество отсчётов (1 отс = 4 x 4 байт)
		int id_MAD; //идентификатор МАДа
		int sampl[SIZE_SAMPL * 4]; //массив отсчётов
	};
	std::deque<dataUnit> fifo_;	//очередь пакетов
	dataUnit packNeutrino_; //пакет содержащий нейтрино
	sockaddr_in bagAddr_;	//адрес БЭГ
	int idMad_; //идентификатор МАД
	int count_; //номер текущего первого отсчёта пакета
	enum {
		SILENCE,	//нет золотых пакетов
		BEATS,	//золотые пакеты передаются в определённые моменты времени
		DRUMROLL	//золотые пакеты передаются с определённой частотой
	} mode_;
	bool isRun_; //передача данных запущена
	Timeval timeTrans_; //время передачи следующего пакета
	int sock_; //сокет передачи
	Timeval period_; //период передачи пакетов
	std::list<Timeval> jobs_; //сюда добавляются моменты времени,когда необходимо передать золотой пакет
public:
	void passGoldPackage(const int** buf);
	Drum(int** buf, unsigned num, const sockaddr_in& bagAddr, int idMad,
			unsigned p_sec = 0, unsigned p_usec = 5); /*передаётся массив буферов, содержащих отсчёты;
			 также передаётся адрес БЭГ и идентификатор МАД, период следования пакетов*/
	void main(Timeval t); //основной цикл программы, в который передаётся текущее время
	void putTimeStamp(Timeval& t);
	virtual ~Drum();
};

} /* namespace mad_n */
#endif /* DRUM_H_ */
