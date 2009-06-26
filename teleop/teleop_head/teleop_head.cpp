#include <cstdlib>
#include <unistd.h>
#include <math.h>
#include "ros/node.h"
#include "joy/Joy.h"

#include "robot_srvs/SetJointCmd.h"
#include "robot_msgs/JointCmd.h"

using namespace ros;

class TeleopHead : public Node
{
   public:
      joy::Joy joy;
      double req_pan, req_tilt, max_pan, max_tilt;
      int axis_pan, axis_tilt;
      int deadman_button, passthrough_button;
  double pan_step, tilt_step;
      bool deadman_no_publish_;


  TeleopHead(bool deadman_no_publish = false) : Node("teleop_head"), max_pan(0.6), max_tilt(0.4), pan_step(0.1), tilt_step(0.1), deadman_no_publish_(deadman_no_publish)
      {
        //     cmd.vx = cmd.vy = cmd.vw = 0;
         if (!hasParam("max_pan") || !getParam("max_pan", max_pan))
            ROS_WARN("maximum pan not set. Assuming 0.6");
         if (!hasParam("max_tilt") || !getParam("max_tilt", max_tilt))
            ROS_WARN("maximum tilt not set. Assuming 0.4");

         param<int>("axis_pan", axis_pan, 4);
         param<int>("axis_tilt", axis_tilt, 5);
         param<int>("deadman_button", deadman_button, 0);
         param<int>("passthrough_button", passthrough_button, 1);

 
         subscribe("joy", joy, &TeleopHead::joy_cb, 1);
         advertise<robot_msgs::JointCmd>("head_controller/set_command_array", 1) ;

         printf("done with ctor\n");
      }
  ~TeleopHead()
  {
    unsubscribe("joy") ;
    unadvertise("head_controller/set_command_array") ;

  }
      void joy_cb()
      {
         if((axis_pan >= 0) && (((unsigned int)axis_pan) < joy.get_axes_size()))
         {
           req_pan += joy.axes[axis_pan] * pan_step;
           req_pan = std::max(std::min(req_pan, max_pan), -max_pan);
         }

         if ((axis_tilt >= 0) && (((unsigned int)axis_tilt) < joy.get_axes_size()))
         {
           req_tilt += joy.axes[axis_tilt] * tilt_step;
           req_tilt = std::max(std::min(req_tilt, max_tilt), -max_tilt);
         }
         robot_msgs::JointCmd joint_cmds ;

         joint_cmds.positions.push_back(req_pan);
         joint_cmds.positions.push_back(req_tilt);
         joint_cmds.velocity.push_back(0.0);
         joint_cmds.velocity.push_back(0.0);
         joint_cmds.acc.push_back(0.0);
         joint_cmds.acc.push_back(0.0);
         joint_cmds.names.push_back("head_pan_joint");
         joint_cmds.names.push_back("head_tilt_joint");
         publish("head_controller/set_command_array", joint_cmds) ;

      }
};

int main(int argc, char **argv)
{
   ros::init(argc, argv);
   const char* opt_no_publish    = "--deadman_no_publish";

   bool no_publish = false;
   for(int i=1;i<argc;i++)
   {
     if(!strncmp(argv[i], opt_no_publish, strlen(opt_no_publish)))
       no_publish = true;
   }
   TeleopHead teleop_head(no_publish);
   while (teleop_head.ok())
   {
      usleep(50000);
   }
   
   exit(0);
   return 0;
}

