/** @file
 *
 * Copyright (c) 2014 Christopher Pinke <pinke@compeng.uni-frankfurt.de>
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

#ifndef _META_PARAMETERS_HMC_HPP_
#define _META_PARAMETERS_HMC_HPP_

#include "parametersBasic.hpp"

namespace meta {
class ParametersHmc {
public:
	enum integrator { leapfrog = 1, twomn };

	double get_tau() const noexcept;
	bool get_reversibility_check() const noexcept;
	int get_integrationsteps(size_t timescale) const noexcept;
	int get_hmcsteps() const noexcept;
	int get_num_timescales() const noexcept;
	integrator get_integrator(size_t timescale) const noexcept;
	double get_lambda(size_t timescale) const noexcept;
	bool get_use_gauge_only() const noexcept;
	bool get_use_mp() const noexcept;

protected:
	double tau;
	bool reversibility_check;
	int integrationsteps0;
	int integrationsteps1;
	int integrationsteps2;
	int hmcsteps;
	int num_timescales;
	integrator integrator0;
	integrator integrator1;
	integrator integrator2;
	double lambda0;
	double lambda1;
	double lambda2;
	bool use_gauge_only;
	bool use_mp;

	po::options_description getOptions();
};

}

#endif
