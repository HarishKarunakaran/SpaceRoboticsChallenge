#ifndef FINISH_BOX_DETECTOR_H
#define FINISH_BOX_DETECTOR_H


#include <ros/ros.h>
#include <nav_msgs/OccupancyGrid.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <opencv/cv.h>

struct Point2D{
    int x;
    int y;

};

inline bool operator == (const Point2D& lhs, const Point2D& rhs){
    return (abs(lhs.x - rhs.x) < 5 || abs(lhs.y - rhs.y) < 5);
}

inline bool operator<(const Point2D& lhs, const Point2D& rhs)
{
    return lhs.x < rhs.x || lhs.y < rhs.y;
}

class FinishBoxDetector{

public:
    FinishBoxDetector(ros::NodeHandle &n);
    ~FinishBoxDetector();
    bool getFinishBoxCenters(std::vector<geometry_msgs::Point> &centers, bool force_detection = false);

private:
    void detectFinishBox(const nav_msgs::OccupancyGrid::Ptr msg);
    void showImage(cv::Mat, std::string caption="FinishBoxDetection");
    ros::NodeHandle nh_;
    ros::Subscriber pointcloudSub_;
    ros::Publisher  mapPub_;
    nav_msgs::OccupancyGrid occGrid_;
    cv::Mat map_image_;
    std::set<Point2D> finish_box_centers_;
    bool start_detection;

    float map_resolution;
    float map_height;
    float map_width;
    float map_x_offset;
    float map_y_offset;

};

#endif // FINISH_BOX_DETECTOR_H


