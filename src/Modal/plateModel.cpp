#include "plateModel.hpp"



Plate::Plate() {
	//buildTables();
	update();
}


void Plate::setSamplerate(int samplerate) {
	this->samplerate = std::max(samplerate, 1);
}


void Plate::setParams(const StructureParams& newParams) {
	if (newParams == prevParams)
		return;

	prevParams = newParams;
	update();
}


void Plate::update() {

}


const SvfCoefficients& Plate::getCoefficients(int idx) const {
	idx = mClamp(idx, 0, MAX_MODES - 1);
	return coefs[idx];
}


void Plate::buildTables() {
	
}