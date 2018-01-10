#include "src_task2/cable_detector.h"
#include <visualization_msgs/Marker.h>
#include "tough_common/val_common_names.h"
#include <pcl/common/centroid.h>
#include <thread>

#define DISABLE_DRAWINGS true
#define DISABLE_TRACKBAR true

CableDetector::CableDetector(ros::NodeHandle nh, tough_perception::MultisenseImage *ms_sensor) : nh_(nh), organizedCloud_(new tough_perception::StereoPointCloudColor)
{
    robot_state_ = RobotStateInformer::getRobotStateInformer(nh_);
    rd_ = RobotDescription::getRobotDescription(nh_);
    ms_sensor_ = ms_sensor;
    ms_sensor_->giveQMatrix(qMatrix_);
    marker_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("/visualization_marker_array",1);
    eigenVecs_.reserve(2);
    quat_.resize(12);
}

void CableDetector::showImage(cv::Mat image, std::string caption)
{
#ifdef DISABLE_DRAWINGS
    return;
#endif
    cv::namedWindow( caption, cv::WINDOW_AUTOSIZE );
    cv::imshow( caption, image);
    cv::waitKey(3);
}

void CableDetector::colorSegment(cv::Mat &imgHSV, cv::Mat &outImg)
{
    cv::inRange(imgHSV,cv::Scalar(hsv_[0], hsv_[2], hsv_[4]), cv::Scalar(hsv_[1], hsv_[3], hsv_[5]), outImg);
#ifdef DISABLE_TRACKBAR
    return;
#endif
    setTrackbar();

    cv::imshow("binary thresholding", outImg);
    cv::imshow("HSV thresholding", imgHSV);
    cv::waitKey(3);
}

size_t CableDetector::findMaxContour(const std::vector<std::vector<cv::Point> >& contours)
{
    int largest_area = 0;
    int largest_contour_index = 0;
    std::vector<std::vector<cv::Point>> hull(contours.size());

    for( size_t i = 0; i< contours.size(); i++ )
    {
        cv::convexHull(contours[i], hull[i], false);
        //  Find the area of contour
        double area = cv::contourArea( hull[i]);

        if(area > largest_area)
        {
            largest_area = area;
            // Store the index of largest contour
            largest_contour_index = i;
        }
    }
    ROS_INFO_STREAM(" Largest Area "<< largest_area << std::endl);
    convexHulls_ = hull[largest_contour_index];

    return largest_contour_index;
}

