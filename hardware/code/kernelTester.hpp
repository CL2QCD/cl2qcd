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

#pragma once

#include <vector>
#include <boost/test/unit_test.hpp>
#include "../../host_functionality/logger.hpp"
#include "testUtilities.hpp"
#include "../system.hpp"
#include "../device.hpp"
#include "../interfaceMockups.hpp"
#include "latticeExtents.hpp"

enum ComparisonType{difference=1, smallerThan};

const double nonTrivialParameter = 0.123456;

typedef std::vector<double> ReferenceValues;
ReferenceValues defaultReferenceValues();

struct TestParameters
{
	int ns;
	int nt;
	LatticeExtents latticeExtents;
	ComparisonType typeOfComparison;
	const double testPrecision = 10e-8;

	TestParameters(const LatticeExtents latticeExtentsIn):
		ns(latticeExtentsIn.ns), nt(latticeExtentsIn.nt), latticeExtents(latticeExtentsIn), typeOfComparison(ComparisonType::difference) {}
	TestParameters(const LatticeExtents latticeExtentsIn, const ComparisonType typeOfComparisonIn):
		ns(latticeExtentsIn.ns), nt(latticeExtentsIn.nt), latticeExtents(latticeExtentsIn),typeOfComparison(typeOfComparisonIn) {}
	TestParameters() = delete;
};

struct ParameterCollection
{
	ParameterCollection(const hardware::HardwareParametersInterface & hardwareParametersIn, const hardware::code::OpenClKernelParametersInterface & kernelParametersIn):
		hardwareParameters(hardwareParametersIn), kernelParameters(kernelParametersIn) {};
	const hardware::HardwareParametersInterface & hardwareParameters;
	const hardware::code::OpenClKernelParametersInterface & kernelParameters;
};

struct KernelTester
{
	KernelTester(std::string kernelNameIn, const hardware::HardwareParametersInterface&,
			const hardware::code::OpenClKernelParametersInterface&, struct TestParameters, const ReferenceValues);
	virtual ~KernelTester();
	
protected:
	const TestParameters testParameters;
	std::vector<double> kernelResult;
	ReferenceValues referenceValues;
	
	const hardware::System * system;
	hardware::Device * device;
	const hardware::HardwareParametersInterface * hardwareParameters;
	const hardware::code::OpenClKernelParametersInterface * kernelParameters;
};

