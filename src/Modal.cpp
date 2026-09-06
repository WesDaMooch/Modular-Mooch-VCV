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
		PARAMS_LEN
	};
	enum InputId
	{
		AUDIO_INPUT,
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
	int srate = 48000;
	SvfCoefficients coefs;
	std::array<SvfCoefficients, MAX_MODES> coefsTemp;
	std::vector<ChamberlinSVF> resonators;
	
	StructureParams sParams;

	String string;
	Drum drum;

	Modal() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Parameters
		configParam(PITCH_PARAM, 20.0f, 1000.0f, 220.0f, "Pitch", "Hz");
		configParam(MORPH_PARAM, 0.0f, 1.0f, 0.5f, "Morph");
		configParam(POSITION_PARAM, 0.0f, 1.0f, 0.5f, "Position");
		configParam(DECAY_PARAM, 0.0f, 1.0f, 0.5f, "Decay");
		configParam(TIMBRE_PARAM, 0.0f, 1.0f, 0.5f, "Timbre");
		// Inputs
		configInput(AUDIO_INPUT, "Input");
		// Outputs
		configOutput(AUDIO_OUTPUT, "Output");

		for (int i = 0; i < MAX_MODES; i++)
		{
			resonators.emplace_back();
		}

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
		for (size_t i = 0; i < MAX_MODES; i++)
		{
			resonators[i].reset();
		}
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
		//float decayParam = params[DECAY_PARAM].getValue();

		float input = inputs[AUDIO_INPUT].getVoltage();
		input *= 0.1f;	// Convert to digital audio range (-1 tp +1)
		input = mClamp(input, -1.0f, 1.0f);

		sParams.pitch = params[PITCH_PARAM].getValue();
		sParams.morph = params[MORPH_PARAM].getValue();
		sParams.position = params[POSITION_PARAM].getValue();
		sParams.decay = params[DECAY_PARAM].getValue();
		sParams.timbre = params[TIMBRE_PARAM].getValue();
		string.setParams(sParams);

		float output = 0.0f;

		for (int i = 0; i < MAX_MODES; i++)
		{
			coefs = {};
			coefs = string.getCoefficients(i);
			coefsTemp[i] = coefs;

			resonators[i].setCoefficients(coefs);
			resonators[i].process(input);

			output += resonators[i].bandpass();
		}

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
		//setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Wolfram.svg")));

		// Srews
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// Parameters
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 20.f)), module, Modal::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 40.f)), module, Modal::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 60.f)), module, Modal::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 80.f)), module, Modal::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 100.f)), module, Modal::TIMBRE_PARAM));
		// Inputs
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(10.f, 22.14f)), module, Modal::AUDIO_INPUT));
		// Ouputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.f, 99.852f)), module, Modal::AUDIO_OUTPUT));
	}
	
	void appendContextMenu(Menu* menu) override
	{
		//Modal* module = dynamic_cast<Modal*>(this->module);
		//menu->addChild(new MenuSeparator);
	}

};


Model* modelModal = createModel<Modal, ModalModuleWidget>("Modal");