bool CableDetector::getCableLocation(geometry_msgs::Point& cableLoc)
{
    bool foundCable = false;
    tough_perception::StereoPointCloudColor::Ptr organizedCloud(new tough_perception::StereoPointCloudColor);
    tough_perception::PointCloudHelper::generateOrganizedRGBDCloud(current_disparity_, current_image_, qMatrix_, organizedCloud);
    tf::TransformListener listener;
    geometry_msgs::PointStamped geom_point;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Mat imgHSV, outImg;

    cv::cvtColor(current_image_, imgHSV, cv::COLOR_BGR2HSV);
    colorSegment(imgHSV, outImg);

    // Find contours
    cv::findContours(outImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    Eigen::Vector4f cloudCentroid;
    if(!contours.empty()) //avoid seg fault at runtime by checking that the contours exist
    {
        findMaxContour(contours);
        foundCable = true;
        cv::Mat hullPoints = cv::Mat::zeros(current_image_.size(), CV_8UC1);
        cv::fillConvexPoly(hullPoints, convexHulls_,cv::Scalar(255));
        cv::Mat nonZeroCoordinates;
        cv::erode(hullPoints, hullPoints, cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(5,5)));
        cv::findNonZero(hullPoints, nonZeroCoordinates);
        if (nonZeroCoordinates.total() < 10)
            return false;

        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud;

        for (int k = 0; k < nonZeroCoordinates.total(); k++ ) {
            pcl::PointXYZRGB temp_pclPoint = organizedCloud->at(nonZeroCoordinates.at<cv::Point>(k).x, nonZeroCoordinates.at<cv::Point>(k).y);
            if (temp_pclPoint.z > -2.0 && temp_pclPoint.z < 2.0 )
            {
                            //ROS_INFO_STREAM("r : " << *(temp_pclPoint.data + 4) << std::endl);
                currentDetectionCloud.push_back(pcl::PointXYZRGB(temp_pclPoint));
            }
        }
        //  Calculating the Centroid of the handle Point cloud

        if (!pcl::compute3DCentroid(currentDetectionCloud, cloudCentroid)){
            ROS_INFO("CableDetector::getCableLocation : the centroid could not be computed");
            return false;
        }
    }

    if( foundCable )
    {
        //pcl::PointXYZRGB temp_pclPoint2 = organizedCloud->at(points[1].x, points[1].y);
        geom_point.point.x = cloudCentroid(0);
        geom_point.point.y = cloudCentroid(1);
        geom_point.point.z = cloudCentroid(2);
        geom_point.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;

        try
        {
            listener.waitForTransform(VAL_COMMON_NAMES::WORLD_TF, VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF, ros::Time(0), ros::Duration(3.0));
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point, geom_point);
        }
        catch (tf::TransformException ex){
            ROS_ERROR("%s",ex.what());
            return false;
        }
        cableLoc.x = double (geom_point.point.x);
        cableLoc.y = double (geom_point.point.y);
        cableLoc.z = double (geom_point.point.z);
        visualize_point(geom_point.point, 0.1, 0.5, 0.0);

        //visualize_direction(new_point, geom_point.point);
        marker_pub_.publish(markers_);
    }
    return foundCable;
}

