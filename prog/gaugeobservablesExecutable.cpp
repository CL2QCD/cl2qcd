/*
 * Copyright 2012, 2013 Lars Zeidlewicz, Christopher Pinke,
 * Matthias Bach, Christian Schäfer, Stefano Lottini, Alessandro Sciarra
 *
 * This file is part of CL2QCD.
 *
 * CL2QCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CL2QCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CL2QCD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gaugeobservablesExecutable.h"

gaugeobservablesExecutable::gaugeobservablesExecutable(int argc, const char* argv[]) : measurementExecutable(argc, argv)
{
	logger.info() << "This executable requires the following parameter value(s) to work properly:";
	logger.info() << "startcondition:\tcontinue";
	if(parameters.get_startcondition() != meta::Inputparameters::start_from_source ) {
		logger.fatal() << "Found wrong startcondition! Aborting..";
		throw Invalid_Parameters("Found wrong startcondition!", "continue", parameters.get_startcondition());
	}
}

inline void gaugeobservablesExecutable::writeGaugeobservablesLogfile()
{
	outputToFile.open(filenameForGaugeobservablesLogfile);
	if (outputToFile.is_open()) {
		meta::print_info_inverter(ownName, &outputToFile, parameters);
		outputToFile.close();
	} else {
		throw File_Exception(filenameForGaugeobservablesLogfile);
	}
}

void gaugeobservablesExecutable::printParametersToScreenAndFile()
{
	meta::print_info_gaugeobservables(ownName, parameters);
	writeGaugeobservablesLogfile();
}

inline void gaugeobservablesExecutable::performApplicationSpecificMeasurements()
{
	logger.info() << "Measure gauge observables on configuration: " << currentConfigurationName;
	if(parameters.get_print_to_screen() ) {
		print_gaugeobservables(*gaugefield, gaugefield->get_parameters_source().trajectorynr_source);
	}
	filenameForGaugeobservables = get_gauge_obs_file_name(parameters, currentConfigurationName);
	print_gaugeobservables(*gaugefield, gaugefield->get_parameters_source().trajectorynr_source, filenameForGaugeobservables);
}

int main(int argc, const char* argv[])
{
	try{
		gaugeobservablesExecutable gaugeobservablesInstance(argc, argv);
		gaugeobservablesInstance.performMeasurements();
	} //try
	//exceptions from Opencl classes
	catch (Opencl_Error& e) {
		logger.fatal() << e.what();
		exit(1);
	} catch (File_Exception& fe) {
		logger.fatal() << "Could not open file: " << fe.get_filename();
		logger.fatal() << "Aborting.";
		exit(1);
	} catch (Print_Error_Message& em) {
		logger.fatal() << em.what();
		exit(1);
	} catch (Invalid_Parameters& es) {
		logger.fatal() << es.what();
		exit(1);
	}

	return 0;
}

