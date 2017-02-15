#!/bin/bash
## Usage: bash setup2.sh
## Author: sumanth
## Purpose: setups the system for ihmc controllers
##
## Options:
##   none
##

echo "$(tput setaf 1)downloading gazebo models$(tput sgr0)"
wget -P /tmp/ https://bitbucket.org/osrf/gazebo_models/get/default.tar.gz
tar -xvf /tmp/default.tar.gz -C $HOME/.gazebo/models --strip 1
rm /tmp/default.tar.gz

echo "$(tput setaf 1)Pre-build ihmc_ros_java_adapter$(tput sgr0)"
source /opt/nasa/indigo/setup.bash
roslaunch ihmc_valkyrie_ros valkyrie_warmup_gradle_cache.launch

echo "$(tput setaf 1)clone the whole ihmc repo$(tput sgr0)"
cd ~ && git clone https://github.com/ihmcrobotics/ihmc-open-robotics-software.git

echo "$(tput setaf 1)compile the ihmc repo$(tput sgr0)"
cd ihmc-open-robotics-software
git checkout 0.8.1
./gradlew 
./gradlew -q deployLocal

echo "$(tput setaf 1)checking and installing and missing ros dependecies$(tput sgr0)"
# TODO:this shoould be done with rosdep
sudo apt-get install ruby ros-indigo-pcl-ros ros-indigo-pcl-conversions ros-indigo-moveit-full ros-indigo-trac-ik ros-indigo-footstep-planner ros-indigo-humanoid-localization ros-indigo-multisense-ros  ros-indigo-laser-assembler

#check if the workspace is already set
# setup the workspace
if [ -d $"/home/$USER/catkin_ws" ]; then  
  WORKSPACE="catkin_ws"
  echo "$(tput setaf 1)found workspace catkin_ws$(tput sgr0)"
elif [ -d $"/home/$USER/indigo_ws" ]; then  
  WORKSPACE="indigo_ws"
  echo "$(tput setaf 1)found workspace indigo_ws$(tput sgr0)"  
else
  "$(tput setaf 1)no workspace found setting up indigo_ws$(tput sgr0)" 
  WORKSPACE="indigo_ws"
  mkdir -p ~/indigo_ws/src && cd ~/indigo_ws/src
  catkin_init_workspace
  cd ~/indigo_ws
  catkin_make 
fi

#TODO: this to account for the fix in humanoid navigation package
#so, we clone the fixed repo
if [ -d $"/home/$USER/$WORKSPACE/src/humanoid_navigation" ]; then  
  echo "$(tput setaf 1)updating the humanoid_navigation package$(tput sgr0)"
  cd ~/$WORKSPACE/src/humanoid_navigation
  git checkout master
  git pull
  cd ~/$WORKSPACE    
  catkin_make
else
  echo "$(tput setaf 1)no humanoid_navigation repo found, clonning it$(tput sgr0)" 
  git clone https://github.com/ninja777/humanoid_navigation.git ~/$WORKSPACE/src/humanoid_navigation
  cd ~/$WORKSPACE/src/humanoid_navigation
  git checkout indigo-devel
  cd ~/$WORKSPACE   
  catkin_make
fi

# update the src repo
if [ -d $"/home/$USER/$WORKSPACE/src/space_robotics_challenge" ]; then  
  echo "$(tput setaf 1)updating the repo$(tput sgr0)"
  cd ~/$WORKSPACE/src/space_robotics_challenge
  git checkout master
  git pull
  cd ~/$WORKSPACE    
  catkin_make
else
  echo "$(tput setaf 1)no src repo found, clonning it$(tput sgr0)" 
  git clone https://gitlab.com/whrl/space_robotics_challenge.git ~/$WORKSPACE/src/space_robotics_challenge
  cd ~/$WORKSPACE   
  catkin_make
fi

echo "$(tput setaf 1)compiling the catkin_workspace$(tput sgr0)"
cd ~/$WORKSPACE     
catkin_make

echo "$(tput setaf 1)testing the setup......$(tput sgr0)"
echo "$(tput setaf 1)launching valkyire with controllers...$(tput sgr0)" \
source /opt/nasa/indigo/setup.bash
cd ~/$WORKSPACE 
rm -rf devel/ build/
catkin_make
source devel/setup.bash
roslaunch val_bringup qual2.launch


