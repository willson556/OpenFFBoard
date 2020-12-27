/*
 * LocalAnalog.cpp
 *
 *  Created on: Nov 6, 2020
 *      Author: Yannick
 */

#include "LocalAnalog.h"
#include "global_callbacks.h"
#include "flash_helpers.h"

ClassIdentifier LocalAnalog::info = {
	 .name = "AIN-Pins" ,
	 .id=0
};


LocalAnalog::LocalAnalog() {
	restoreFlash();

	for (int i=0; i < numPins; ++i) {
		filters[i].setGain(0.01f);
	}
}

LocalAnalog::~LocalAnalog() {

}

const ClassIdentifier LocalAnalog::getInfo(){
	return info;
}

void LocalAnalog::saveFlash(){
	Flash_Write(ADR_LOCALANALOG_MASK, LocalAnalog::encodeAnalogConfToInt(this->aconf));
}

void LocalAnalog::restoreFlash(){
	uint16_t aconfint;
	if(Flash_Read(ADR_LOCALANALOG_MASK,&aconfint)){
		this->aconf = LocalAnalog::decodeAnalogConfFromInt(aconfint);
	}
}


void LocalAnalog::setAutorange(bool autorange){
	for(uint8_t i = 0 ; i<numPins ; i++){
		minMaxVals[i] = MinMaxPair(); // Reset minmax
	}
	this->aconf.autorange = autorange;
}

void LocalAnalog::adcUpd(volatile uint32_t* ADC_BUF, uint8_t chans, ADC_HandleTypeDef* hadc){
	if (hadc != &AIN_HADC) {
		return;
	}

	uint axes = MIN(chans-ADC_CHAN_FPIN,numPins);
	for (uint i=0; i < axes; ++i){
		if(!(aconf.analogmask & 0x01 << i))
			continue;

		int32_t val = ((ADC_BUF[i+ADC_CHAN_FPIN] & 0xFFF) << 4)-0x7fff;
		filters[i].process(static_cast<float>(val));
	}
}

std::vector<int32_t>* LocalAnalog::getAxes(){
	this->buf.clear();

	for(uint8_t i = 0;i<numPins;i++){
		if(!(aconf.analogmask & 0x01 << i))
			continue;

		int32_t val = static_cast<int32_t>(filters[i].getOutput());

		if(aconf.autorange){
			minMaxVals[i].max = std::max(minMaxVals[i].max,val);
			minMaxVals[i].min = std::min(minMaxVals[i].min,val);

			int32_t range = (minMaxVals[i].max - minMaxVals[i].min);
			if(range > 1){
				float scaler = ((float)0xffff / (float)range);
				val *= scaler;
				val = val - ((scaler*(float)minMaxVals[i].min) + 0x7fff);
				val = clip(val,-0x7fff,0x7fff); // Clip if slightly out of range because of inaccuracy
			}
		}


		this->buf.push_back(val);
	}
	return &this->buf;
}

ParseStatus LocalAnalog::command(ParsedCommand* cmd,std::string* reply){
	ParseStatus flag = ParseStatus::OK;
	if(cmd->cmd == "local_ain_mask"){
		if(cmd->type == CMDtype::get){
			*reply+=std::to_string(aconf.analogmask);
		}else if(cmd->type == CMDtype::set){
			aconf.analogmask = cmd->val;
		}else{
			flag = ParseStatus::ERR;
		}
	}else if(cmd->cmd == "local_ain_acal"){
		if(cmd->type == CMDtype::get){
			*reply+=std::to_string(aconf.autorange);
		}else if(cmd->type == CMDtype::set){
			setAutorange(cmd->val != 0);
		}else{
			flag = ParseStatus::ERR;
		}
	}else if(cmd->cmd == "local_ain_num"){
		if(cmd->type == CMDtype::get){
			*reply+=std::to_string(numPins); // Max num inputs
		}else{
			flag = ParseStatus::ERR;
		}
	}else{
		flag = ParseStatus::NOT_FOUND;
	}
	return flag;
}


LocalAnalogConfig LocalAnalog::decodeAnalogConfFromInt(uint16_t val){
	LocalAnalogConfig aconf;
	aconf.analogmask = val & 0xff;
	aconf.autorange = (val >> 8) & 0x1;
	return aconf;
}
uint16_t LocalAnalog::encodeAnalogConfToInt(LocalAnalogConfig conf){
	uint16_t val = conf.analogmask & 0xff;
	val |= (conf.autorange & 0x1) << 8;
	return val;
}
