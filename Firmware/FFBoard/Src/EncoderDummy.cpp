/*
 * EncoderDummy.cpp
 *
 *  Created on: 17.12.2020
 *      Author: Leon
 */

#include "EncoderDummy.h"

ClassIdentifier EncoderDummy::info = {
		 .name = "Dummy" ,
		 .id=3,
		 .hidden = true,
 };
const ClassIdentifier EncoderDummy::getInfo(){
	return info;
}

EncoderDummy::EncoderDummy() {
	setPos(0);
}


EncoderDummy::~EncoderDummy() {
	setPos(0);
}

EncoderType EncoderDummy::getType(){
	return EncoderType::absolute;
}


int32_t EncoderDummy::getPos(){
	return pos;
}
void EncoderDummy::setPos(int32_t pos){
	this->pos = pos;
}

ParseStatus EncoderDummy::command(ParsedCommand* cmd,std::string* reply){
	if(!this->commandsEnabled){
		return ParseStatus::NOT_FOUND;
	}
	if(cmd->cmd != "encoderdummy")
		return ParseStatus::NOT_FOUND;

	switch(cmd->type){
	case CMDtype::get:
	case CMDtype::getat:
		*reply+=std::to_string(this->getPos());
		return ParseStatus::OK;
		break;

	case CMDtype::set:
	case CMDtype::setat:
		setPos(cmd->val);
		return ParseStatus::OK;
		break;
	default:
		return ParseStatus::ERR;
		break;
	}
}
