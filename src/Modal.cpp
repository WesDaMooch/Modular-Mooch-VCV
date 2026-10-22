//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <array>
#include "Modal/common.hpp"
#include "Modal/chamberlinSVF.hpp"
#include "dsp/ramp.hpp"
#include "Modal/exciter.hpp"
#include "Modal/stringModel.hpp"
#include "Modal/beamModel.hpp"
#include "Modal/rectModel.hpp"
#include "Modal/drumModel.hpp"

// Ideas

// Models
// String x
// 
// Euler-Bernoulli beam
// 
// Plate
// https://www.acs.psu.edu/drussell/Demos/rect-membrane/rect-mem.html
// 
// 
// Drum (circular - bessel func)

// I think the timbre dial (maybe rename tone) 
// should act more like a material selector
// full left only f0 -> 808/909 Kick/Tom thing -> dense stone 
// -> wood -> metal -> glass -> full right lower harmonics are muted 

// FILTERS
// The conventional resonant filter in modal synthesis is designed by taking 
// a two-pole complex resonator and digitizing it with the Impulse-Invariant Transform. 
// 
// TODO:
// Different filter types
// Digital biquad - RBJ band-pass biquad?
// T Bridge (808-909)
// Wasp filter 
// Moog Ladder?
// How will this change the sound

// Feedback amound though coupling filter and body resonator

// Randomness (see max msp book) on the amplitudes and q of modes

// TODO:
// Body resonitors (shell) 
// https://ccrma.stanford.edu/~jos/jnmr/Body_Resonators.html
// Guitar / violin 
// Piano soundboard
// Voice formants?

// Piano hammer model https://ccrma.stanford.edu/~jos/jnmr/Pianos.html

// Soft clip std::tanh for now
// See for tanh with ceiling limiter https://github.com/odoare/MechanOdd
// 
// Old (0s Mackie desk style clipping?
// Per harmonic soft clipping?

// Punch
// Pitch env, Amount and decay

// use normalize q input 0? meh

// Very small amount of slew on the pitch to stop filter click?
// or is this just the limiation of a biquad


struct Modal : Module
{
	enum ParamId
	{
		TRIG_PARAM,
		EXCITER_TEXTURE_PARAM,
		EXCITER_DENSITY_PARAM,
		VELOCITY_PARAM,
		PITCH_PARAM,
		MORPH_PARAM,
		POSITION_PARAM,
		STRUCTURE_PARAM,
		DECAY_PARAM,
		TIMBRE_PARAM,
		PUNCH_AMOUNT_PARAM,
		PUNCH_DECAY_PARAM,
		MUFFLE_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		TRIG_INPUT,
		EXCITER_INPUT,
		VELOCITY_INPUT,
		PITCH_INPUT,
		MORPH_INPUT,
		POSITION_INPUT,
		DECAY_INPUT,
		TIMBRE_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		TEST_LIGHT,
		LIGHTS_LEN
	};

	int srate = 48000; 

	dsp::BooleanTrigger trigBoolean;
	dsp::SchmittTrigger trigSchmitt;

	Exciter exciter;
	ExciterParams exciterParams;

	SvfCoefficients coefs;
	std::array<ChamberlinSVF, MAX_MODES> resonators = {};

	std::array<BaseModel*, NUM_STRUCTURES> structures = {};
	StringModel stringModel;
	BeamModel beamModel;
	//RectModel rectModel;
	//Drum2 drum;
	StructureParams sParams;

	bool muffle = false;
	dsp::BooleanTrigger muffleBoolean;
	Ramp muffleRamp;

	float feedback = 0.f;
	float feedbackParam = 0.f;

