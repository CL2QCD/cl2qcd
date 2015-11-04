/** @file
 * Definition of staggered fermionmatrix classes.
 *
 * Copyright (c) 2013 Alessandro Sciarra <sciarra@th.physik.uni-frankfurt.de>
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

#ifndef _PHYSICS_FERMIONMATRIX_FERMIONMATRIX_STAGG_
#define _PHYSICS_FERMIONMATRIX_FERMIONMATRIX_STAGG_

#include "../../hardware/code/fermions_staggered.hpp"

#include "../lattices/staggeredfield_eo.hpp"
#include "../lattices/gaugefield.hpp"
#include "../../hardware/device.hpp"
#include "fermionmatrixInterfaces.hpp"

/**
 * This is the definition of the classes "Fermionmatrix_stagg"
 */
namespace physics {

namespace fermionmatrix {

/*
 * Explicit Fermion operations
 */
void DKS_eo(const physics::lattices::Staggeredfield_eo * out, const physics::lattices::Gaugefield& gf, const physics::lattices::Staggeredfield_eo& in, int evenodd);


/**
 * A generic staggered fermion matrix
 */
class Fermionmatrix_stagg_basic {
public:
	/**
	 * Get if the matrix is hermitian
	 */
	bool is_hermitian() const noexcept;
	/**
	 * Get the net flops performed by this function.
	 */
	virtual cl_ulong get_flops() const = 0;
	virtual ~Fermionmatrix_stagg_basic(){}

protected:
	Fermionmatrix_stagg_basic(const hardware::System& system, const FermionmatrixStaggeredParametersInterface& fermionmatrixStaggeredParametersInterface,
	                          bool herm, hmc_float _mass = ARG_DEF)
        : fermionmatrixStaggeredParametersInterface(fermionmatrixStaggeredParametersInterface),
          _is_hermitian(herm), mass((_mass == ARG_DEF) ? system.get_inputparameters().get_mass() : _mass), system(system) {};

	/**
	 * Get the mass of the fermion.
	 */
	virtual hmc_float get_mass() const = 0;
	
	/**
	 * Get the system to operate on.
	 */
	const hardware::System& get_system() const noexcept;
	const FermionmatrixStaggeredParametersInterface& fermionmatrixStaggeredParametersInterface;

private:
	/**
	 * Shows if matrix is hermitian.
	 */
	const bool _is_hermitian;

	/**
	 * Mass of the fermion.
	 */
	const hmc_float mass;

	/**
	 * The system we are operating on.
	 */
	const hardware::System& system;
};


/**
 * A generic fermion matrix (with even-odd preconditioning)
 */
class Fermionmatrix_stagg_eo : public Fermionmatrix_stagg_basic {
public:
	/**
	 * Invoke the matrix function.
	 */
	virtual void operator() (const physics::lattices::Staggeredfield_eo * out, const physics::lattices::Gaugefield& gf, const physics::lattices::Staggeredfield_eo& in) const = 0;
	virtual hmc_float get_mass() const = 0;
	virtual ~Fermionmatrix_stagg_eo(){}

protected:
	Fermionmatrix_stagg_eo(const hardware::System& system, const FermionmatrixStaggeredParametersInterface& fermionmatrixStaggeredParametersInterface,
	                       bool herm, hmc_float _mass = ARG_DEF) : Fermionmatrix_stagg_basic(system, fermionmatrixStaggeredParametersInterface, herm, _mass) {};
};


/**
 * Actual fermion matrices (using even-odd)
 */
class D_KS_eo final : public Fermionmatrix_stagg_eo {
public:
	D_KS_eo(const hardware::System& system,  const FermionmatrixStaggeredParametersInterface& fermionmatrixStaggeredParametersInterface, bool evenodd)
        : Fermionmatrix_stagg_eo(system, fermionmatrixStaggeredParametersInterface, false), evenodd(evenodd) {};
	void operator() (const physics::lattices::Staggeredfield_eo * out, const physics::lattices::Gaugefield& gf, const physics::lattices::Staggeredfield_eo& in) const override;
	cl_ulong get_flops() const override;
	hmc_float get_mass() const override;
private:
	//This variable is to switch between the Deo and Doe:
	//   evenodd==EVEN  ---> Deo
	//   evenodd==ODD   ---> Doe
	bool evenodd;
};


class MdagM_eo final : public Fermionmatrix_stagg_eo {
public:
	MdagM_eo(const hardware::System& system, const physics::FermionStaggeredEoParametersInterface& fermionStaggeredEoParametersInterface, hmc_float _mass, bool ul=EVEN)
        : Fermionmatrix_stagg_eo(system, fermionStaggeredEoParametersInterface, true, _mass), tmp(system, fermionStaggeredEoParametersInterface), upper_left(ul) {};
	void operator() (const physics::lattices::Staggeredfield_eo * out, const physics::lattices::Gaugefield& gf, const physics::lattices::Staggeredfield_eo& in) const override;
	cl_ulong get_flops() const override;
	hmc_float get_mass() const noexcept override;
	bool get_upper_left() const;
private:
	physics::lattices::Staggeredfield_eo tmp;
	//This variable is to switch between the upper_left and the lower-right block
	//of the MdagM matrix: upper_left==true  ---> MdagM_eo = mass**2 - Deo*Doe
	//                     upper_left==false ---> MdagM_eo = mass**2 - Doe*Deo
	bool upper_left;
};



}
}
#endif /* _PHYSICS_FERMIONMATRIX_FERMIONMATRIX_STAGG_ */
