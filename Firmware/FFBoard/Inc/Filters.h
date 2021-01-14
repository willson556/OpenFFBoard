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
	virtual ~Filter() {}

	virtual float process(float in) = 0;
	virtual void setFc(float Fc) = 0;
};

class Biquad : public Filter{
public:
	Biquad();
    Biquad(BiquadType type, float Fc, float Q, float peakGainDB);
    virtual ~Biquad() {}
    float process(float in) override;
    void setBiquad(BiquadType type, float Fc, float Q, float peakGain);
    void setFc(float Fc) override; //frequency

protected:
    void calcBiquad(void);

    BiquadType type;
    float a0, a1, a2, b1, b2;
    float Fc, Q, peakGain;
    float z1, z2;
};

class ExponentialFilter : public Filter{
public:
	ExponentialFilter(float fc) : fc{fc} {}
	ExponentialFilter() : fc{0.01f} {}
	virtual ~ExponentialFilter() {}

	float process(float in) override {
		if (first) {
			sum = in;
			first = false;
		} else {
			sum += (in - sum) * fc;
		}

		return sum;
	}

	void setFc(float fc) override {
		first = true;
		this->fc = fc;
	}

	float getOutput() {
		return sum;
	}

private:
	float sum{0};
	bool first{true};
	float fc;
};


#endif

#endif /* FILTERS_H_ */
