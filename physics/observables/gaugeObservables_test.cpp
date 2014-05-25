/** @file
 * Unit test for the physics::gaugeObservables class
 *
 * Copyright 2014,Christopher Pinke
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

#include "gaugeObservables.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE physics::gaugeObservables
#include <boost/test/unit_test.hpp>

#include "../../host_functionality/logger.hpp"
#include "../../meta/type_ops.hpp"
#include <stdexcept>

#include "../lattices/gaugefield.hpp"

BOOST_AUTO_TEST_SUITE( BUILD )

BOOST_AUTO_TEST_CASE( BUILD_1 )
{
  const char * _params[] = {"foo"};
  meta::Inputparameters params(1, _params);

  BOOST_REQUIRE_NO_THROW(physics::observables::gaugeObservables tester(&params) );
}

BOOST_AUTO_TEST_SUITE_END()

class GaugeObservablesTester
{
public:
  GaugeObservablesTester(int argc, const char** argv)
  {
    parameters = new meta::Inputparameters(argc, argv);
    gaugeObservables = new physics::observables::gaugeObservables(parameters);
    system = new hardware::System(*parameters);
    prng = new physics::PRNG(*system);
    gaugefield = new physics::lattices::Gaugefield(*system, *prng);
  }
  ~GaugeObservablesTester()
  {
    gaugeObservables= 0;
    system = 0;
    prng = 0;
    gaugefield = 0;
    parameters = 0;
  }

  meta::Inputparameters * parameters;
  physics::observables::gaugeObservables * gaugeObservables;
  physics::lattices::Gaugefield * gaugefield;
private:
  hardware::System *  system;
  physics::PRNG * prng;
};

BOOST_AUTO_TEST_SUITE( PLAQUETTE  )

class PlaquetteTester : public GaugeObservablesTester
{
public:
  PlaquetteTester(int argc, const char** argv, double referenceValue):
    GaugeObservablesTester(argc, argv)
  {
    double testPrecision = 1e-8;
    GaugeObservablesTester::gaugeObservables->measurePlaquette(GaugeObservablesTester::gaugefield);
    BOOST_CHECK_CLOSE(GaugeObservablesTester::gaugeObservables->getPlaquette(), referenceValue, testPrecision);
  }
};

BOOST_AUTO_TEST_CASE( PLAQUETTE_1 )
{
  double referenceValue = 1.;
  const char * _params[] = {"foo", "--startcondition=cold"};
  PlaquetteTester(2, _params, referenceValue);
}

BOOST_AUTO_TEST_CASE( PLAQUETTE_2 )
{
  double referenceValue = 0.57107711169452713;
  const char * _params[] = {"foo", "--startcondition=continue", "--sourcefile=conf.00200", "--nt=4"};
  PlaquetteTester(4, _params, referenceValue);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( RECTANGLES  )

class RectanglesTester : public GaugeObservablesTester
{
public:
  RectanglesTester(int argc, const char** argv, double referenceValue):
    GaugeObservablesTester(argc, argv)
  {
    double testPrecision = 1e-8;
    GaugeObservablesTester::gaugeObservables->measureRectangles(GaugeObservablesTester::gaugefield);
    BOOST_CHECK_CLOSE(GaugeObservablesTester::gaugeObservables->getRectangles(), referenceValue, testPrecision);
  }
};

BOOST_AUTO_TEST_CASE( RECTANGLES_1 )
{
  double referenceValue = 1.;
  const char * _paramsWithWrongGaugeAction[] = {"foo", "--startcondition=cold", "--gaugeact=wilson"};
  BOOST_REQUIRE_THROW(RectanglesTester(3, _paramsWithWrongGaugeAction, referenceValue), std::logic_error);
}

BOOST_AUTO_TEST_CASE( RECTANGLES_2 )
{
  double referenceValue = 6144.;
  const char * _params[] = {"foo", "--startcondition=cold", "--gaugeact=tlsym"};
  RectanglesTester(3, _params, referenceValue);
}

BOOST_AUTO_TEST_CASE( RECTANGLES_3 )
{
  double referenceValue = 1103.2398401620451;
  const char * _params[] = {"foo", "--startcondition=continue", "--sourcefile=conf.00200", "--nt=4", "--gaugeact=tlsym"};
  RectanglesTester(5, _params, referenceValue);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( POLYAKOVLOOP  )

class PolyakovloopTester : public GaugeObservablesTester
{
public:
  PolyakovloopTester(int argc, const char** argv, hmc_complex referenceValue):
    GaugeObservablesTester(argc, argv)
  {
    double testPrecision = 1e-8;
    GaugeObservablesTester::gaugeObservables->measurePolyakovloop(GaugeObservablesTester::gaugefield);
    hmc_complex poly = GaugeObservablesTester::gaugeObservables->getPolyakovloop();
    BOOST_CHECK_CLOSE(poly.re, referenceValue.re, testPrecision);
    BOOST_CHECK_CLOSE(poly.im, referenceValue.im, testPrecision);
  }
};

BOOST_AUTO_TEST_CASE( POLYAKOVLOOP_1 )
{
  hmc_complex referenceValue = {1., 0.};
  const char * _params[] = {"foo", "--startcondition=cold"};
  PolyakovloopTester(2, _params, referenceValue);
}

BOOST_AUTO_TEST_CASE( POLYAKOVLOOP_2 )
{
  hmc_complex referenceValue = {-0.11349672123636857,0.22828243566855227};
  const char * _params[] = {"foo", "--startcondition=continue", "--sourcefile=conf.00200", "--nt=4"};
  PolyakovloopTester(4, _params, referenceValue);
}

BOOST_AUTO_TEST_SUITE_END()
