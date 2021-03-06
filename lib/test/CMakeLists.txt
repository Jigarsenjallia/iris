##############################################################################
#                                                                            #
# This file is part of iris, a lightweight C++ camera calibration library    #
#                                                                            #
# Copyright (C) 2012 Alexandru Duliu                                         #
#                                                                            #
# iris is free software; you can redistribute it and/or                      #
# modify it under the terms of the GNU Lesser General Public                 #
# License as published by the Free Software Foundation; either               #
# version 3 of the License, or (at your option) any later version.           #
#                                                                            #
# iris is distributed in the hope that it will be useful, but WITHOUT ANY    #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS  #
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the #
# GNU General Public License for more details.                               #
#                                                                            #
# You should have received a copy of the GNU Lesser General Public           #
# License along with iris. If not, see <http://www.gnu.org/licenses/>.       #
#                                                                            #
##############################################################################

# set include directories
include_directories( ${Iris_INCLUDE_DIRS} )

# add test for util
set( Iris_Test_Util test_util )
add_executable( ${Iris_Test_Util} TestUtil.cpp )
target_link_libraries( ${Iris_Test_Util} -lm -lc -Wall ${Iris_LIBRARIES} )
add_test( TestUtil ${Iris_Test_Util} )


# add test for ocv single
set( Iris_Test_OpenCVSingleCalibration test_opencv_single )
add_executable( ${Iris_Test_OpenCVSingleCalibration} TestOpenCVSingleCalibration.cpp )
target_link_libraries( ${Iris_Test_OpenCVSingleCalibration} -lm -lc -Wall ${Iris_LIBRARIES} )
add_test( OpenCVSingleCalibration ${Iris_Test_OpenCVSingleCalibration} )


# add test for rdf
set( Iris_Test_RandomFeatureDescriptor test_rfd )
add_executable( ${Iris_Test_RandomFeatureDescriptor} TestRandomFeatureDescriptor.cpp )
target_link_libraries( ${Iris_Test_RandomFeatureDescriptor} -lm -lc -Wall ${Iris_LIBRARIES} )
add_test( TestRandomFeatureDescriptor ${Iris_Test_RandomFeatureDescriptor} )


# add test for rff
set( Iris_Test_RandomFeatureFinder test_rff )
add_executable( ${Iris_Test_RandomFeatureFinder} TestRandomFeatureFinder.cpp )
target_link_libraries( ${Iris_Test_RandomFeatureFinder} -lm -lc -Wall ${Iris_LIBRARIES} )
add_test( TestRandomFeatureFinder ${Iris_Test_RandomFeatureFinder} )