bool CableDetector::getCablePose(geometry_msgs::Pose& cablePose)
{
    bool foundCable = false;
    tough_perception::StereoPointCloudColor::Ptr organizedCloud(new tough_perception::StereoPointCloudColor);
    tough_perception::PointCloudHelper::generateOrganizedRGBDCloud(current_disparity_, current_image_, qMatrix_, organizedCloud);
    tf::TransformListener listener;
    geometry_msgs::PointStamped geom_point0;
    geometry_msgs::PointStamped geom_point;
    geometry_msgs::PointStamped geom_point1;
    geometry_msgs::PointStamped geom_point2;
    geometry_msgs::PointStamped geom_point3;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Mat imgHSV, outImg;
    std::vector<cv::Point> points;

    cv::cvtColor(current_image_, imgHSV, cv::COLOR_BGR2HSV);
    colorSegment(imgHSV, outImg);

    // Find contours
    cv::findContours(outImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    Eigen::Vector4f cloudCentroid;
    Eigen::Vector4f cloudCentroid0;
    Eigen::Vector4f cloudCentroid1;
    Eigen::Vector4f cloudCentroid2;
    Eigen::Vector4f cloudCentroid3;
    if(!contours.empty()) //avoid seg fault at runtime by checking that the contours exist
    {
        foundCable = true;
        points = getCentroid(contours[findMaxContour(contours)], outImg);
        //ROS_INFO_STREAM(point << std::endl);
        double theta = (1/3600.0) * M_PI;
        double r = std::sqrt(std::pow(points[1].x - points[0].x , 2) + std::pow(points[1].y - points[0].y , 2)) + 1.0;
        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud;
        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud0;
        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud1;
        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud2;
        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud3;

        if (points[0].x * points[0].y > organizedCloud->size() || points[1].x * points[1].y > organizedCloud->size() || points[2].x * points[2].y > organizedCloud->size() || points[3].x * points[3].y > organizedCloud->size() )
        {
            ROS_INFO("CableDetector::getCableLocation : One of the detected pixel points is out of organized cloud range");
            return false;
        }
        for (size_t k = 0; k < 3600; k++ )
        {
            pcl::PointXYZRGB temp_pclPoint0;
            temp_pclPoint0 = organizedCloud->at(int(points[1].x + r * std::cos(k * theta)), int(points[1].y + r * std::sin(k * theta)));

            //ROS_INFO_STREAM("blue: "<< *(temp_pclPoint0.data + 5) << std::endl);
            if (temp_pclPoint0.z > -2.0 && temp_pclPoint0.z < 2.0 && (*(temp_pclPoint0.data + 5)) < 0.7)
            {
                currentDetectionCloud0.push_back(pcl::PointXYZRGB(temp_pclPoint0));
            }
        }

        for (size_t k = 0; k < 10; k++ )
        {
            for (size_t l = 0; l < 10; l++ )
            {
                pcl::PointXYZRGB temp_pclPoint;
                pcl::PointXYZRGB temp_pclPoint1;
                pcl::PointXYZRGB temp_pclPoint2;
                pcl::PointXYZRGB temp_pclPoint3;

                temp_pclPoint = organizedCloud->at(points[0].x + k, points[0].y + l );
                temp_pclPoint1 = organizedCloud->at(points[1].x + k, points[1].y + l);
                temp_pclPoint2 = organizedCloud->at(points[2].x + k, points[2].y + l);
                temp_pclPoint3 = organizedCloud->at(points[3].x + k, points[3].y + l);

                if (temp_pclPoint.z > -2.0 && temp_pclPoint.z < 2.0 )
                {
                    currentDetectionCloud.push_back(pcl::PointXYZRGB(temp_pclPoint));
                }

                if (temp_pclPoint1.z > -2.0 && temp_pclPoint1.z < 2.0 )
                {
                    currentDetectionCloud1.push_back(pcl::PointXYZRGB(temp_pclPoint1));
                }

                if (temp_pclPoint2.z > -2.0 && temp_pclPoint2.z < 2.0 )
                {
                    currentDetectionCloud2.push_back(pcl::PointXYZRGB(temp_pclPoint2));
                }
                if (temp_pclPoint3.z > -2.0 && temp_pclPoint3.z < 2.0 )
                {
                    currentDetectionCloud3.push_back(pcl::PointXYZRGB(temp_pclPoint3));
                }
            }
        }
        //  Calculating the Centroid of the handle Point cloud

        if (!pcl::compute3DCentroid(currentDetectionCloud, cloudCentroid) || !pcl::compute3DCentroid(currentDetectionCloud0, cloudCentroid0)
                || !pcl::compute3DCentroid(currentDetectionCloud1, cloudCentroid1) || !pcl::compute3DCentroid(currentDetectionCloud2, cloudCentroid2)
                || !pcl::compute3DCentroid(currentDetectionCloud3, cloudCentroid3)){
            ROS_INFO("CableDetector::getCableLocation : one of the centroids could not be computed");
            return false;
        }
    }

    if( foundCable )
    {
        //pcl::PointXYZRGB temp_pclPoint2 = organizedCloud->at(points[1].x, points[1].y);
        geom_point.point.x = cloudCentroid(0);
        geom_point.point.y = cloudCentroid(1);
        geom_point.point.z = cloudCentroid(2);
        geom_point.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;

        geom_point0.point.x = cloudCentroid0(0);
        geom_point0.point.y = cloudCentroid0(1);
        geom_point0.point.z = cloudCentroid0(2);
        geom_point0.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;

        geom_point1.point.x = cloudCentroid1(0);
        geom_point1.point.y = cloudCentroid1(1);
        geom_point1.point.z = cloudCentroid1(2);
        geom_point1.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;

        geom_point2.point.x = cloudCentroid2(0);
        geom_point2.point.y = cloudCentroid2(1);
        geom_point2.point.z = cloudCentroid2(2);
        geom_point2.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;

        geom_point3.point.x = cloudCentroid3(0);
        geom_point3.point.y = cloudCentroid3(1);
        geom_point3.point.z = cloudCentroid3(2);
        geom_point3.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;
        try
        {
            listener.waitForTransform(VAL_COMMON_NAMES::WORLD_TF, VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF, ros::Time(0), ros::Duration(3.0));
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point, geom_point);
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point0, geom_point0_);
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point1, geom_point1);
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point2, geom_point2);
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point3, geom_point3);
        }
        catch (tf::TransformException ex){
            ROS_ERROR("%s",ex.what());
            return false;
        }

        geometry_msgs::Point new_point;
        geometry_msgs::Point dir_vec;
        geometry_msgs::Point dir_vector;
        dir_vec.x = geom_point2.point.x - geom_point1.point.x;
        dir_vec.y = geom_point2.point.y - geom_point1.point.y;

        new_point.x = geom_point3.point.x + 0.2;
        double C = (new_point.x - geom_point3.point.x)*(dir_vec.x);
        new_point.y = geom_point3.point.y - C/double(dir_vec.y);
        new_point.z = geom_point3.point.z;

        robot_state_->transformPoint(geom_point3, geom_point3, rd_->getPelvisFrame());
        robot_state_->transformPoint(new_point, new_point, VAL_COMMON_NAMES::WORLD_TF, rd_->getPelvisFrame());
        double geom_norm = std::pow(geom_point3.point.x, 2) + std::pow(geom_point3.point.y, 2);
        double dir_norm = std::pow(new_point.x, 2) + std::pow(new_point.y, 2);
        robot_state_->transformPoint(geom_point3, geom_point3, VAL_COMMON_NAMES::WORLD_TF);
        robot_state_->transformPoint(new_point, new_point, rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);

        if(dir_norm <= geom_norm)
        {
            new_point.x = geom_point3.point.x - 0.2;
            C = (new_point.x - geom_point3.point.x)*(dir_vec.x);
            new_point.y = geom_point3.point.y - C/double(dir_vec.y);
        }

        dir_vector.x = new_point.x - geom_point3.point.x;
        dir_vector.y = new_point.y - geom_point3.point.y;
        dir_vector.z = 0.0;

        float cos_theta = dir_vector.x / std::sqrt(std::pow(dir_vector.x , 2) + std::pow(dir_vector.y , 2) );
        float sin_theta = dir_vector.y / std::sqrt(std::pow(dir_vector.x ,2) + std::pow(dir_vector.y ,2) );
        float theta = std::atan2(sin_theta, cos_theta);

        for (int i = 1 ;i <= 12 ; i++)
        {
            float theta1 = theta + i * 30 * (M_PI/180.0);
            geometry_msgs::Quaternion quaternion1 = tf::createQuaternionMsgFromYaw(theta1);
            ROS_INFO_STREAM("Theta " << theta1 << std::endl << "Quaternion " << quaternion1);
            quat_[i-1] = quaternion1;
        }

        ROS_INFO("Yaw angle : %f", theta);
        pose_.position.x = geom_point3.point.x;
        pose_.position.y = geom_point3.point.y;
        pose_.position.z = geom_point3.point.z;

        geometry_msgs::Quaternion quaternion = tf::createQuaternionMsgFromYaw(theta);
        pose_.orientation = quaternion;
        visualize_pose(pose_);
        cablePose = pose_;

        visualize_point(geom_point.point, 0.7, 0.5, 0.0);
        visualize_point(geom_point1.point, 1.0, 0.0, 1.0);
        visualize_point(geom_point2.point, 0.0, 0.0, 1.0);
        visualize_point(geom_point3.point, 0.0, 1.0, 1.0);
        visualize_point(geom_point0_.point, 1.0, 0.8, 0.3);
        //visualize_direction(new_point, geom_point.point);
        marker_pub_.publish(markers_);
    }
    return foundCable;
}

