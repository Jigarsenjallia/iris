////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is part of iris, a lightweight C++ camera calibration library    //
//                                                                            //
// Copyright (C) 2010, 2011 Alexandru Duliu                                   //
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
 * OpenCVStereoCalibration.cpp
 *
 *  Created on: Jun 12, 2012
 *      Author: duliu
 */


#include <iris/util.hpp>
#include <iris/OpenCVStereoCalibration.hpp>


namespace iris {

OpenCVStereoCalibration::OpenCVStereoCalibration() :
    OpenCVCalibration(),
    m_relativeToPattern(true),
    m_sameFocalLength(false)
{
}


OpenCVStereoCalibration::~OpenCVStereoCalibration() {
	// TODO Auto-generated destructor stub
}


void OpenCVStereoCalibration::setRelativeToPattern( bool val )
{
    m_relativeToPattern = val;
}


void OpenCVStereoCalibration::setSameFocalLength( bool val )
{
    m_sameFocalLength = val;
}


void OpenCVStereoCalibration::setFixIntrinsic( bool val )
{
    m_fixIntrinsic = val;
}


void OpenCVStereoCalibration::calibrate( CameraSet_d& cs )
{
    // assuming there are only two cameras
    if( cs.cameras().size() != 2 )
        throw std::runtime_error("OpenCVStereoCalibration::calibrate: exactly 2 cameras are required.");

    // get refs of camera
    iris::Camera_d& cam1 = cs.cameras().begin()->second;
    iris::Camera_d& cam2 = (++(cs.cameras().begin()))->second;

    // detect correspondences over all poses
    if( m_finder->useOpenMP() )
    {
        #pragma omp parallel for
        for( int p=0; p<cam1.poses.size(); p++ )
        {
            m_finder->find( cam1.poses[p] );
            m_finder->find( cam2.poses[p] );
        }
    }
    else
        for( int p=0; p<cam1.poses.size(); p++ )
        {
            m_finder->find( cam1.poses[p] );
            m_finder->find( cam2.poses[p] );
        }

    // filter the poses
    filter( cs );

    // calibrate the cameras
    stereoCalibrate( m_filteredCameras.begin()->second,
                 (++(m_filteredCameras.begin()))->second );

    // commit the results
    commit( cs );
}


void OpenCVStereoCalibration::stereoCalibrate( iris::Camera_d& cam1, iris::Camera_d& cam2 )
{
    // init stuff
    std::vector< std::vector<cv::Point2f> > cvVectorPoints2D_1;
    std::vector< std::vector<cv::Point2f> > cvVectorPoints2D_2;
    std::vector< std::vector<cv::Point3f> > cvVectorPoints3D;
    size_t frameCount = cam1.poses.size();

    // init camera params
    cv::Mat A_1 = cv::Mat::eye(3,3,CV_64F); // intrinsic matrix 1
    cv::Mat A_2 = cv::Mat::eye(3,3,CV_64F); // intrinsic matrix 2
    cv::Mat E = cv::Mat::eye(3,3,CV_64F); // essential matrix
    cv::Mat F = cv::Mat::eye(3,3,CV_64F); // fundamental matrix
    cv::Mat dc_1(5,1,CV_64F); // distortion coefficients
    cv::Mat dc_2(5,1,CV_64F); // distortion coefficients
    cv::Mat R; // rotation
    cv::Mat T; // translation

    // run over all the poses of this camera and assemble the correspondences
    for( size_t f=0; f<frameCount; f++ )
    {
        // add them to the opencv vectors
        cvVectorPoints2D_1.push_back( iris::eigen2cv<float>( cam1.poses[f].points2D ) );
        cvVectorPoints2D_2.push_back( iris::eigen2cv<float>( cam2.poses[f].points2D ) );
        cvVectorPoints3D.push_back( iris::eigen2cv<float>( cam1.poses[f].points3D ) );
    }

    // try to compute the intrinsic and extrinsic parameters
    eigen2cv( cam1.intrinsic, A_1 );
    eigen2cv( cam2.intrinsic, A_2 );
    double error = cv::stereoCalibrate( cvVectorPoints3D,
                                        cvVectorPoints2D_1,
                                        cvVectorPoints2D_2,
                                        A_1, dc_1,
                                        A_2, dc_2,
                                        cv::Size( cam1.imageSize(0), cam1.imageSize(1) ),
                                        R, T, E, F,
                                        cv::TermCriteria( cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 1e-6),
                                        flags() );

    // instrinsic matrix
    cv::cv2eigen( A_1, cam1.intrinsic );
    cv::cv2eigen( A_2, cam2.intrinsic );

    // distortion coefficeints
    cam1.distortion.clear();
    cam2.distortion.clear();
    for( int i=0; i<dc_1.rows; i++ )
    {
        cam1.distortion.push_back( dc_1.at<double>( i, 0 ) );
        cam2.distortion.push_back( dc_2.at<double>( i, 0 ) );
    }

    // error
    cam1.error = error;
    cam2.error = error;

    // convert and save the poses
    Eigen::Matrix4d RT;
    iris::cv2eigen( R, T, RT );
    #pragma omp parallel for
    for( int i=0; i<frameCount; i++ )
    {
        // compute the pose
        cv::Mat rVec_cam1, tVec_cam1, rVec_cam2, tVec_cam2;
        cv::solvePnP( cvVectorPoints3D[i], cvVectorPoints2D_1[i], A_1, dc_1, rVec_cam1, tVec_cam1, false );
        cv::solvePnP( cvVectorPoints3D[i], cvVectorPoints2D_2[i], A_2, dc_2, rVec_cam2, tVec_cam2, false );

        // convert to eigen and store
        Eigen::Matrix4d trans_cam1, trans_cam2;
        iris::cv2eigen( rVec_cam1, tVec_cam1, trans_cam1 );
        iris::cv2eigen( rVec_cam2, tVec_cam2, trans_cam2 );
        cam1.poses[i].transformation = trans_cam1;
        cam2.poses[i].transformation = trans_cam2;

        // now the ugly part, convert back to openCV for back projection
        cv::Mat rv1, rv2, tv1, tv2;
        iris::eigen2cv( cam1.poses[i].transformation, rv1, tv1 );
        iris::eigen2cv( cam2.poses[i].transformation, rv2, tv2 );

        // reproject points from the opencv poses
        cam1.poses[i].projected2D = projectPoints( cvVectorPoints3D[i], rv1, tv1, A_1, dc_1 );
        cam2.poses[i].projected2D = projectPoints( cvVectorPoints3D[i], rv2, tv2, A_2, dc_2 );
    }

    // if only the relative pose is desired, just blank it all
    for( size_t i=0; !m_relativeToPattern && i<frameCount; i++ )
    {
        // get the extrinsics
        cam1.poses[i].transformation = Eigen::Matrix4d::Identity();
        cam2.poses[i].transformation = RT;
    }
}


void OpenCVStereoCalibration::filter( CameraSet_d& cs )
{
    // init stuff
    m_filteredCameras.clear();
    size_t framesAdded = 0;

    // get refs of camera
    iris::Camera_d& cam1 = cs.cameras().begin()->second;
    iris::Camera_d& cam2 = (++(cs.cameras().begin()))->second;

    // check that the cameras have the same image size
    if( !(m_intrinsicGuess || m_fixIntrinsic) && ( (cam1.imageSize(0) != cam2.imageSize(0)) || (cam1.imageSize(1) != cam2.imageSize(1)) ) )
        throw std::runtime_error("OpenCVStereoCalibration::filter: cameras do not have the same images size.");

    // do the actual filtering
    for( size_t p=0; p<cam1.poses.size(); p++ )
    {
        // guilty untill proven innocent
        cam1.poses[p].rejected = true;
        cam2.poses[p].rejected = true;

        // check if this frame is valid
        if( checkFrame( cam1.poses[p], cam2.poses[p] ) )
        {
            m_filteredCameras[cam1.id].poses.push_back( cam1.poses[p] );
            m_filteredCameras[cam2.id].poses.push_back( cam2.poses[p] );
            framesAdded++;
        }
        else
            std::cout << "OpenCVStereoCalibration::filter: frame rejected." << std::endl;
    }

    // set image size
    if( framesAdded > 0 )
    {
        m_filteredCameras[cam1.id].imageSize = cam1.imageSize;
        m_filteredCameras[cam2.id].imageSize = cam2.imageSize;
        m_filteredCameras[cam1.id].id = cam1.id;
        m_filteredCameras[cam2.id].id = cam2.id;
        m_filteredCameras[cam1.id].intrinsic = cam1.intrinsic;
        m_filteredCameras[cam2.id].intrinsic = cam2.intrinsic;
    }
}


bool OpenCVStereoCalibration::checkFrame( const iris::Pose_d& pose1, const iris::Pose_d& pose2 )
{
    // check if the arrays have the same length
    if( pose1.points2D.size() == 0 ||
        pose1.pointIndices.size() == 0 ||
        (pose1.points2D.size() != pose2.points2D.size() ) ||
        (pose1.pointIndices.size() != pose2.pointIndices.size() ) )
        return false;

    // check that all the indices match
    for( size_t i=0; i<pose1.pointIndices.size(); i++ )
        if( pose1.pointIndices[i] != pose2.pointIndices[i] )
            return false;

    return true;
}


int OpenCVStereoCalibration::flags()
{
    int result = 0;

    if( m_fixPrincipalPoint )
        result = result | CV_CALIB_FIX_PRINCIPAL_POINT;

    if( m_fixAspectRatio )
        result = result | CV_CALIB_FIX_ASPECT_RATIO;

    if( m_sameFocalLength )
        result = result | CV_CALIB_SAME_FOCAL_LENGTH;

    if( !m_tangentialDistortion )
        result = result | CV_CALIB_ZERO_TANGENT_DIST;

    if( m_fixIntrinsic )
        result = result | CV_CALIB_FIX_INTRINSIC;

    if( m_intrinsicGuess )
        result = result | CV_CALIB_USE_INTRINSIC_GUESS;

    return result;
}


} // end namespace iris

