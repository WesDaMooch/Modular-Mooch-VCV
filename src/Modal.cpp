//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <vector>

#include "Modal\resonator.hpp"

// Ideas
// Multiple layers of modes


static constexpr int NUM_MODES = 6;

struct Modal : Module
{
	enum ParamId
	{
		FREQ_PARAM,
		HARMO_PARAM,
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
	std::vector<IIRResonator> modes;

	Modal() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		
		configParam(FREQ_PARAM, 20.f, 1000.f, 440.f, "Freq", "Hz");
		configParam(HARMO_PARAM, 0.001f, 4.f, 2.f, "Harmo");

		configInput(AUDIO_INPUT, "Input");

		configOutput(AUDIO_OUTPUT, "Output");

		onSampleRateChange();

		for (int i = 0; i < NUM_MODES; i++)
		{
			modes.emplace_back();
		}
	}


	void onSampleRateChange() override 
	{
		srate = APP->engine->getSampleRate();
	}

	void onReset(const ResetEvent& e) override 
	{

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

		float freq = params[FREQ_PARAM].getValue();
		float harmonicRatio = params[HARMO_PARAM].getValue();

		float input = inputs[AUDIO_INPUT].getVoltage();
		// Convert to digital audio range (-1 tp +1)
		input *= 0.1;
		input = rack::clamp(input, -1.f, 1.f);

		float output = 0.0;
		for (int i = 0; i < NUM_MODES; i++)
		{
			modes[i].set(srate, freq + (freq * harmonicRatio * i), 2.f, 1.f); // slow
			output += modes[i].proccess(input);
		}

		output = output / (float)NUM_MODES;

		// Convert to voltage range (-10 to +10)
		output *= 10.f;

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
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 20.f)), module, Modal::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 40.f)), module, Modal::HARMO_PARAM));
		// Inputs
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(10.f, 22.14f)), module, Modal::AUDIO_INPUT));
		// Ouputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.f, 99.852f)), module, Modal::AUDIO_OUTPUT));
	}
	
	void appendContextMenu(Menu* menu) override
	{
		//Modal* module = dynamic_cast<Modal*>(this->module);

		menu->addChild(new MenuSeparator);
	}

};

Model* modelModal = createModel<Modal, ModalModuleWidget>("Modal");