/*
 * ButtonSourceSPI.cpp
 *
 *  Created on: 11.02.2020
 *      Author: Yannick
 */

#include <math.h>
#include <tuple>

#include "constants.h"
#include <SPIButtons.h>
#include "eeprom_addresses.h"

static ButtonSourceConfig decodeIntToConf(uint16_t config, uint16_t config_int_2);
static std::tuple<uint16_t, uint16_t> encodeConfToInt(ButtonSourceConfig* c);

ClassIdentifier SPI_Buttons_1::info = {
		 .name = "SPI 1" ,
		 .id=1
 };
const ClassIdentifier SPI_Buttons_1::getInfo(){
	return info;
}

ClassIdentifier SPI_Buttons_2::info = {
		 .name = "SPI 2" ,
		 .id=2
 };
const ClassIdentifier SPI_Buttons_2::getInfo(){
	return info;
}

SPI_Buttons::SPI_Buttons(uint16_t configuration_address, uint16_t configuration_address_2)
	: configuration_address{configuration_address}, configuration_address_2{configuration_address_2}, spi_config{*getExternalSPI_CSPins()[0]} {

	this->spi_config.peripheral.BaudRatePrescaler = SPIBUTTONS_SPEED;
	this->spi_config.peripheral.FirstBit = SPI_FIRSTBIT_LSB;
	this->spi_config.peripheral.CLKPhase = SPI_PHASE_1EDGE;
	this->spi_config.peripheral.CLKPolarity = SPI_POLARITY_LOW;

	restoreFlash();
	initSPI();

	this->setCommandsEnabled(true);
}

const SPIConfig& SPI_Buttons::getConfig() const {
	return spi_config;
}

void SPI_Buttons::beginRequest(SPIPort::Pipe& pipe) {
	pipe.beginRx(spi_buf, MIN(bytes, 4));
}

void SPI_Buttons::initSPI(){
	attachToPort(external_spi);
}

SPI_Buttons::~SPI_Buttons() {

}

/*
 * Called on preset change
 */
void SPI_Buttons::setMode(SPI_BtnMode mode){
	this->conf.mode = mode;
	setConfig(conf);
}

void SPI_Buttons::setConfig(ButtonSourceConfig config){
	config.numButtons = MIN(this->maxButtons, config.numButtons);
	this->conf = config;
	
	auto cs_pins{getExternalSPI_CSPins()};
	if (config.cs_num < cs_pins.size()) {
		this->spi_config.cs = *cs_pins[config.cs_num - 1];
	}

	// Setup presets
	if(conf.mode == SPI_BtnMode::TM){
		this->conf.cspol = false;
		this->conf.cutRight = true;
		this->spi_config.peripheral.CLKPolarity = SPI_POLARITY_LOW;

	}else if(conf.mode == SPI_BtnMode::PISOSR){
		this->conf.cspol = true;
		this->conf.cutRight = false;
		this->spi_config.peripheral.CLKPhase = SPI_PHASE_1EDGE;
		this->spi_config.peripheral.CLKPolarity = SPI_POLARITY_LOW;
	}

	mask = pow(2,config.numButtons)-1;
	offset = 8 - (config.numButtons % 8);

	// Thrustmaster uses extra bits for IDs
	if(config.mode == SPI_BtnMode::TM){
		bytes = 1+((config.numButtons+2)/8);
	}else{
		bytes = 1+((config.numButtons-1)/8);
	}

	this->btnnum = config.numButtons;
}

ButtonSourceConfig* SPI_Buttons::getConfig(){
	return &this->conf;
}

void SPI_Buttons::saveFlash(){
	auto [configuration_int, cs_num_int] = encodeConfToInt(this->getConfig());

	Flash_Write(configuration_address, configuration_int);
	Flash_Write(configuration_address_2, cs_num_int);
}

void SPI_Buttons::restoreFlash(){
	uint16_t conf_int = Flash_Read(configuration_address, 0);
	uint16_t cs_num_int = Flash_Read(configuration_address_2, 1);

	setConfig(decodeIntToConf(conf_int, cs_num_int));
}

