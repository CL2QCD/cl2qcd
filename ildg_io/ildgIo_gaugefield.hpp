/** @file
 * Reading of gauge field from files.
 *
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

#ifndef _READGAUGEH_
#define _READGAUGEH_

#include "../common_header_files/types.h"
#include "../executables/exceptions.h"

//todo: remove this eventually
#include "limeUtilities.hpp"
#include "SourcefileParameters_values.hpp"

#include "checksum.h"
extern "C" {
#include <lime.h>
}

//todo: add namespace ildg_io

/**
 * Parser class for a stored gaugefield.
 *
 * Contains metadata of the parsed gaugefield as members.
 */
class sourcefileparameters : public sourcefileparameters_values {
public:
	sourcefileparameters() : sourcefileparameters_values() {};
	sourcefileparameters(const meta::Inputparameters * parameters, int trajectoryNumber, double plaquette, std::string hmcVersion) : sourcefileparameters_values(parameters, trajectoryNumber, plaquette, hmcVersion) {};
	
	/**
	 * Read gauge configuration from the given file into the given array.
	 *
	 * @param[in] file      The file to read the gauge configuration from
	 * @param[in] precision The precision expected for the gaugefield.
	 * @param[out] array    The loaded gaugefield
	 */
  void readsourcefile(std::string file, int precision, char ** data);
	
 private:
	void readMetaDataFromLimeFile();
	void readDataFromLimeFile(char ** destination);
	int calcNumberOfEntriesBasedOnFieldType(std::string fieldType);
	LimeFileProperties extractMetaDataFromLimeEntry(LimeReader * r, LimeHeaderData limeHeaderData);
	size_t	sizeOfGaugefieldBuffer();
	void extractBinaryDataFromLimeEntry(LimeReader * r, LimeHeaderData limeHeaderData, char ** destination);
	void readLimeFile(char ** destination);
	void extractMetadataFromLimeFile();
	void extractDataFromLimeFile(char ** destination);
	void extractInformationFromLimeEntry(LimeReader * r, char ** destination);
	void goThroughLimeRecords(LimeReader * r, char ** destination);

	int checkLimeEntryForFermionInformations(std::string lime_type);
	bool checkLimeEntryForBinaryData(std::string lime_type);

	LimeFilePropertiesCollector limeFileProp;
	
	std::string sourceFilename;
	int desiredPrecision;
	LimeEntryTypes limeEntryTypes;
};

/**
 * Write the gaugefield to a file
 *
 * \param array The float array representing the gaugefield.
 * \param array_size The number of floats in the gaugefield array.
 *
 * \todo complete documentation
 */
void write_gaugefield (
  char * binary_data, n_uint64_t num_bytes, Checksum checksum, sourcefileparameters_values srcFileParameters_values, std::string filename);


#endif /* _READGAUGEH_ */