std::vector<cv::Point> CableDetector::getCentroid(const std::vector<cv::Point> &contourPts, cv::Mat &img)
{
    int sz = static_cast<int>(contourPts.size());
    cv::Mat data_pts = cv::Mat(sz, 2, CV_64FC1);

    for (int i = 0; i < data_pts.rows; ++i)
    {
        data_pts.at<double>(i, 0) = contourPts[i].x;
        data_pts.at<double>(i, 1) = contourPts[i].y;
    }

    //Perform PCA analysis
    cv::PCA pca_analysis(data_pts, cv::Mat(), CV_PCA_DATA_AS_ROW);

    //Store the center of the object
    cv::Point cntr = cv::Point(static_cast<int>(pca_analysis.mean.at<double>(0, 0)),
                               static_cast<int>(pca_analysis.mean.at<double>(0, 1)));

    //Store the eigenvalues and eigenvectors
    //std::vector<cv::Point2d> eigenVecs_(2);
    std::vector<double> eigen_val(2);

    for (int i = 0; i < 2; ++i)
    {
        eigenVecs_[i] = cv::Point2d(pca_analysis.eigenvectors.at<double>(i, 0),
                                    pca_analysis.eigenvectors.at<double>(i, 1));
        eigen_val[i] = pca_analysis.eigenvalues.at<double>(0, i);
    }
    // Draw the principal components
    cv::circle(current_image_, cntr, 3, cv::Scalar(255, 0, 255), 2);
    cv::Point p1 = cntr + 0.03 * cv::Point(static_cast<int>(eigenVecs_[0].x * eigen_val[0]), static_cast<int>(eigenVecs_[0].y * eigen_val[0]));
    cv::Point p2 = cntr - 0.03 * cv::Point(static_cast<int>(eigenVecs_[0].x * eigen_val[0]), static_cast<int>(eigenVecs_[0].y * eigen_val[0]));
    cv::Point p3 = (p2 - cntr);//0.03 * cv::Point(static_cast<int>(eigenVecs_[1].x * eigen_val[1]), static_cast<int>(eigenVecs_[1].y * eigen_val[1]));
    p3.x = cntr.x + p3.x * OFF_SET/10.0;
    p3.y = cntr.y + p3.y * OFF_SET/10.0;

    //VISUALIZATION - Uncomment this line
    drawAxis(current_image_, cntr, p1, cv::Scalar(0, 255, 0), 1);
    drawAxis(current_image_, cntr, p2, cv::Scalar(255, 255, 0), 1);
    //ROS_INFO_STREAM("p2 "<< p2 << std::endl);
    //ROS_INFO_STREAM("p3 "<< p3 << std::endl);
    std::vector<cv::Point> points(4);
    points[0] = cntr;
    points[1] = p1;
    points[2] = p2;
    points[3] = p3;
    showImage(current_image_);
    return points;
}

