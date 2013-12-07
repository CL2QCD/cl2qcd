/*
 * @file
 * Declaration of the generationExecutable class.
 * This class provides features for the generation of gauge configurations
 * according to certain algorithms.
 */

#include "generalExecutable.h"

class generationExecutable : public generalExecutable
{
public:
	generationExecutable(int argc, const char* argv[]);

protected:
	int writeFrequency;
	int saveFrequency;
	int thermalizationSteps;
	std::string filenameForGaugeobservables;

	void setIterationParameters();

	void writeGaugeObservablesToFile(int& iteration);

	void writeGaugeObservablesToScreen(int& iteration);

	void writeGaugeObservablesToScreenAndFile(int iteration);
};