	Modal() : structures{ &stringModel, &beamModel }
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Parameters
		configButton(TRIG_PARAM, "Trigger");
		configParam(EXCITER_TEXTURE_PARAM, 0.f, 1.f, 0.75f, "Exciter Texture");
		configParam(EXCITER_DENSITY_PARAM, 0.f, 1.f, 0.f, "Exciter Density");
		configParam(VELOCITY_PARAM, 0.f, 1.f, 0.8f, "Velocity");
		configParam(PITCH_PARAM, -54.0f, 54.0f, 0.0f, "Pitch", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(MORPH_PARAM, 0.0f, 1.0f, 0.5f, "Morph");
		configParam(POSITION_PARAM, 0.0f, 1.0f, 0.5f, "Position");
		configParam(STRUCTURE_PARAM, 0.f, 1.f, 0.f, "Strutcure");
		configParam(DECAY_PARAM, 0.0f, 1.0f, 0.5f, "Decay");
		configParam(TIMBRE_PARAM, 0.0f, 1.0f, 0.5f, "Timbre");
		configParam(PUNCH_AMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "Punch Amount");
		configParam(PUNCH_DECAY_PARAM, 0.0f, 1.0f, 0.0f, "Punch Decay");
		configButton(MUFFLE_PARAM, "Muffle");
		// Inputs
		configInput(TRIG_INPUT, "Trigger");
		configInput(EXCITER_INPUT, "Exciter");
		configInput(VELOCITY_INPUT, "Velocity");
		configInput(PITCH_INPUT, "1V/octave pitch");
		configInput(MORPH_INPUT, "Morph CV");
		configInput(POSITION_INPUT, "Position CV");
		configInput(DECAY_INPUT, "Decay CV");
		configInput(TIMBRE_INPUT, "Timbre CV");
		// Outputs
		configOutput(AUDIO_OUTPUT, "Output");

		muffleRamp.setCurrentValue(1.f);

		onSampleRateChange();
	}

	void onSampleRateChange() override 
	{
		srate = APP->engine->getSampleRate();

		for (auto* structure : structures) {
			if (structure != nullptr)
				structure->setSamplerate(srate);
		}

		for (auto& resonator : resonators)
			resonator.setSamplerate(srate);
	}

	void onReset(const ResetEvent& e) override 
	{
		for (auto& resonator : resonators)
			resonator.reset();
	}
	
	json_t* dataToJson() override 
	{
		json_t* rootJ = json_object();
		return rootJ;
	}
	
	void dataFromJson(json_t* rootJ) override 
	{

	}
	
