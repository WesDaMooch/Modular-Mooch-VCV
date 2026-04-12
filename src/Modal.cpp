//export RACK_DIR=/home/wes-l/Rack-SDK

#include "plugin.hpp"
#include <array>
#include <vector>
#include "Modal\resonator.hpp"
#include "Modal\chamberlinSVF.hpp"
#include "Modal\bessel.hpp"

// Ideas
// Multiple layers of modes

// Params
// 
// Resonator
// Pitch
// Shape
// Harmo

/*
		1.0,
		1.59,
		2.14,
		2.30,
		2.65,
		2.92,
		3.16,
		3.5,
		3.6,
		3.65,
		4.06,
		4.15
*/

struct Modal : Module
{
	enum ParamId
	{
		FREQ_PARAM,
		SIZE_PARAM,
		POSITION_PARAM,
		DAMPING_PARAM,
		BRIGHTNESS_PARAM,
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
	//std::vector<IIRResonator> resonator;
	std::vector<ChamberlinSVF> resonator;
	//std::vector<dsp::BiquadFilter> resonator;
	
	Drum drum;

	Modal() 
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Parameter
		configParam(FREQ_PARAM, 20.f, 1000.f, 220.f, "Freq", "Hz");
		configParam(SIZE_PARAM, 0.f, 1.f, 0.5f, "Size");
		configParam(POSITION_PARAM, 0.f, 1.f, 0.5f, "Position");
		configParam(DAMPING_PARAM, 0.f, 1.f, 0.5f, "Damping");
		configParam(BRIGHTNESS_PARAM, 0.f, 1.f, 0.5f, "Brightness");
		// Input
		configInput(AUDIO_INPUT, "Input");
		// Output
		configOutput(AUDIO_OUTPUT, "Output");

		onSampleRateChange();

		for (int i = 0; i < MAX_MODES; i++)
		{
			resonator.emplace_back();
		}
	}


	void onSampleRateChange() override 
	{
		srate = APP->engine->getSampleRate();
		drum.setSamplerate(srate);
	}

	void onReset(const ResetEvent& e) override 
	{
		for (int i = 0; i < MAX_MODES; i++)
		{
			resonator[i].reset();
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
		//float maxDecayTime = 10.f;
		//float decayParam = params[DECAY_PARAM].getValue();
		//float decay = decayParam * maxDecayTime;

		float output = 0.f;
		float input = inputs[AUDIO_INPUT].getVoltage();
		// Convert to digital audio range (-1 tp +1)
		input *= 0.1;
		input = rack::clamp(input, -1.f, 1.f);

		/*
		drum.setPitch(fundimentalFreq);
		//bessel.setSize(1.f);
		//bessel.setPosition(0.3f);
		//bessel.setDamping(0.5f);
		//bessel.setOvertones(0.5f);
		drum.update();

		for (int i = 0; i < MAX_MODES; i++)
		{
			//float amplitude = 1.f;
			float amplitude = drum.getWeight(i);
			float freq = drum.getFreq(i);

			resonator[i].set(srate, freq, decay, amplitude);
			output += resonator[i].proccess(input);
		}

		output = output / (float)MAX_MODES;
		*/

		drum.setPitch(params[FREQ_PARAM].getValue()); // Pitch should be size?
		drum.setSize(params[SIZE_PARAM].getValue() * 5.f);
		drum.setPosition(params[POSITION_PARAM].getValue());
		drum.setDamping(params[DAMPING_PARAM].getValue());
		drum.setOvertones(params[BRIGHTNESS_PARAM].getValue());
		drum.update(); // slow

		for (int i = 0; i < MAX_MODES; i++)
		{
			//float amplitude = 1.f;
			float weight = drum.getWeight(i);
			float freq = drum.getFreq(i);
			float decay = 100.f;

			//float freq = params[FREQ_PARAM].getValue() * (i + 1);
			//float weight = 20.f;

			resonator[i].setCoefficients(freq, decay, srate);
			resonator[i].process(input);
			output += weight * resonator[i].bandpass();
			//output += resonator[i].bandpass();
		}

		output = output / (float)MAX_MODES;;

		// Convert to voltage range (-10 to +10)
		output *= 10.f;

		outputs[AUDIO_OUTPUT].setVoltage(output);
		//outputs[AUDIO_OUTPUT].setVoltage(drum.getFreq(0));
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
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 40.f)), module, Modal::SIZE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 60.f)), module, Modal::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 80.f)), module, Modal::DAMPING_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.f, 100.f)), module, Modal::BRIGHTNESS_PARAM));
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