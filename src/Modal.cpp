//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <array>
#include <vector>
#include "Modal\common.hpp"
#include "Modal\chamberlinSVF.hpp"
#include "Modal\structures.hpp"

// Ideas
// Multiple layers of modes

// Params
// 
// Pitch
// 
// Material
// Position

struct Modal : Module
{
	enum ParamId
	{
		PITCH_PARAM,
		MORPH_PARAM,
		POSITION_PARAM,
		DECAY_PARAM,
		TIMBRE_PARAM,
		DELAY_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		EXCITER_INPUT,
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

	// DSP
	static constexpr int MAX_DELAY_SAMPLES = 48000;

	int srate = 48000;
	SvfCoefficients coefs;
	std::array<SvfCoefficients, MAX_MODES> coefsTemp = {};
	std::array<ChamberlinSVF, MAX_MODES> resonators = {};

	//std::array<std::array<float, MAX_DELAY_SAMPLES>, MAX_MODES> delayBuffers = {};
	//std::array<int, MAX_MODES> writeIdxs = {};

	std::array<float, MAX_DELAY_SAMPLES> delayBuffer = {};
	//std::array<int, MAX_MODES> writeIdxs = {};

	int writeIdx;

	StructureParams sParams;

	String string;
	Drum drum;

	Modal() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Parameters
		configParam(PITCH_PARAM, -54.0f, 54.0f, 0.0f, "Pitch", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(MORPH_PARAM, 0.0f, 1.0f, 0.5f, "Morph");
		configParam(POSITION_PARAM, 0.0f, 1.0f, 0.5f, "Position");
		configParam(DECAY_PARAM, 0.0f, 1.0f, 0.5f, "Decay");
		configParam(TIMBRE_PARAM, 0.0f, 1.0f, 0.5f, "Timbre");
		configParam(DELAY_PARAM, 0.0f, 1.0f, 0.0f, "Delay", " s");
		// Inputs
		configInput(EXCITER_INPUT, "Exciter");
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
		drum.setSamplerate(srate);

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
		// Audio input
		float exciterIn = inputs[EXCITER_INPUT].getVoltage();
		exciterIn *= 0.1f;	// Convert to digital audio range (-1 tp +1)
		exciterIn = mClamp(exciterIn, -1.0f, 1.0f);

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
		string.setParams(sParams);
		
		// Exciter delay
		float delayParam = params[DELAY_PARAM].getValue();
		// Have a max delay time and the scale into it
		
		// Write into the buffer
		delayBuffer[writeIdx] = exciterIn;

		float delayAmount = MAX_DELAY_SAMPLES / MAX_MODES;

		float output = 0.0f;
		for (int i = 0; i < MAX_MODES; i++)
		{
			// Delay
			int delaySamples = i * delayAmount * delayParam;
			int readIdx = writeIdx - delaySamples;

			if (readIdx < 0)
				readIdx += MAX_DELAY_SAMPLES;

			float exciterOut = delayBuffer[readIdx];

			coefs = {};
			coefs = string.getCoefficients(i);
			coefsTemp[i] = coefs;

			resonators[i].setCoefficients(coefs);
			resonators[i].process(exciterOut);

			output += resonators[i].bandpass();
		}

		// Advance write index
		writeIdx++;
		if (writeIdx >= MAX_DELAY_SAMPLES)
			writeIdx = 0;

		output = output * string.getActiveModesScaler();
		output *= 10.f;	// Convert to voltage range (-10 to +10)
		outputs[AUDIO_OUTPUT].setVoltage(output);
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
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 20.0f)), module, Modal::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 40.0f)), module, Modal::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 60.0f)), module, Modal::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 80.0f)), module, Modal::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 100.0f)), module, Modal::TIMBRE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.0f, 20.0f)), module, Modal::DELAY_PARAM));

		// Inputs
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 10.0f)), module, Modal::EXCITER_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 20.0f)), module, Modal::PITCH_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 40.0f)), module, Modal::MORPH_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 60.0f)), module, Modal::POSITION_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 80.0f)), module, Modal::DECAY_INPUT));
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(8.0f, 100.0f)), module, Modal::TIMBRE_INPUT));
		// Ouputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.0f, 99.852f)), module, Modal::AUDIO_OUTPUT));
	}
	
	void appendContextMenu(Menu* menu) override
	{
		//Modal* module = dynamic_cast<Modal*>(this->module);
		//menu->addChild(new MenuSeparator);
	}

};


Model* modelModal = createModel<Modal, ModalModuleWidget>("Modal");