// Voice.hpp


#pragma once
#include "common.hpp"
#include "exciter.hpp"
#include "chamberlinSVF.hpp"
#include "waveformModel.hpp"
#include "stringModel.hpp"

constexpr static int NUM_VOICES = 1;


class Voice {
public:
	Voice();
	void noteOn();
	void noteOff();
	void setParams();
	float process();

protected:
	bool active = false;

	// Engine
	Exciter exciter;
	ExciterParams exciterParams;

	SvfCoefficients coefs;
	std::array<ChamberlinSVF, MAX_MODES> resonators = {};

	std::array<BaseModel*, NUM_STRUCTURES> structures = {};
	WaveformModel waveformModel;
	StringModel stringModel;
};


struct VoiceAllocator {
	//

	
};

