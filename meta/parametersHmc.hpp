/** @file
 *
 * Copyright (c) 2014 Christopher Pinke
 * Copyright (c) 2014 Matthias Bach
 * Copyright (c) 2015,2018 Francesca Cuteri
 * Copyright (c) 2018 Alessandro Sciarra
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CL2QCD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _META_PARAMETERS_HMC_HPP_
#define _META_PARAMETERS_HMC_HPP_

#include "parametersBasic.hpp"

namespace meta {
    class ParametersHmc {
      public:
        int get_hmcsteps() const noexcept;
        bool get_use_gauge_only() const noexcept;
        bool get_use_mp() const noexcept;

      private:
        int hmcsteps;
        bool use_gauge_only;
        bool use_mp;

      protected:
        ParametersHmc();
        virtual ~ParametersHmc()            = default;
        ParametersHmc(ParametersHmc const&) = delete;
        ParametersHmc& operator=(ParametersHmc const&) = delete;

        InputparametersOptions options;
    };

}  // namespace meta

#endif
