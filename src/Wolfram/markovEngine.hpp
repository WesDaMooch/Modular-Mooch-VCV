// markovEngine.hpp
// Part of the Modular Mooch Wolfram module (VCV Rack)
//
// GitHub: https://github.com/WesDaMooch/Modular-Mooch-VCV
// 
// Copyright (c) 2026 Wesley Lawrence Leggo-Morrell
// License: GPL-3.0-or-later

// Markov chain
// 
// Zero order - only consider the probability of a state occuring
// First order - next state is based on the current state
// Second order - choose the next state based on the 2 previous states & the probability of subsequent states following those two states
// 
// Could have a level of forgetfullness?


#pragma once
#include "algoEngine.hpp"

class MarkovEngine : public AlgoEngine {
public:
	MarkovEngine();

	void updateDisplay(bool advance, size_t length = 8) override;
	void updateMenuParams(const EngineMenuParams& p) override;

	void reinitialise() override;
	void process(const EngineCoreParams& p, EngineOutput& output) override;

	// Save setters
	void setBufferFrame(uint64_t newFrame, int index, bool setDisplayMatrix = false) override;

	void setRuleSelect(int newRule) override;
	void setRuleCv(float newRuleCv) override;
	void setSeed(int newSeed) override;
	void setMode(int newMode) override;

	// Save getters 
	uint64_t getBufferFrame(int index, bool getDisplayMatrix = false, bool getDisplayMatrixSave = false) override;

	int getRuleSelect() override;
	int getSeed() override;
	int getMode() override;

	// UI getters
	void getRuleActiveLabel(char out[5]) override;
	void getRuleSelectLabel(char out[5]) override;
	void getSeedLabel(char out[5]) override;
	void getModeLabel(char out[5]) override;

protected:
	std::array<uint8_t, MAX_SEQUENCE_LENGTH> rowBuffer{};
	uint64_t internalDisplayMatrix = 0;

	// Zero order
	// Only consider the probability of a state occuring based on previous states
	std::array<uint64_t, UINT8_MAX + 1> zeroOrderStateCount{};
		
	
	static constexpr int MAX_RULE = 2;
	static constexpr int ruleDefault = 0;
	int ruleSelect = ruleDefault;
	int ruleCv = 0;
	int rule = 0;

	static constexpr int NUM_MODES = 2;
	static const char modeLabel[NUM_MODES][5];
	static constexpr int modeDefault = 1;
	int modeIndex = modeDefault;

	static constexpr  float voltageScaler = 1.f / UINT8_MAX;
	static constexpr  float modeScaler = 1.f / (static_cast<float>(NUM_MODES) - 1.f);

	void onGenerate();
	void learn();
	void resetToSeed(bool sync); //seeds: random, drunken walk?
	void inject(int inject, bool sync);
	void onRuleChange();
	void renderOutput(EngineOutput& output);
};