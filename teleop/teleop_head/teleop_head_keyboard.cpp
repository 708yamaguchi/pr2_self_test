/*
 * teleop_head_keyboard
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

/**

@mainpage

@b teleop_head_keyboard teleoperates a PR-2 head using keyboard commands. 
WASD pans and tilts the head.

<hr>

@section usage Usage
@verbatim
$ teleop_head_keyboard [standard ROS args]
@endverbatim

Key mappings are printed to screen on startup. 

<hr>

@section topic ROS topics

Subscribes to (name/type):
- None

Publishes to (name / type):
- @b "head_controller/set_command_array"/JointCmd : velocity to the robot; sent on every keypress.

<hr>

@section parameters ROS parameters

- None

 **/

#include <termios.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>

#include <ros/ros.h>
#include <robot_msgs/JointCmd.h>

#define HEAD_TOPIC "/head_controller/set_command_array"

#define KEYCODE_A 0x61
#define KEYCODE_D 0x64
#define KEYCODE_S 0x73
#define KEYCODE_W 0x77 

// should we continuously send commands?
bool always_command = false;



class THK_Node
{
  private:
  double req_tilt, req_pan;
  double max_pan, max_tilt, min_tilt, tilt_step, pan_step;

  ros::NodeHandle n_;
  ros::Publisher head_pub_;

  public:
  void init()
  {
    req_tilt = 0.0;
    req_pan = 0.0;
 
    head_pub_ = n_.advertise<robot_msgs::JointCmd>(HEAD_TOPIC, 1);
    
    n_.param("max_pan", max_pan, 2.7);
    n_.param("max_tilt", max_tilt, 1.4);
    n_.param("min_tilt", min_tilt, -0.4);
    n_.param("tilt_step", tilt_step, 0.02);
    n_.param("pan_step", pan_step, 0.02);
    }
  
  ~THK_Node()   { }
  void keyboardLoop();

};

THK_Node* thk;
int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{
  tcsetattr(kfd, TCSANOW, &cooked);
  exit(0);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "head_keyboard");

  THK_Node thk;
  thk.init();
  //thk = new THK_Node();

  signal(SIGINT,quit);

  thk.keyboardLoop();

  return(0);
}

void THK_Node::keyboardLoop()
{
  char c;
  bool dirty=false;

  // get the console in raw mode
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  // Setting a new line, then end of file
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);

  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Use 'WASD' to pan and tilt");


  for(;;)
  {
    // get the next event from the keyboard
    if(read(kfd, &c, 1) < 0)
    {
      perror("read():");
      exit(-1);
    }

    switch(c)
    {
    case KEYCODE_W:
      req_tilt += tilt_step;
      dirty = true;
      break;
    case KEYCODE_S:
      req_tilt -= tilt_step;
      dirty = true;
      break;
    case KEYCODE_A:
      req_pan += pan_step;
      dirty = true;
      break;
    case KEYCODE_D:
      req_pan -= pan_step;
      dirty = true;
      break;
    }

    req_tilt = std::max(std::min(req_tilt, max_tilt), min_tilt);
    req_pan = std::max(std::min(req_pan, max_pan), -max_pan);

    if (dirty == true)
    {
      robot_msgs::JointCmd joint_cmds ;
      joint_cmds.positions.push_back(req_pan);
      joint_cmds.positions.push_back(req_tilt);
      joint_cmds.velocity.push_back(0.0);
      joint_cmds.velocity.push_back(0.0);
      joint_cmds.acc.push_back(0.0);
      joint_cmds.acc.push_back(0.0);
      joint_cmds.names.push_back("head_pan_joint");
      joint_cmds.names.push_back("head_tilt_joint");
      
      head_pub_.publish(joint_cmds);
    }
  }
}