	void process(const ProcessArgs& args) override
	{
		// Get structure
		const float MAX_STRUCTURE = static_cast<float>(NUM_STRUCTURES - 1);
		float structParam = mClamp(params[STRUCTURE_PARAM].getValue() * MAX_STRUCTURE, 0.f, MAX_STRUCTURE);

		size_t structAIdx = static_cast<size_t>(std::floor(structParam));
		size_t structBIdx = static_cast<size_t>(std::ceil(structParam));

		BaseModel* structA = structures[structAIdx];
		BaseModel* structB = nullptr;

		if (structAIdx != structBIdx) {
			structB = structures[structBIdx];
		}

		float structCrossfade = structParam - static_cast<float>(structAIdx);

		// Trigger button
		bool trig = params[TRIG_PARAM].getValue() > 0.f;

		// Trigger input
		trigSchmitt.process(inputs[TRIG_INPUT].getVoltage(), 0.1f, 1.f);

		bool gate = trig || trigSchmitt.isHigh();


		// Set exciter params
		// Velocity
		bool velocityCvConnected = inputs[VELOCITY_INPUT].isConnected();
		float velocityCv = velocityCvConnected ? inputs[VELOCITY_INPUT].getVoltage() * 0.1f : 1.f;
		velocityCv = mClamp(velocityCv, 0.f, 1.f);
		exciterParams.velocity = velocityCv * params[VELOCITY_PARAM].getValue();
		exciterParams.texture = params[EXCITER_TEXTURE_PARAM].getValue();
		exciterParams.density = params[EXCITER_DENSITY_PARAM].getValue();
		exciter.process(args.sampleTime, exciterParams);

		if (trigBoolean.process(gate)) 
			exciter.trigger();

		float excitation = exciter.get(); //+ feedback; // Add feedback into exciter


		// Resonator
		float pitch = (params[PITCH_PARAM].getValue() / 12.f) + inputs[PITCH_INPUT].getVoltage();
		float freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
		sParams.fundamentalFreq = freq;

		float morchCv = inputs[MORPH_INPUT].getVoltage() * 0.1f;
		float morph = params[MORPH_PARAM].getValue() + morchCv;
		sParams.morph = mClamp(morph, 0.f, 1.f);

		float positionCv = inputs[POSITION_INPUT].getVoltage() * 0.1f;
		float position = params[POSITION_PARAM].getValue() + positionCv;
		sParams.position = mClamp(position, 0.f, 1.f);

		float decayCv = inputs[DECAY_INPUT].getVoltage() * 0.1f;
		float decay = params[DECAY_PARAM].getValue() + decayCv;
		sParams.decay = mClamp(decay, 0.f, 1.f);

		float timbreCv = inputs[TIMBRE_INPUT].getVoltage() * 0.1f;
		float timbre = params[TIMBRE_PARAM].getValue() + timbreCv;
		sParams.timbre = mClamp(timbre, 0.f, 1.f);

		if (structA != nullptr) {
			structA->setParams(sParams);

			if (structB != nullptr)
				structB->setParams(sParams);
		}


		// Muffle button
		if (muffleBoolean.process(params[MUFFLE_PARAM].getValue())) {
			muffle ^= true;
			muffleRamp.setTarget(muffleRamp.getValue(), muffle ? -1.f : 1.f, 1000);
		}

		float polarity = muffle ? -1.f : 1.f;

		if (!muffleRamp.isFinished())
			polarity = muffleRamp.process();


		float output = 0.0f;
		for (int i = 0; i < MAX_MODES; i++)
		{
			// Structure
			coefs = {};

			if (structA == nullptr)
				continue;

			const SvfCoefficients& structCoefsA = structA->getCoefficients(i);

			if (structB != nullptr) {
				const SvfCoefficients& structCoefsB = structB->getCoefficients(i);
				mInterpSvfCoefs(structCrossfade, coefs, structCoefsA, structCoefsB);
			}
			else {
				coefs = structCoefsA;
			}
			
			// Filter bank
			resonators[i].setCoefficients(coefs);
			resonators[i].process(excitation);

			float modeOut = resonators[i].bandpass();
			// TODO: per mode soft clipping? tanh?

			// TODO: optimize %
			// use polarity patterns not just odd even
			if (i % 2 != 0)
				modeOut *= polarity;

			output += modeOut;
		}

		output *= 0.0625; // 1/16, 1/MAX MODES
		output = std::tanh(output); // easy soft clipping

		// TODO: look up coupling filter stuff
		// Add feedback back in when body is implemented
		//feedback = output * 0.f; //params[FEEDBACK].getValue();

		output *= 10.f;	// Convert to voltage range (-10 to +10)
		outputs[AUDIO_OUTPUT].setVoltage(output);
		//outputs[AUDIO_OUTPUT].setVoltage(excitation);
	}		
};


struct ModalModuleWidget : ModuleWidget 
{
	ModalModuleWidget(Modal* module) 
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/modal.svg")));
		// Srews
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// Parameters
		addParam(createParamCentered<VCVButton>(mm2px(Vec(10.0f, 20.0f)), module, Modal::TRIG_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0f, 80.0f)), module, Modal::EXCITER_TEXTURE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0f, 100.0f)), module, Modal::EXCITER_DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.0f, 40.0f)), module, Modal::VELOCITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 20.0f)), module, Modal::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 40.0f)), module, Modal::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 60.0f)), module, Modal::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 80.0f)), module, Modal::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 100.0f)), module, Modal::TIMBRE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 80.0f)), module, Modal::STRUCTURE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 20.0f)), module, Modal::PUNCH_AMOUNT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 40.0f)), module, Modal::PUNCH_DECAY_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(30.0f, 60.0f)), module, Modal::MUFFLE_PARAM));


		// Inputs
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(10.0f, 40.0f)), module, Modal::TRIG_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(10.0f, 60.0f)), module, Modal::EXCITER_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(30.0f, 20.0f)), module, Modal::VELOCITY_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(40.0f, 20.0f)), module, Modal::PITCH_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(40.0f, 40.0f)), module, Modal::MORPH_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(40.0f, 60.0f)), module, Modal::POSITION_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(40.0f, 80.0f)), module, Modal::DECAY_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(40.0f, 100.0f)), module, Modal::TIMBRE_INPUT));
		// Ouputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(80.f, 100.f)), module, Modal::AUDIO_OUTPUT));
	}
	
	void appendContextMenu(Menu* menu) override
	{
		//Modal* module = dynamic_cast<Modal*>(this->module);
		//menu->addChild(new MenuSeparator);
	}

};


Model* modelModal = createModel<Modal, ModalModuleWidget>("Modal");