//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <array>
#include "Modal\common.hpp"
#include "Modal\chamberlinSVF.hpp"
#include "Modal\structures.hpp"
#include "Modal\exciter.hpp"

// Ideas
// Multiple layers of modes

// Randomness (see max msp book) on the amplitudes and q of modes

// TODO:
// Exciter goes into spectal env -> an amount of pre gain to the exciter input to a mode. Not sure about this now

// String
// decay is exp (or modelled) q, decay param just controls base decay

// Body resonitors
// https://ccrma.stanford.edu/~jos/jnmr/Body_Resonators.html
// Guitar / violin 
// Piano soundboard
// Voice formants?

// Piano hammer model https://ccrma.stanford.edu/~jos/jnmr/Pianos.html

// Soft clip std::tanh for now
// Old (0s Mackie desk style clipping?
// Per harmonic soft clipping?

// Punch
// Pitch env, Amount and decay

// Optimize SVF or uses biquads
// use normalize q input 0

struct Modal : Module
{
	enum ParamId
	{
		TRIG_PARAM,
		EXCITER_SHAPE_PARAM,
		EXCITER_DENSITY_PARAM,
		VELOCITY_PARAM,
		PITCH_PARAM,
		MORPH_PARAM,
		POSITION_PARAM,
		STRUCTURE_PARAM,
		DECAY_PARAM,
		TIMBRE_PARAM,
		DELAY_TIME_PARAM,
		DELAY_TYPE_PARAM,
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

	SvfCoefficients coefs;
	std::array<ChamberlinSVF, MAX_MODES> resonators = {};

	Exciter exciter;
	ExciterParams exciterParams;

	std::array<StructureBase*, NUM_STRUCTURES> structures = {};
	String string;
	Drum2 drum;
	StructureParams sParams;

	bool soft = false;
	dsp::BooleanTrigger softBoolean;

	Modal() : structures{ &string, &drum }
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Parameters
		configButton(TRIG_PARAM, "Trigger");
		configParam(EXCITER_SHAPE_PARAM, 0.f, 1.f, 0.75f, "Exciter Shape");
		configParam(EXCITER_DENSITY_PARAM, 0.f, 1.f, 0.f, "Exciter Density");
		configParam(VELOCITY_PARAM, 0.f, 1.f, 0.8f, "Velocity");
		configParam(PITCH_PARAM, -54.0f, 54.0f, 0.0f, "Pitch", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(MORPH_PARAM, 0.0f, 1.0f, 0.5f, "Morph");
		configParam(POSITION_PARAM, 0.0f, 1.0f, 0.5f, "Position");
		configParam(STRUCTURE_PARAM, 0.f, 1.f, 0.f, "Strutcure");
		configParam(DECAY_PARAM, 0.0f, 1.0f, 0.5f, "Decay");
		configParam(TIMBRE_PARAM, 0.0f, 1.0f, 0.5f, "Timbre");
		configParam(DELAY_TIME_PARAM, 0.0f, 1.0f, 0.0f, "Delay Time");
		configParam(DELAY_TYPE_PARAM, 0.0f, 1.0f, 0.0f, "Delay Type");
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

		StructureBase* structA = structures[structAIdx];
		StructureBase* structB = nullptr;

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
		exciterParams.shape = params[EXCITER_SHAPE_PARAM].getValue();
		exciterParams.density = params[EXCITER_DENSITY_PARAM].getValue();
		exciter.process(exciterParams);

		if (trigBoolean.process(gate)) 
			exciter.trigger();

		float excitation = exciter.get();

		// Resonator
		float pitch = (params[PITCH_PARAM].getValue() / 12.f) + inputs[PITCH_INPUT].getVoltage();
		float freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
		sParams.fundamentalFreq = freq;

		float morchCv = inputs[MORPH_INPUT].getVoltage() * 0.1f;
		float morph = params[MORPH_PARAM].getValue() + morchCv;
		sParams.morph = mClamp(morph, 0.0f, 1.0f);

		float positionCv = inputs[POSITION_INPUT].getVoltage() * 0.1f;
		float position = params[POSITION_PARAM].getValue() + positionCv;
		sParams.position = mClamp(position, 0.0f, 1.0f);

		float decayCv = inputs[DECAY_INPUT].getVoltage() * 0.1f;
		float decay = params[DECAY_PARAM].getValue() + decayCv;
		sParams.decay = mClamp(decay, 0.0f, 1.0f);

		float timbreCv = inputs[TIMBRE_INPUT].getVoltage() * 0.1f;
		float timbre = params[TIMBRE_PARAM].getValue() + timbreCv;
		sParams.timbre = mClamp(timbre, 0.0f, 1.0f);

		if (structA != nullptr) {
			structA->setParams(sParams);

			if (structB != nullptr)
				structB->setParams(sParams);
		}

		// Solf button
		if (softBoolean.process(params[MUFFLE_PARAM].getValue()))
			soft ^= true;

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

			// Odd polarity flip
			if (soft) {
				// TODO: optimize %
				if (i % 2 != 0)
					modeOut *= -1;
			}

			output += modeOut;
		}

		// Output
		/*
		float activeModesScaler = 0.f;

		if (structA != nullptr) {
			activeModesScaler = structA->getActiveModesScaler();

			if (structB != nullptr)
				activeModesScaler = mInterp(structCrossfade, activeModesScaler, structB->getActiveModesScaler());
		}
		output = output * activeModesScaler;
		*/
		output *= 0.0625; // 1/16
		output = std::tanh(output); // easy soft clipping
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
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0f, 80.0f)), module, Modal::EXCITER_SHAPE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.0f, 100.0f)), module, Modal::EXCITER_DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.0f, 40.0f)), module, Modal::VELOCITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 20.0f)), module, Modal::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 40.0f)), module, Modal::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 60.0f)), module, Modal::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 80.0f)), module, Modal::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.0f, 100.0f)), module, Modal::TIMBRE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 80.0f)), module, Modal::STRUCTURE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 20.0f)), module, Modal::DELAY_TIME_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.0f, 40.0f)), module, Modal::DELAY_TYPE_PARAM));
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