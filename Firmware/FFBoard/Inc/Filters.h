/*
 * Filters.h
 *
 *  Created on: Feb 13, 2020
 *      Author: Yannick
 */

#ifndef FILTERS_H_
#define FILTERS_H_
#include "cppmain.h"

#ifdef __cplusplus

enum class BiquadType : uint8_t {
    lowpass = 0,
    highpass,
    bandpass,
    notch,
    peak,
    lowshelf,
    highshelf
};

class Filter{
public:
	virtual float process(float in) = 0;
};

class Biquad : public Filter{
public:
	Biquad();
    Biquad(BiquadType type, float Fc, float Q, float peakGainDB);
    ~Biquad();
    float process(float in) override;
    void setBiquad(BiquadType type, float Fc, float Q, float peakGain);
    void setFc(float Fc); //frequency

protected:
    void calcBiquad(void);

    BiquadType type;
    float a0, a1, a2, b1, b2;
    float Fc, Q, peakGain;
    float z1, z2;
};

class ExponentialFilter : public Filter{
public:
	ExponentialFilter(float gain) : gain{gain} {}
	ExponentialFilter() : gain{0.01f} {}

	float process(float in) override {
		if (first) {
			sum = in;
			first = false;
		} else {
			sum += (in - sum) * gain;
		}

		return sum;
	}

	void setGain(float newGain) {
		first = true;
		gain = newGain;
	}

	float getOutput() {
		return sum;
	}

private:
	float sum{0};
	bool first{true};
	float gain;
};


#endif

#endif /* FILTERS_H_ */
