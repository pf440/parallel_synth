#include "Envelope.h"
#include <cmath>

double Envelope::nextSample() {
	if (currentStage != ENVELOPE_STAGE_OFF &&
		currentStage != ENVELOPE_STAGE_SUSTAIN) {
		if (currentSampleIndex == nextStageSampleIndex) {
			EnvelopeStage newStage = static_cast<EnvelopeStage>(
				(currentStage + 1) % kNumEnvelopeStages
				);
			enterStage(newStage);
		}
		currentLevel *= multiplier;
		currentSampleIndex++;
	}
	return currentLevel;
}

void Envelope::calculateMultiplier(double startLevel,
	double endLevel,
	unsigned long long lengthInSamples) {
	multiplier = 1.0 + (log(endLevel) - log(startLevel)) / (lengthInSamples);
}

void Envelope::enterStage(EnvelopeStage newStage) {
	currentStage = newStage;
	currentSampleIndex = 0;
	if (currentStage == ENVELOPE_STAGE_OFF ||
		currentStage == ENVELOPE_STAGE_SUSTAIN) {
		nextStageSampleIndex = 0;
	}
	else {
		nextStageSampleIndex = stageValue[currentStage] * sampleRate;
	}
	switch (newStage) {
	case ENVELOPE_STAGE_OFF:
		currentLevel = 0.0;
		multiplier = 1.0;
		break;
	case ENVELOPE_STAGE_ATTACK:
		currentLevel = minimumLevel;
		calculateMultiplier(currentLevel,
			1.0,
			nextStageSampleIndex);
		break;
	case ENVELOPE_STAGE_DECAY:
		currentLevel = 1.0;
		calculateMultiplier(currentLevel,
			fmax(stageValue[ENVELOPE_STAGE_SUSTAIN], minimumLevel),
			nextStageSampleIndex);
		break;
	case ENVELOPE_STAGE_SUSTAIN:
		currentLevel = stageValue[ENVELOPE_STAGE_SUSTAIN];
		multiplier = 1.0;
		break;
	case ENVELOPE_STAGE_RELEASE:
		// We could go from ATTACK/DECAY to RELEASE,
		// so we're not changing currentLevel here.
		calculateMultiplier(currentLevel,
			minimumLevel,
			nextStageSampleIndex);
		break;
	default:
		break;
	}
}

void Envelope::setSampleRate(double newSampleRate) {
	sampleRate = newSampleRate;
}

void Envelope::setStageValue(EnvelopeStage stage,
	double value) {
	stageValue[stage] = value;
	if (stage == currentStage) {
		// Re-calculate the multiplier and nextStageSampleIndex
		if (currentStage == ENVELOPE_STAGE_ATTACK ||
			currentStage == ENVELOPE_STAGE_DECAY ||
			currentStage == ENVELOPE_STAGE_RELEASE) {
			double nextLevelValue;
			switch (currentStage) {
			case ENVELOPE_STAGE_ATTACK:
				nextLevelValue = 1.0;
				break;
			case ENVELOPE_STAGE_DECAY:
				nextLevelValue = fmax(stageValue[ENVELOPE_STAGE_SUSTAIN], minimumLevel);
				break;
			case ENVELOPE_STAGE_RELEASE:
				nextLevelValue = minimumLevel;
				break;
			default:
				break;
			}
			// How far the generator is into the current stage:
			double currentStageProcess = (currentSampleIndex + 0.0) / nextStageSampleIndex;
			// How much of the current stage is left:
			double remainingStageProcess = 1.0 - currentStageProcess;
			unsigned long long samplesUntilNextStage = remainingStageProcess * value * sampleRate;
			nextStageSampleIndex = currentSampleIndex + samplesUntilNextStage;
			calculateMultiplier(currentLevel, nextLevelValue, samplesUntilNextStage);
		}
		else if (currentStage == ENVELOPE_STAGE_SUSTAIN) {
			currentLevel = value;
		}

		if (currentStage == ENVELOPE_STAGE_DECAY &&
			stage == ENVELOPE_STAGE_SUSTAIN) {
			// We have to decay to a different sustain value than before.
			// Re-calculate multiplier:
			unsigned long long samplesUntilNextStage = nextStageSampleIndex - currentSampleIndex;
			calculateMultiplier(currentLevel,
				fmax(stageValue[ENVELOPE_STAGE_SUSTAIN], minimumLevel),
				samplesUntilNextStage);
		}
	}
}