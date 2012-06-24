////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is part of iris, a lightweight C++ camera calibration library    //
//                                                                            //
// Copyright (C) 2012 Alexandru Duliu                                         //
//                                                                            //
// iris is free software; you can redistribute it and/or                      //
// modify it under the terms of the GNU Lesser General Public                 //
// License as published by the Free Software Foundation; either               //
// version 3 of the License, or (at your option) any later version.           //
//                                                                            //
// iris is distributed in the hope that it will be useful, but WITHOUT ANY    //
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS  //
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the //
// GNU General Public License for more details.                               //
//                                                                            //
// You should have received a copy of the GNU Lesser General Public           //
// License along with iris. If not, see <http://www.gnu.org/licenses/>.       //
//                                                                            //
///////////////////////////////////////////////////////////////////////////////

/*
 * CameraCalibration.cpp
 *
 *  Created on: Jun 5, 2011
 *      Author: duliu
 */


#include <iris/CameraCalibration.hpp>

namespace iris {

CameraCalibration::CameraCalibration() :
    m_finder(0),
    m_poseCount(0)
{
}


CameraCalibration::~CameraCalibration() {
	// TODO Auto-generated destructor stub
}


size_t CameraCalibration::addImage( std::shared_ptr<cimg_library::CImg<uint8_t> > image, const size_t cameraID )
{
    // check that all is well
    check();

    // assemble the pose
    Pose_d pose;
    pose.id = m_poseCount;
    pose.image = image;

    // add the pose
    m_cameras[cameraID].poses.push_back( pose );

    // get the image size
    Eigen::Vector2i imageSize( image->width(), image->height() );

    // make sure the image sizes are the same
    if( m_cameras[cameraID].poses.size() == 1 )
        m_cameras[cameraID].imageSize = imageSize;
    else
    {
        Eigen::Vector2i tmp = imageSize - m_cameras[cameraID].imageSize;
        if( (tmp(0) != 0) || (tmp(1) != 0) )
            throw std::runtime_error( "CameraCalibration::addImage: pose has different image size than already stored poses." );
    }

    // increment pose count and return id
    m_poseCount++;
    return pose.id;
}


void CameraCalibration::clear()
{
    m_poseCount = 0;
    m_cameras.clear();
}



void CameraCalibration::setFinder( std::shared_ptr<Finder> finder )
{
    m_finder = finder;
}


const Finder& CameraCalibration::finder() const
{
    return *m_finder;
}


const Camera_d& CameraCalibration::camera( const size_t id ) const
{
    // find the camera
    std::map< size_t, Camera_d >::const_iterator camIt = m_cameras.find( id );

    // search for the camera
    if( camIt != m_cameras.end() )
        return (*camIt).second;
    else
        throw std::runtime_error("CameraCalibration::camera: camera not found.");
}


const Pose_d& CameraCalibration::pose( const size_t id ) const
{
    for( auto camIt=m_cameras.begin(); camIt != m_cameras.end(); camIt++ )
        for( size_t p=0; p<camIt->second.poses.size(); p++ )
            if( camIt->second.poses[p].id == id )
                return camIt->second.poses[p];

    // nothing found
    throw std::runtime_error("IrisCC::pose: pose not found.");
}


void CameraCalibration::check()
{
    if( m_finder == 0 )
        throw std::runtime_error("CameraCalibration: Finder not set.");
}


} // end namespace iris

