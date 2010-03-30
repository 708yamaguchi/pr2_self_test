#! /usr/bin/env python
#
# Copyright (c) 2010, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Willow Garage, Inc. nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

##\author Kevin Watts
##\brief Sends goals to arm to move it in collision free way

PKG = 'life_test'
import roslib; roslib.load_manifest(PKG)
import rospy
import actionlib
import random

from pr2_controllers_msgs.msg import *
from trajectory_msgs.msg import JointTrajectoryPoint

ranges = {
    'r_shoulder_pan_joint': (-2.0, 0.4),
    'r_shoulder_lift_joint': (-0.4, 1.25),
    'r_upper_arm_roll_joint': (-3.65, 0.45),
    'r_elbow_flex_joint': (-2.0, -0.05),
    'r_forearm_roll_joint': (-3.14, 3.14),
    'r_wrist_flex_joint': (-1.8, -0.2),
    'r_wrist_roll_joint': (-3.14, 3.14),
    'l_shoulder_pan_joint': (-0.4, 2.0),
    'l_shoulder_lift_joint': (-0.4, 1.25),
    'l_upper_arm_roll_joint': (-0.45, 3.65),
    'l_elbow_flex_joint': (-2.0, -0.05),
    'l_forearm_roll_joint': (-3.14, 3.14),
    'l_wrist_flex_joint': (-1.8, -0.2),
    'l_wrist_roll_joint': (-3.14, 3.14)
}


if __name__ == '__main__':
    rospy.init_node('arm_cmder_client')
    client = actionlib.SimpleActionClient('collision_free_arm_trajectory_action_both_arms', 
                                          JointTrajectoryAction)
    rospy.loginfo('Waiting for server for right collision free arm commander')
    client.wait_for_server()

    rospy.loginfo('Right, left arm commanders ready')
    my_rate = rospy.Rate(2.0)

    left = True
    last_switch = rospy.get_time()

    while not rospy.is_shutdown():
        goal = JointTrajectoryGoal()
        point = JointTrajectoryPoint()
        point.time_from_start = rospy.Duration.from_sec(0)

        goal.trajectory.points.append(point)

	positions = {}
	for joint, range in ranges.iteritems():
	    positions[joint] = random.uniform(range[0], range[1])
	positions['r_shoulder_pan_joint'] = positions['l_shoulder_pan_joint'] + (ranges['r_shoulder_pan_joint'][0] - ranges['l_shoulder_pan_joint'][0])

        for joint, range in ranges.iteritems():
            goal.trajectory.joint_names.append(joint)
            goal.trajectory.points[0].positions.append(positions[joint])
            
        rospy.logdebug('Sending goal to arms.')
        client.send_goal(goal)

        client.wait_for_result(rospy.Duration.from_sec(3))

        my_rate.sleep()

