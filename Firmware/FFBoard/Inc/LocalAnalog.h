/*
 * LocalAnalog.h
 *
 *  Created on: Nov 6, 2020
 *      Author: Yannick
 */

#ifndef SRC_LOCALANALOG_H_
#define SRC_LOCALANALOG_H_

#include <vector>
#include <limits.h>

#include "AnalogSource.h"
#include "PersistentStorage.h"
#include "CommandHandler.h"
#include "AdcHandler.h"
#include "Filters.h"

struct MinMaxPair{
	int32_t min = INT_MAX;
	int32_t max = INT_MIN;
};

struct LocalAnalogConfig{
	uint8_t analogmask = 0xff;
	bool autorange = false;
};

class LocalAnalog : public AnalogSource, public CommandHandler, public AdcHandler{
public:
	LocalAnalog();
	virtual ~LocalAnalog();

	const ClassIdentifier getInfo();
	static ClassIdentifier info;

	void saveFlash(); 		// Write to flash here
	void restoreFlash();	// Load from flash

	std::vector<int32_t>* getAxes();

	ParseStatus command(ParsedCommand* cmd,std::string* reply);
	void setAutorange(bool autorange);
	virtual std::string getHelpstring(){return "Analog pins: local_ain_mask,local_ain_num,local_ain_acal\n";}

	void adcUpd(volatile uint32_t* ADC_BUF, uint8_t chans, ADC_HandleTypeDef* hadc) override;

private:
	bool autorange = false;
	static LocalAnalogConfig decodeAnalogConfFromInt(uint16_t val);
	static uint16_t encodeAnalogConfToInt(LocalAnalogConfig conf);
	static constexpr uint8_t numPins{ADC_PINS};
	MinMaxPair minMaxVals[numPins];
	ExponentialFilter filters[numPins];

	LocalAnalogConfig aconf;
};



#endif /* SRC_LOCALANALOG_H_ */