void SPI_Buttons::process(uint32_t* buf){
	if(offset){
		if(this->conf.cutRight){
			*buf = *buf >> offset;
		}else{
			*buf = *buf & this->mask;
		}
	}
	if(conf.invert)
		*buf = (~*buf);
	*buf = *buf  & mask;
}

__attribute__((optimize("-Ofast")))
void SPI_Buttons::readButtons(uint32_t* buf){
	memcpy(buf,this->spi_buf,MIN(this->bytes,4));
	process(buf); // give back last buffer

	if(this->requestPending())
		return;	// Don't wait.

	// Get next buffer
	requestPort();
}

void SPI_Buttons::printModes(std::string* reply){
	for(uint8_t i = 0; i<mode_names.size();i++){
		*reply+=  mode_names[i]  + ":" + std::to_string(i)+"\n";
	}
}

ParseStatus SPI_Buttons::command(ParsedCommand* cmd,std::string* reply){
	ParseStatus result = ParseStatus::OK;
	auto prefix{"spi" + std::to_string(static_cast<CommandHandler&>(*this).getInfo().id) + "_"};

	if(cmd->cmd == prefix + "btnnum"){
		if(cmd->type == CMDtype::set){
			ButtonSourceConfig* c = this->getConfig();
			c->numButtons = cmd->val;
			this->setConfig(*c);

		}else if(cmd->type == CMDtype::get){
			*reply+=std::to_string(this->getBtnNum());
		}else{
			*reply+="Err. Supply number of buttons";
		}
	}else if(cmd->cmd == prefix + "btnpol"){
		if(cmd->type == CMDtype::set){
			ButtonSourceConfig* c = this->getConfig();
			c->invert = cmd->val == 0 ? false : true;
			this->setConfig(*c);

		}else if(cmd->type == CMDtype::get){
			ButtonSourceConfig* c = this->getConfig();
			*reply+=std::to_string(c->invert);
		}else{
			*reply+="Err. invert: 1 else 0";
			result = ParseStatus::ERR;
		}
	}else if(cmd->cmd == prefix + "btncut"){
		if(cmd->type == CMDtype::set){
			ButtonSourceConfig* c = this->getConfig();
			c->cutRight = cmd->val == 0 ? false : true;
			this->setConfig(*c);

		}else if(cmd->type == CMDtype::get){
			ButtonSourceConfig* c = this->getConfig();
			*reply+=std::to_string(c->cutRight);

		}else{
			*reply+="Err. cut bytes right: 1 else 0";
		}
	}else if(cmd->cmd == prefix + "btn_mode"){
		if(cmd->type == CMDtype::set){
			setMode((SPI_BtnMode)cmd->val);
		}else if(cmd->type == CMDtype::get){
			*reply += std::to_string((uint8_t)this->conf.mode);
		}else{
			printModes(reply);
		}
	} else if (cmd->cmd == prefix + "btn_cs"){
		if (handleGetSet(cmd, reply, this->conf.cs_num)) {
			setConfig(this->conf);
		}
	} else if(cmd->cmd == "help"){
		result = ParseStatus::OK_CONTINUE;
		*reply += "SPI Button: spi#_btn_mode,spi#_btncut,spi#_btnpol,spi#_btnnum\n";
	}else{
		result = ParseStatus::NOT_FOUND;
	}
	return result;
}

static ButtonSourceConfig decodeIntToConf(uint16_t config_int, uint16_t config_int_2){
	ButtonSourceConfig c;
	c.numButtons = config_int & 0x3F;
	c.invert = (config_int >> 6) & 0x1;
	c.cutRight = (config_int >> 7) & 0x1;
	c.mode = SPI_BtnMode(config_int >> 8);
	c.cs_num = static_cast<uint8_t>(config_int_2 & 0xF); // Leaving space for other use.
	return c;
}
static std::tuple<uint16_t, uint16_t> encodeConfToInt(ButtonSourceConfig* c){
	uint16_t val = c->numButtons & 0x3F;
	val |= c->invert << 6;
	val |= c->cutRight << 7;
	val |= (uint8_t)c->mode << 8;
	return { val, c->cs_num & 0xF };
}