bool CableDetector::findCable(geometry_msgs::Point &cableLoc)
{
    //VISUALIZATION - include a spinOnce here to visualize the eigenvectors
    markers_.markers.clear();
    convexHulls_.clear();
    bool result;
    if(ms_sensor_->giveImage(current_image_))
    {
        if( ms_sensor_->giveDisparityImage(current_disparity_))
        {
            result= getCableLocation(cableLoc);
            ROS_INFO_STREAM("Cable position x: "<<cableLoc.x<<"\t"<<"Cable position y: "<<cableLoc.y<<"\t"<<"Cable position z: "<<cableLoc.z<<"\n");
            ros::Duration(0.1).sleep(); // this is required to avoid processing same image in next request
            return result;
        }
    }
    ROS_WARN("CableDetector::findCable : Cannot read image from multisense");
    return false;
}

bool CableDetector::isCableinHand()
{
    //VISUALIZATION - include a spinOnce here to visualize the eigenvectors
    bool cable_in_hand = false;
    markers_.markers.clear();
    convexHulls_.clear();
    if(ms_sensor_->giveImage(current_image_))
    {
        std::vector<std::vector<cv::Point> > contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::Mat imgHSV, outImg;

        cv::cvtColor(current_image_, imgHSV, cv::COLOR_BGR2HSV);
        colorSegment(imgHSV, outImg);

        // Find contours
        cv::findContours(outImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
        ROS_INFO_STREAM(" Contour Size "<< contours.size() << std::endl);
        if(!contours.empty())
        {
            double total_area = 0.0;
            std::vector<std::vector<cv::Point>> hull(contours.size());
            for( size_t i = 0; i< contours.size(); i++ )
            {
                cv::convexHull(contours[i], hull[i], false);
                //  Find the total area of all contours
                total_area += cv::contourArea( hull[i]);
            }
            ROS_INFO_STREAM(" Total Area "<< total_area << std::endl);
            if(total_area > CONTOUR_AREA_THRESHOLD)
                cable_in_hand = true;
        }
    }
    ros::Duration(0.5).sleep();
    ROS_INFO_STREAM(" Cable in hand? "<< cable_in_hand << std::endl);
    return cable_in_hand;
}

bool CableDetector::findCable(geometry_msgs::Pose& cablePose)
{
    //VISUALIZATION - include a spinOnce here to visualize the eigenvectors
    markers_.markers.clear();
    convexHulls_.clear();
    if(ms_sensor_->giveImage(current_image_))
    {
        if( ms_sensor_->giveDisparityImage(current_disparity_))
        {
            return getCablePose(cablePose);
        }
    }
    ROS_WARN("CableDetector::findCable pose: Cannot read image from multisense");
    return false;

}

void CableDetector::drawAxis(cv::Mat& img, cv::Point p, cv::Point q, cv::Scalar colour, const float scale)
{
    double angle;
    double hypotenuse;

    angle = std::atan2( (double) p.y - q.y, (double) p.x - q.x ); // angle in radians
    hypotenuse = std::sqrt( (double) (p.y - q.y) * (p.y - q.y) + (p.x - q.x) * (p.x - q.x));
    // double degrees = angle * 180 / CV_PI; // convert radians to degrees (0-180 range)
    // cout << "Degrees: " << abs(degrees - 180) << endl; // angle in 0-360 degrees range
    // Here we lengthen the arrow by a factor of scale

    q.x = (int) (p.x - scale * hypotenuse * std::cos(angle));
    q.y = (int) (p.y - scale * hypotenuse * std::sin(angle));
    cv::line(img, p, q, colour, 1, CV_AA);
    // create the arrow hooks
    p.x = (int) (q.x + 9 * std::cos(angle + CV_PI / 4));
    p.y = (int) (q.y + 9 * std::sin(angle + CV_PI / 4));
    cv::line(img, p, q, colour, 1, CV_AA);
    p.x = (int) (q.x + 9 * std::cos(angle - CV_PI / 4));
    p.y = (int) (q.y + 9 * std::sin(angle - CV_PI / 4));
    cv::line(img, p, q, colour, 1, CV_AA);
}

void CableDetector::setTrackbar()
{
    cv::namedWindow("binary thresholding");
    cv::createTrackbar("hl", "binary thresholding", &hsv_[0], 180);
    cv::createTrackbar("hu", "binary thresholding", &hsv_[1], 180);
    cv::createTrackbar("sl", "binary thresholding", &hsv_[2], 255);
    cv::createTrackbar("su", "binary thresholding", &hsv_[3], 255);
    cv::createTrackbar("vl", "binary thresholding", &hsv_[4], 255);
    cv::createTrackbar("vu", "binary thresholding", &hsv_[5], 255);

    cv::getTrackbarPos("hl", "binary thresholding");
    cv::getTrackbarPos("hu", "binary thresholding");
    cv::getTrackbarPos("sl", "binary thresholding");
    cv::getTrackbarPos("su", "binary thresholding");
    cv::getTrackbarPos("vl", "binary thresholding");
    cv::getTrackbarPos("vu", "binary thresholding");
}

void CableDetector::visualize_point(geometry_msgs::Point point, double r, double g, double b){

    ROS_INFO_STREAM("goal origin :\n"<< point );
    static int id = 0;
    visualization_msgs::Marker marker;
    // Set the frame ID and timestamp.  See the TF tutorials for information on these.
    marker.header.frame_id = VAL_COMMON_NAMES::WORLD_TF;
    marker.header.stamp = ros::Time::now();

    // Set the namespace and id for this marker.  This serves to create a unique ID
    // Any marker sent with the same namespace and id will overwrite the old one
    marker.ns = std::to_string(frameID_);
    marker.id = id++;

    // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
    marker.type = visualization_msgs::Marker::CUBE;

    // Set the marker action.  Options are ADD, DELETE, and new in ROS Indigo: 3 (DELETEALL)
    marker.action = visualization_msgs::Marker::ADD;

    // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
    marker.pose.position = point;
    marker.pose.orientation.w = 1.0f;

    marker.scale.x = 0.01;
    marker.scale.y = 0.01;
    marker.scale.z = 0.01;
    marker.color.a = 1.0; // Don't forget to set the alpha!
    marker.color.r = r;
    marker.color.g = g;
    marker.color.b = b;

    markers_.markers.push_back(marker);
}

void CableDetector::visualize_pose(geometry_msgs::Pose pose)
{
    static int id = 0;
    visualization_msgs::Marker marker;
    // Set the frame ID and timestamp.  See the TF tutorials for information on these.
    marker.header.frame_id = VAL_COMMON_NAMES::WORLD_TF;
    marker.header.stamp = ros::Time::now();

    // Set the namespace and id for this marker.  This serves to create a unique ID
    // Any marker sent with the same namespace and id will overwrite the old one
    marker.ns = "cable";
    marker.id = id++;

    // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
    marker.type = visualization_msgs::Marker::ARROW;

    // Set the marker action.  Options are ADD, DELETE, and new in ROS Indigo: 3 (DELETEALL)
    marker.action = visualization_msgs::Marker::ADD;

    // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
    marker.pose = pose;

    marker.scale.x = 0.1;
    marker.scale.y = 0.01;
    marker.scale.z = 0.01;
    marker.color.a = 1.0; // Don't forget to set the alpha!
    marker.color.r = 1.0;
    marker.color.g = 1.0;
    marker.color.b = 0.0;

    markers_.markers.push_back(marker);
}

geometry_msgs::Pose CableDetector::getPose() const
{
    return pose_;
}

geometry_msgs::PointStamped CableDetector::getOffsetPoint() const
{
    return geom_point0_;
}


std::vector<geometry_msgs::Quaternion> CableDetector::getQuat() const
{
    return quat_;
}

void CableDetector::visualize_direction(geometry_msgs::Point point1, geometry_msgs::Point point2)
{
    static int id = 0;
    visualization_msgs::Marker marker;
    // Set the frame ID and timestamp.  See the TF tutorials for information on these.
    marker.header.frame_id = VAL_COMMON_NAMES::WORLD_TF;
    marker.header.stamp = ros::Time::now();

    // Set the namespace and id for this marker.  This serves to create a unique ID
    // Any marker sent with the same namespace and id will overwrite the old one
    marker.ns = std::to_string(frameID_);
    marker.id = id++;

    // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
    marker.type = visualization_msgs::Marker::ARROW;
    marker.points.push_back(point1);
    marker.points.push_back(point2);
    // Set the marker action.  Options are ADD, DELETE, and new in ROS Indigo: 3 (DELETEALL)
    marker.action = visualization_msgs::Marker::ADD;
    // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
    //marker.pose.position = point;
    //marker.pose.orientation.w = 1.0f;
    marker.scale.x = 0.1;
    marker.scale.y = 0.01;
    marker.scale.z = 0.01;
    marker.color.a = 1.0; // Don't forget to set the alpha!
    marker.color.r = 1.0;
    marker.color.g = 0.0;
    marker.color.b = 0.0;
    markers_.markers.push_back(marker);
}

CableDetector::~CableDetector()
{
    marker_pub_.shutdown();
}

//-0.593457877636
//y: -0.91660118103  point right
//z: 0.772663831711

//x: -0.197707682848
//y: -0.73062390089   point down
//z: 0.794347047806

//x: -0.180317223072
//y: -1.69919061661    point up
//z: 0.807236671448

//x: 0.863122105598
//y: -0.865445196629   point left
//z: 0.776612520218
