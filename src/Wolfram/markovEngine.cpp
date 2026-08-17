// wolfEngine.cpp
// Part of the Modular Mooch Wolfram module (VCV Rack)
//
// GitHub: https://github.com/WesDaMooch/Modular-Mooch-VCV
// 
// Copyright (c) 2026 Wesley Lawrence Leggo-Morrell
// License: GPL-3.0-or-later

#include "markovEngine.hpp"


const char MarkovEngine::modeLabel[MarkovEngine::NUM_MODES][5] = {
	"GEN",
	"LRN"
};


MarkovEngine::MarkovEngine() {
	snprintf(engineLabel, 5, "%4s", "MRKV");

	zeroOrderStateCount.fill(1);


	// TODO: set seed
	for (auto& row : rowBuffer)
		row = rack::random::get<uint8_t>();

	updateDisplay(false);
}


void MarkovEngine::updateDisplay(bool step, size_t length) {
	if (step) {
		advanceHeads(length);
		internalDisplayMatrix <<= 8;	// Shift matrix up
	}

	internalDisplayMatrix &= ~0xFFULL;
	internalDisplayMatrix |= rowBuffer[readHead];

	// Apply offset
	uint64_t tempMatrix = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t row = (internalDisplayMatrix >> (i * 8)) & 0xFFULL;
		tempMatrix |= uint64_t(applyOffset(row, offset)) << (i * 8);
	}
	displayMatrix = tempMatrix;
	displayMatrixUpdated = true;
}


void MarkovEngine::updateMenuParams(const EngineMenuParams& p) {
	// Rule

	// Seed

	// Mode
	int newModeSelect = updateSelect(p.menuDelta[EngineMenuParams::MODE_DELTA],
		p.menuReset[EngineMenuParams::MODE_RESET],
		modeIndex, modeDefault, NUM_MODES);
	setMode(newModeSelect);
};


void MarkovEngine::onGenerate() {
	switch (rule) {
	case 0: {
		// Zero order
			uint64_t totalCount = 0;

			for (uint64_t count : zeroOrderStateCount)
				totalCount += count;

			uint64_t randInt = static_cast<uint64_t>(rack::random::uniform() * totalCount);

			for (size_t state = 0; state < zeroOrderStateCount.size(); state++) {
				if (randInt < zeroOrderStateCount[state]) {
					rowBuffer[writeHead] = static_cast<uint8_t>(state);
					break;
				}

				randInt -= zeroOrderStateCount[state];
			}

		break;
	}
	case 1: {
		// First order
		rowBuffer[writeHead] = 2;
		break;
	}
	case 2: {
		// Second order
		rowBuffer[writeHead] = 3;
		break;
	}
	default: { break; };
	}
}

void MarkovEngine::learn() {
	switch (rule) {
	case 0:
		// Zero order
		zeroOrderStateCount[rowBuffer[readHead]] += 10;
	
		break;
	case 1:
		// First order

		break;

	case 2:
		// Second order

		break;
	
	default:
		break;
	}
}


void MarkovEngine::resetToSeed(bool sync) {
	//size_t head = sync ? writeHead : readHead;
}


void MarkovEngine::inject(int inject, bool sync) {
	//size_t head = sync ? writeHead : readHead;
	
}


void MarkovEngine::renderOutput(EngineOutput& output) {

	uint8_t firstRow = displayMatrix & 0xFFULL;
	output.voltage[0] = firstRow * voltageScaler;

	// Mode LED brightness
	output.modeLED = static_cast<float>(modeIndex) * modeScaler;
}


void MarkovEngine::reinitialise() {
	for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++)
		setBufferFrame(0, i);

	setBufferFrame(0, 0, true);
	setReadHead(0);
	setWriteHead(1);
	//setRuleSelect(ruleDefault);
	//setSeed(seedDefault);
	//setMode(modeDefault);

	//rowBuffer[readHead] = seed;
	updateDisplay(false);
}

void MarkovEngine::process(const EngineCoreParams& p, EngineOutput& output) {

	// Sequencer
	bool refreshDisplay = p.step;
	bool syncStep = p.sync && p.step;
	generate = rack::random::get<float>() < p.probability;

	if (!p.sync || (syncStep))
		setRuleCv(p.ruleCv);

	bool injectOccured = (p.inject != 0);
	if (injectOccured && p.sync)
		injectPending += p.inject;

	// Non-sync inject
	if (injectOccured && !p.sync) {
		inject(p.inject, p.sync);
		refreshDisplay = true;
	}

	// Reset
	bool seedReset = (p.miniMenuChanged && generate) && !p.sync;

	if (p.miniMenuChanged && p.sync)
		seedResetPending = true;

	if (p.reset && p.sync)
		resetPending = true;

	if (((p.reset || seedReset) && !p.sync) || ((resetPending || seedResetPending) && syncStep)) {
		if (generate) {
			resetToSeed(p.sync);
			generate = false;
		}
		else if (!seedResetPending) {
			// Sequence reset
			if (p.sync) {
				writeHead = 0;
			}
			else {
				readHead = 0;
				writeHead = 1;
			}
		}
		resetPending = false;
		seedResetPending = false;
		refreshDisplay = true;
	}

	// Generate
	if (generate && p.step) {
		onGenerate();
	}

	if (p.step && (modeIndex == 1)) {
		learn();
	}

	// Sync inject
	if (injectPending && syncStep) {
		inject(injectPending, p.sync);
		injectPending = 0;
	}

	// Offset
	int newOffset = p.offset - 4;
	if ((!p.sync && (offset != newOffset)) || syncStep) {
		offset = newOffset;
		refreshDisplay = true;
	}

	// Update
	if (refreshDisplay)
		updateDisplay(p.step, p.length);

	// Render output
	renderOutput(output);
	displayMatrixUpdated = false;
}


// Save setters
void MarkovEngine::setBufferFrame(uint64_t newFrame, int index, bool setDisplayMatrix) {

}


void MarkovEngine::onRuleChange() {
	
}


void MarkovEngine::setRuleSelect(int newRule) {
	
	onRuleChange();
}


void MarkovEngine::setRuleCv(float newRuleCv) {
	
	onRuleChange();
}


void MarkovEngine::setSeed(int newSeed) {

}


void MarkovEngine::setMode(int newMode) {
	if (newMode == modeIndex)
		return;

	modeIndex = rack::clamp(newMode, 0, NUM_MODES - 1);
}


// Save getters
uint64_t MarkovEngine::getBufferFrame(int index, bool getDisplayMatrix, bool getDisplayMatrixSave) {
	if (getDisplayMatrix)
		return displayMatrix;
	else if (getDisplayMatrixSave)
		return internalDisplayMatrix;
	else if ((index >= 0) && (index < MAX_SEQUENCE_LENGTH))
		return static_cast<uint64_t>(rowBuffer[index]);
	else
		return 0;
}


int MarkovEngine::getRuleSelect() {
	return 0;
}


int MarkovEngine::getSeed() {
	return 0;
}


int MarkovEngine::getMode() {
	return modeIndex;
}


// UI getters
void MarkovEngine::getRuleActiveLabel(char out[5]) {
	
}


void MarkovEngine::getRuleSelectLabel(char out[5]) {
	
}


void MarkovEngine::getSeedLabel(char out[5]) {
	
}


void MarkovEngine::getModeLabel(char out[5]) {
	snprintf(out, 5, "%4s", modeLabel[modeIndex]);
}