/*
 * EncoderDummy.h
 *
 *  Created on: 17.12.2020
 *      Author: Leon
 */

#ifndef ENCODERDUMMY_H_
#define ENCODERDUMMY_H_

#include "cppmain.h"
#include <Encoder.h>
#include "constants.h"

class EncoderDummy: public Encoder, CommandHandler {
public:

	static ClassIdentifier info;
	const ClassIdentifier getInfo();

	EncoderDummy();
	virtual ~EncoderDummy();

	EncoderType getType();

	int32_t getPos();
	void setPos(int32_t pos);

	ParseStatus command(ParsedCommand* cmd,std::string* reply);

private:
	int32_t pos = 0;
};

#endif /* ENCODERDUMMY_H_ */
