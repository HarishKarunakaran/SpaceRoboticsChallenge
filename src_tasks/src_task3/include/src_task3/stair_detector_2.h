#ifndef STAIR_DETECTOR_2_H
#define STAIR_DETECTOR_2_H

#include <geometry_msgs/Point.h>

#include <tough_perception_common/MultisenseImage.h>
#include <tough_perception_common/MultisensePointCloud.h>
#include <tough_perception_common/PointCloudHelper.h>

#include <geometry_msgs/Point.h>
#include <geometry_msgs/PointStamped.h>
#include <visualization_msgs/MarkerArray.h>

#include <tf/transform_broadcaster.h>
#include "val_task3_utils.h"
#include <iostream>
#include <vector>

class stair_detector_2
{
    ros::NodeHandle nh_;
    ros::Publisher marker_pub_;
    ros::Publisher points_pub_;
    ros::Publisher blacklist_pub_;
    ros::Subscriber pcl_sub_;

    tf::TransformListener tf_listener_;

    tough_perception::MultisensePointCloud point_cloud_listener_;

    std::vector<std::vector<geometry_msgs::Pose>> detections_;

public:

    typedef pcl::PointXYZ                   Point;
    typedef pcl::PointCloud<Point>          PointCloud;
    typedef pcl::Normal                     Normal;
    typedef pcl::PointCloud<Normal>         NormalCloud;
    typedef pcl::PointXYZL                  LabeledPoint;
    typedef pcl::PointCloud<LabeledPoint>   LabeledCloud;

    stair_detector_2(ros::NodeHandle nh);

    void cloudCB(const stair_detector_2::PointCloud::ConstPtr &cloud);
    bool getDetections(std::vector<geometry_msgs::Pose> &detections);

    ~stair_detector_2();

private:
    PointCloud::Ptr prefilterCloud(const PointCloud::ConstPtr &cloud_raw) const;
    NormalCloud::Ptr estimateNormals(const PointCloud::ConstPtr &filtered_cloud) const;
    bool estimateStairs(const PointCloud::ConstPtr &filtered_cloud,
                            const NormalCloud::Ptr &filtered_normals);

    float estimateStairPose(const PointCloud::ConstPtr &filtered_cloud,
                            const std::vector<pcl::PointIndices> &step_clusters,
                            const Eigen::Vector3f &stairs_dir, const Eigen::Vector3f &robot_pos,
                            Eigen::Affine3f &stairs_pose) const;

    std::vector<pcl::PointIndices> findStairClusters(const Eigen::Vector3f &avg_normal,
                                                     const PointCloud::ConstPtr &filtered_cloud) const;

    void publishClusters(const PointCloud::ConstPtr &cloud, const std::vector<pcl::PointIndices> &clusters) const;
    void publishNormals(const PointCloud &points, const NormalCloud &normals) const;

    Eigen::Vector3f modelToVector(const pcl::ModelCoefficients &model) const;

    task3Utils utils_;
};
#endif // STAIR_DETECTOR_2_H
