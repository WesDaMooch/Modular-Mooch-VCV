#pragma once
#include "common.hpp"
#include <array>
#include <cmath>

static constexpr int NUM_STRUCTURES = 2;

// TODO: clamp freq range on all models??

struct StructureParams {
	float fundamentalFreq = 220.0f;
	float morph = 0.f;
	float position = 0.f;
	float decay = 0.f;
	float timbre = 0.f;

	bool operator==(const StructureParams& other) const {
		return	fundamentalFreq	== other.fundamentalFreq && 
			morph				== other.morph &&
			position			== other.position &&
			decay				== other.decay &&
			timbre				== other.timbre;
	}

	bool operator!=(const StructureParams& other) const {
		return !(*this == other);
	}
};


class StructureBase {
public:
	virtual ~StructureBase() = default;  

	virtual void setSamplerate(int newSamplerate) = 0;
	virtual void setParams(const StructureParams& newParams) = 0;
	virtual void update() = 0;
	const virtual SvfCoefficients& getCoefficients(int idx) const = 0;

	// TODO: Implement this cloning thing?
	//virtual std::unique_ptr<StructureBase> clone() const = 0;
};