/*
 * teleop_pr2
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Author: Kevin Watts

#include <cstdlib>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include "ros/ros.h"
#include "joy/Joy.h"
#include "geometry_msgs/Twist.h"
#include "mechanism_msgs/JointState.h"
#include "mechanism_msgs/JointStates.h"

#include "std_msgs/Float64.h"

#define TORSO_TOPIC "torso_lift_controller/set_command"
#define HEAD_TOPIC "head_controller/command"

class TeleopPR2 
{
   public:
  geometry_msgs::Twist cmd;
  std_msgs::Float64 torso_vel;
  //joy::Joy joy;
  double req_vx, req_vy, req_vw, req_torso, req_pan, req_tilt;
  double max_vx, max_vy, max_vw, max_vx_run, max_vy_run, max_vw_run;
  double max_pan, max_tilt, min_tilt, pan_step, tilt_step;
  int axis_vx, axis_vy, axis_vw, axis_pan, axis_tilt;
  int deadman_button, run_button, torso_dn_button, torso_up_button, head_button;
  bool deadman_no_publish_, torso_publish, head_publish;

  bool deadman_;

  ros::Time last_recieved_joy_message_time_;
  ros::Duration joy_msg_timeout_;

  ros::NodeHandle n_;
  ros::Publisher vel_pub_;
  ros::Publisher head_pub_;
  ros::Publisher torso_pub_;
  ros::Subscriber joy_sub_;

  TeleopPR2(bool deadman_no_publish = false) : max_vx(0.6), max_vy(0.6), max_vw(0.8), max_vx_run(0.6), max_vy_run(0.6), max_vw_run(0.8), max_pan(2.7), max_tilt(1.4), min_tilt(-0.4), pan_step(0.02), tilt_step(0.015), deadman_no_publish_(deadman_no_publish)
  { }

  void init()
  {
        torso_vel.data = 0;
        cmd.linear.x = cmd.linear.y = cmd.angular.z = 0;
        req_pan = req_tilt = 0;
        n_.param("max_vx", max_vx, max_vx);
        n_.param("max_vy", max_vy, max_vy);
        n_.param("max_vw", max_vw, max_vw);
        
        // Set max speed while running
        n_.param("max_vx_run", max_vx_run, max_vx_run);
        n_.param("max_vy_run", max_vy_run, max_vy_run);
        n_.param("max_vw_run", max_vw_run, max_vw_run);

        // Head pan/tilt parameters
        n_.param("max_pan", max_pan, max_pan);
        n_.param("max_tilt", max_tilt, max_tilt);
        n_.param("min_tilt", min_tilt, min_tilt);
        
        n_.param("tilt_step", tilt_step, tilt_step);
        n_.param("pan_step", pan_step, pan_step);
        
        n_.param("axis_pan", axis_pan, 0);
        n_.param("axis_tilt", axis_tilt, 2);

        n_.param("axis_vx", axis_vx, 3);
        n_.param("axis_vw", axis_vw, 0);
        n_.param("axis_vy", axis_vy, 2);
        
        n_.param("deadman_button", deadman_button, 0);
        n_.param("run_button", run_button, 0);
        n_.param("torso_dn_button", torso_dn_button, 0);
        n_.param("torso_up_button", torso_up_button, 0);
        n_.param("head_button", head_button, 0);

	double joy_msg_timeout;
        n_.param("joy_msg_timeout", joy_msg_timeout, -1.0); //default to no timeout
	if (joy_msg_timeout <= 0)
	  {
	    joy_msg_timeout_ = ros::Duration().fromSec(9999999);//DURATION_MAX;
	    ROS_DEBUG("joy_msg_timeout <= 0 -> no timeout");
	  }
	else
	  {
	    joy_msg_timeout_.fromSec(joy_msg_timeout);
	    ROS_DEBUG("joy_msg_timeout: %.3f", joy_msg_timeout_.toSec());
	  }

        ROS_DEBUG("max_vx: %.3f m/s\n", max_vx);
        ROS_DEBUG("max_vy: %.3f m/s\n", max_vy);
        ROS_DEBUG("max_vw: %.3f deg/s\n", max_vw*180.0/M_PI);
        
        ROS_DEBUG("max_vx_run: %.3f m/s\n", max_vx_run);
        ROS_DEBUG("max_vy_run: %.3f m/s\n", max_vy_run);
        ROS_DEBUG("max_vw_run: %.3f deg/s\n", max_vw_run*180.0/M_PI);
        
        ROS_DEBUG("tilt step: %.3f rad\n", tilt_step);
        ROS_DEBUG("pan step: %.3f rad\n", pan_step);
        
        ROS_DEBUG("axis_vx: %d\n", axis_vx);
        ROS_DEBUG("axis_vy: %d\n", axis_vy);
        ROS_DEBUG("axis_vw: %d\n", axis_vw);
        ROS_DEBUG("axis_pan: %d\n", axis_pan);
        ROS_DEBUG("axis_tilt: %d\n", axis_tilt);
        
        ROS_DEBUG("deadman_button: %d\n", deadman_button);
        ROS_DEBUG("run_button: %d\n", run_button);
        ROS_DEBUG("torso_dn_button: %d\n", torso_dn_button);
        ROS_DEBUG("torso_up_button: %d\n", torso_up_button);
        ROS_DEBUG("head_button: %d\n", head_button);
        ROS_DEBUG("joy_msg_timeout: %f\n", joy_msg_timeout);
        
        if (torso_dn_button != 0)
          torso_pub_ = n_.advertise<std_msgs::Float64>(TORSO_TOPIC, 1);
        
        if (head_button != 0)
          head_pub_ = n_.advertise<mechanism_msgs::JointStates>(HEAD_TOPIC, 1);

        vel_pub_ = n_.advertise<geometry_msgs::Twist>("cmd_vel", 1);

        joy_sub_ = n_.subscribe("joy", 10, &TeleopPR2::joy_cb, this);
      }
  
  ~TeleopPR2() { }

  
  /** Callback for joy topic **/
  void joy_cb(const joy::Joy::ConstPtr& joy_msg)
  {
    //Record this message reciept
    last_recieved_joy_message_time_ = ros::Time::now();

    deadman_ = (((unsigned int)deadman_button < joy_msg->get_buttons_size()) && joy_msg->buttons[deadman_button]);

    if (!deadman_)
      return;
    
    bool cmd_head = (((unsigned int)head_button < joy_msg->get_buttons_size()) && joy_msg->buttons[head_button]);
    
    // Base
    bool running = (((unsigned int)run_button < joy_msg->get_buttons_size()) && joy_msg->buttons[run_button]);
    double vx = running ? max_vx_run : max_vx;
    double vy = running ? max_vy_run : max_vy;
    double vw = running ? max_vw_run : max_vw;
    
    if((axis_vx >= 0) && (((unsigned int)axis_vx) < joy_msg->get_axes_size()) && !cmd_head)
      req_vx = joy_msg->axes[axis_vx] * vx;
    else
      req_vx = 0.0;
    if((axis_vy >= 0) && (((unsigned int)axis_vy) < joy_msg->get_axes_size()) && !cmd_head)
      req_vy = joy_msg->axes[axis_vy] * vy;
    else
      req_vy = 0.0;
    if((axis_vw >= 0) && (((unsigned int)axis_vw) < joy_msg->get_axes_size()) && !cmd_head)
      req_vw = joy_msg->axes[axis_vw] * vw;
    else
      req_vw = 0.0;
    
    // Head
    // Update commanded position by how joysticks moving
    // Don't add commanded position if deadman off
    if((axis_pan >= 0) && (((unsigned int)axis_pan) < joy_msg->get_axes_size()) && cmd_head && deadman_)
    {
      req_pan += joy_msg->axes[axis_pan] * pan_step;
      req_pan = std::max(std::min(req_pan, max_pan), -max_pan);
    }
    
    if ((axis_tilt >= 0) && (((unsigned int)axis_tilt) < joy_msg->get_axes_size()) && cmd_head && deadman_)
    {
      req_tilt += joy_msg->axes[axis_tilt] * tilt_step;
      req_tilt = std::max(std::min(req_tilt, max_tilt), min_tilt);
    }
    
    // Torso
    bool down = (((unsigned int)torso_dn_button < joy_msg->get_buttons_size()) && joy_msg->buttons[torso_dn_button]);
    bool up = (((unsigned int)torso_up_button < joy_msg->get_buttons_size()) && joy_msg->buttons[torso_up_button]);
    
    // Bring torso up/down 
    if (down && !up)
      req_torso = -0.01;
    else if (up && !down)
      req_torso = 0.01;
    else
      req_torso = 0;
  }

  
  void send_cmd_vel()
  {
    if(deadman_  &&
       last_recieved_joy_message_time_ + joy_msg_timeout_ > ros::Time::now())
    { 
      // Base
      cmd.linear.x = req_vx;
      cmd.linear.y = req_vy;
      cmd.angular.z = req_vw;
      vel_pub_.publish(cmd);
      
      // Torso
      torso_vel.data = req_torso;
      if (torso_dn_button != 0)
        torso_pub_.publish(torso_vel);
      
      // Head
      if (head_button != 0)
      {
        mechanism_msgs::JointState joint_cmd ;
        mechanism_msgs::JointStates joint_cmds;
        
        joint_cmd.name ="head_pan_joint";
        joint_cmd.position = req_pan;
        joint_cmds.joints.push_back(joint_cmd);
        joint_cmd.name="head_tilt_joint";
        joint_cmd.position = req_tilt;
        joint_cmds.joints.push_back(joint_cmd);
        
        head_pub_.publish(joint_cmds);
      }
      
      if (req_torso != 0)
        fprintf(stdout,"teleop_base:: %f, %f, %f. Head:: %f, %f. Torso cmd: %f.\n", cmd.linear.x, cmd.linear.y, cmd.angular.z, req_pan, req_tilt, torso_vel.data);
      else
        fprintf(stdout,"teleop_base:: %f, %f, %f. Head:: %f, %f\n", cmd.linear.x ,cmd.linear.y, cmd.angular.z, req_pan, req_tilt);
    }
    else
    {
      // Publish zero commands iff deadman_no_publish is false
      cmd.linear.x = cmd.linear.y = cmd.angular.z = 0;
      torso_vel.data = 0;
      if (!deadman_no_publish_)
      {
        // Base
        vel_pub_.publish(cmd);

        // Torso
        if (torso_dn_button != 0)
          torso_pub_.publish(torso_vel);
        
        // Publish head
        if (head_button != 0)
        {
          mechanism_msgs::JointState joint_cmd ;
          mechanism_msgs::JointStates joint_cmds;
          
          joint_cmd.name ="head_pan_joint";
          joint_cmd.position = req_pan;
          joint_cmds.joints.push_back(joint_cmd);
          joint_cmd.name="head_tilt_joint";
          joint_cmd.position = req_tilt;
          joint_cmds.joints.push_back(joint_cmd);
          
          head_pub_.publish(joint_cmds);
        }
        
      }
    }
  }
};

int main(int argc, char **argv)
{
  ros::init(argc, argv, "teleop_pr2");
  const char* opt_no_publish    = "--deadman_no_publish";
  
  bool no_publish = false;
  for(int i=1;i<argc;i++)
  {
    if(!strncmp(argv[i], opt_no_publish, strlen(opt_no_publish)))
      no_publish = true;
  }
  
  TeleopPR2 teleop_pr2(no_publish);
  teleop_pr2.init();
  
  ros::Rate pub_rate(20);
  
  while (teleop_pr2.n_.ok())
  {
    ros::spinOnce(); 
    teleop_pr2.send_cmd_vel();
    pub_rate.sleep();
  }
  
  exit(0);
  return 0;
}

