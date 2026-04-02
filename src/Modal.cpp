//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <array>

static constexpr int NUM_MODES = 2;

struct Filter
{

};

struct Modal : Module
{
	enum ParamId
	{
		TEST_PARAM,
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
	
	dsp::RCFilter mode;
	//std::array<Filter, NUM_MODES> mode = {};

	Modal() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(AUDIO_INPUT, "Input");
		configOutput(AUDIO_OUTPUT, "Output");

		//mode.setCutoffFreq(10);

		onSampleRateChange();
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
		float input = inputs[AUDIO_INPUT].getVoltage();

		mode.setCutoffFreq(0.001);

		mode.process(input);
		float output = mode.lowpass();

		//for (int i = 0; i < NUM_MODES; i++)
		//{
		//	//mode[i].
		//}

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
		// Inputs
		addInput(createInputCentered<BananutBlack>(mm2px(Vec(7.62f, 22.14f)), module, Modal::AUDIO_INPUT));
		// Ouputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62f, 99.852f)), module, Modal::AUDIO_OUTPUT));
	}
	
	void appendContextMenu(Menu* menu) override
	{
		//Modal* module = dynamic_cast<Modal*>(this->module);

		menu->addChild(new MenuSeparator);
	}

};

Model* modelModal = createModel<Modal, ModalModuleWidget>("Modal");