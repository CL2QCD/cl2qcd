/*
 * Copyright 2015 Christopher Pinke
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

# pragma once

#include "SpinorTester.hpp"
#include "GaugefieldTester.hpp"
#include "gaugefield.hpp"
#include "fermions.hpp"

const double nonTrivialParameter = 0.123456;

struct WilsonMassParameters
{
	WilsonMassParameters(const double kappaIn) : kappa(kappaIn){};
	const double kappa;
};

struct TwistedMassMassParameters : public WilsonMassParameters
{
	TwistedMassMassParameters(const double kappaIn, const double muIn):
		WilsonMassParameters(kappaIn), mu(muIn) {}

	const double mu;

	double getMubar() const { return 2.*kappa*mu; }
};

struct FermionTestParameters : public SpinorTestParameters, GaugefieldTestParameters
{
	FermionTestParameters(const LatticeExtents lE, const SpinorFillType spinorFillTypeIn, const GaugefieldFillType gaugefieldFillTypesIn) :
		SpinorTestParameters(lE, SpinorFillTypes{spinorFillTypeIn}),
		GaugefieldTestParameters(defaultReferenceValues(), lE, gaugefieldFillTypesIn) {};
	FermionTestParameters(const LatticeExtents lE, const SpinorFillType spinorFillTypeIn) :
		SpinorTestParameters(lE, SpinorFillTypes{spinorFillTypeIn}),
		GaugefieldTestParameters(defaultReferenceValues(), lE, GaugefieldFillType::cold) {};
	FermionTestParameters(const LatticeExtents lE, const SpinorFillTypes spinorFillTypesIn) :
		SpinorTestParameters(lE, spinorFillTypesIn),
		GaugefieldTestParameters(defaultReferenceValues(), lE, GaugefieldFillType::cold) {};
};

template< class MassParameters>
 struct FermionMatrixTestParameters : public FermionTestParameters
{
	FermionMatrixTestParameters(const LatticeExtents latticeExtentsIn, const SpinorFillType spinorFillTypeIn, const GaugefieldFillType gaugefieldFillTypeIn,
			const MassParameters massParametersIn) :
		FermionTestParameters(latticeExtentsIn, spinorFillTypeIn, gaugefieldFillTypeIn), massParameters(massParametersIn) {};
	FermionMatrixTestParameters(const LatticeExtents latticeExtentsIn, const SpinorFillType spinorFillTypeIn, const MassParameters massParametersIn ) :
		FermionTestParameters(latticeExtentsIn, spinorFillTypeIn, GaugefieldFillType::cold), massParameters(massParametersIn) {};
	const MassParameters massParameters;
};

typedef FermionMatrixTestParameters<WilsonMassParameters> WilsonTestParameters;
typedef FermionMatrixTestParameters<TwistedMassMassParameters> TwistedMassTestParameters;

template <class BufferType, class TesterType>
struct FermionmatrixTester : public TesterType
{
	FermionmatrixTester(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV, const int numberOfElements) :
		TesterType(kernelName, parameterCollection, testParameters, rV)
	{
		gaugefieldBuffer = new hardware::buffers::SU3( calculateGaugefieldSize(testParameters.SpinorTestParameters::latticeExtents), this->device);
		const Matrixsu3 * gf_host = createGaugefield(calculateGaugefieldSize(testParameters.SpinorTestParameters::latticeExtents), testParameters.GaugefieldTestParameters::fillType);
		this->device->getGaugefieldCode()->importGaugefield(gaugefieldBuffer, gf_host);
		delete[] gf_host;

		code = SpinorTester::device->getFermionCode();

		in = new const BufferType(numberOfElements, this->device);
		out = new const BufferType(numberOfElements, this->device);
		out->load(TesterType::createSpinorfield(SpinorFillType::zero));
		in->load(TesterType::createSpinorfield(testParameters.SpinorTestParameters::fillTypes.at(0)));
	}
protected:
	const BufferType * in;
	const BufferType * out;
	const hardware::code::Fermions * code;
	const hardware::buffers::SU3 * gaugefieldBuffer;
};

struct NonEvenOddFermionmatrixTester : public FermionmatrixTester<hardware::buffers::Plain<spinor>, NonEvenOddSpinorTester>
{
	NonEvenOddFermionmatrixTester(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV) :
		FermionmatrixTester<hardware::buffers::Plain<spinor>, NonEvenOddSpinorTester>(kernelName, parameterCollection, testParameters, rV, calculateSpinorfieldSize(testParameters.SpinorTestParameters::latticeExtents)) {}
};

struct NonEvenOddFermionmatrixTesterWithSquarenormAsResult : public NonEvenOddFermionmatrixTester
{
	NonEvenOddFermionmatrixTesterWithSquarenormAsResult(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV) :
		NonEvenOddFermionmatrixTester(kernelName, parameterCollection, testParameters, rV) {};
	~NonEvenOddFermionmatrixTesterWithSquarenormAsResult()
	{
		calcSquarenormAndStoreAsKernelResult(out);
	}
};

struct EvenOddFermionmatrixTester : public FermionmatrixTester<hardware::buffers::Spinor, EvenOddSpinorTester>
{
	EvenOddFermionmatrixTester(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV) :
		FermionmatrixTester<hardware::buffers::Spinor, EvenOddSpinorTester>(kernelName, parameterCollection, testParameters, rV, calculateEvenOddSpinorfieldSize(testParameters.SpinorTestParameters::latticeExtents)) {}
};

struct FermionmatrixEvenOddTesterWithSquarenormAsKernelResult : public EvenOddFermionmatrixTester
{
	FermionmatrixEvenOddTesterWithSquarenormAsKernelResult(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV) :
		EvenOddFermionmatrixTester(kernelName, parameterCollection, testParameters, rV) {}
	~FermionmatrixEvenOddTesterWithSquarenormAsKernelResult()
	{
		calcSquarenormEvenOddAndStoreAsKernelResult(out);
	}
};

struct FermionmatrixEvenOddTesterWithSumAsKernelResult : public EvenOddFermionmatrixTester
{
	FermionmatrixEvenOddTesterWithSumAsKernelResult(std::string kernelName, const ParameterCollection parameterCollection, const FermionTestParameters testParameters, const ReferenceValues rV) :
		EvenOddFermionmatrixTester(kernelName, parameterCollection, testParameters, rV) {}
	~FermionmatrixEvenOddTesterWithSumAsKernelResult()
	{
		spinor * sf_in;
		sf_in = new spinor[spinorfieldEvenOddElements];
		out->dump(sf_in);
		kernelResult.at(0) = count_sf(sf_in, spinorfieldEvenOddElements);
		delete sf_in;
	}
};
