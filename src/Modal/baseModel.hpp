// baseModel.hpp

#pragma once
#include <array>
#include <cmath>
#include "common.hpp"

static constexpr int NUM_STRUCTURES = 3;

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

// TODO: rename BaseModel
class BaseModel {
public:
	virtual ~BaseModel() = default;  

	virtual void setSamplerate(int samplerate) = 0;
	virtual void setParams(const StructureParams& newParams) = 0;
	virtual void update() = 0;
	const virtual SvfCoefficients& getCoefficients(int idx) const = 0;

	// TODO: Implement this cloning thing?
	//virtual std::unique_ptr<StructureBase> clone() const = 0;

protected:
	// TODO: tune max and min q
	static constexpr float MIN_Q = 0.00001f;
	static constexpr float MAX_Q = 500.f;
};