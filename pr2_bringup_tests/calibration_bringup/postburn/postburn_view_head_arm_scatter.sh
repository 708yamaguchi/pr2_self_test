#! /bin/bash
rosrun pr2_calibration_estimation post_process.py /u/pr2admin/postburn/cal_measurements.bag /u/pr2_admin/postburn `rospack find pr2_calibration_launch`/view_results/head_arm_config.yaml

