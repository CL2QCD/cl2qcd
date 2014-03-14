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

#include "checksum.h"
extern "C" {
#include <lime.h>
}

class LimeHeaderData
{
public:
  LimeHeaderData(LimeReader *r)
  {
    numberOfBytes    = limeReaderBytes(r);
    limeEntryType = (limeReaderType(r));
    bytes_pad = limeReaderPadBytes(r);
    MB_flag   = limeReaderMBFlag(r);
    ME_flag   = limeReaderMEFlag(r);
  }

  n_uint64_t numberOfBytes;
  size_t bytes_pad;
  int MB_flag, ME_flag;
  std::string limeEntryType;
};

/**
 * Parser class for a stored gaugefield.
 *
 * Contains metadata of the parsed gaugefield as members.
 */
class sourcefileparameters {
public:
  sourcefileparameters() {
		set_defaults();
		numberOfFermionFieldsRead = 0;
	};
	/**
	 * Read gauge configuration from the given file into the given array.
	 *
	 * @param[in] file      The file to read the gauge configuration from
	 * @param[in] precision The precision expected for the gaugefield.
	 * @param[out] array    The loaded gaugefield
	 */
  void readsourcefile(std::string file, int precision, char ** data);
	void set_defaults();
	void val_assign_source(int * out, int in);
	void val_assign_source(hmc_float * out, hmc_float in);
	int lx_source, ly_source, lz_source, lt_source, prec_source, num_entries_source, flavours_source,
	    trajectorynr_source, time_source, time_solver_source, noiter_source;
	hmc_float plaquettevalue_source, beta_source, kappa_source, mu_source, c2_rec_source, mubar_source, epsilonbar_source,
	          epssq_source, kappa_solver_source, mu_solver_source;
	char field_source[100];
	char hmcversion_source[50];
	char date_source[50];
	char solvertype_source[50];
	char hmcversion_solver_source[50];
	char date_solver_source[50];
	Checksum checksum;

 private:
	void readMetaDataFromLimeFile(std::string sourceFilename);
	void get_XML_infos(const char * buffer, int size, const char * filename, char * field_out );
	void get_XLF_infos(const char * filename,char * hmcversion, char * date );
	void get_inverter_infos(const char * filename, char * solver, char * hmcversion, char * date );
	void printMetaDataToScreen(std::string sourceFilename);
	void readDataFromLimeFile(std::string sourceFilename, char * data, int desiredPrecision, size_t bytes);
	void checkPrecision(int desiredPrecision);
	int calcNumberOfEntriesBasedOnFieldType(char * fieldType);
	int calcNumberOfEntriesForDiracFermionfield();
	int calcNumberOfEntriesForGaugefield();
	void checkLimeEntry(std::string sourceFilename, int *  numberOfFermionEntries, LimeReader * r, LimeHeaderData limeHeaderData);
	void checkLimeEntryForInverterInfos(std::string lime_type, int switcher, LimeReader *r, size_t nbytes);
	void checkLimeEntryForXlfInfos(std::string lime_type, int switcher, LimeReader *r, size_t nbytes);
	void checkLimeEntryForXlmInfos(std::string lime_type, int switcher, LimeReader *r, size_t nbytes, std::string sourceFilename);
	void checkLimeEntryForScidacChecksum(std::string lime_type, LimeReader *r, size_t nbytes,  std::string sourceFilename);
	int numberOfFermionFieldsRead;
};

#endif /* _READGAUGEH_ */
