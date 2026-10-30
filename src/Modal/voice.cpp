// Voice.cpp


#include "voice.hpp"


Voice::Voice() {

}


// Pass exciter params?
void Voice::noteOn() {
	active = true;

	// Trigger excitor
	exciter.trigger();

	// Start attack env
	// Turn on env follower?
}


void Voice::noteOff() {
	active = false;

	// Reset envelope follower / turn it off?
	// Reset resonators?
}


void Voice::setParams() {

}


float Voice::process() {
	
	if (!active)
		return 0.f;

	float excitation = exciter.get(); //+ feedback; // Add feedback into exciter


	return excitation;
}