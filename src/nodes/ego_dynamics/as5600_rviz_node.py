#!/usr/bin/env python3
import math
import re
import serial

import rclpy
from rclpy.node import Node
from rclpy.duration import Duration

from std_msgs.msg import Float32, String, Bool
from sensor_msgs.msg import JointState
from visualization_msgs.msg import Marker


LINE_RE = re.compile(
    r"AS5600_RT ch=(?P<ch>\d+) raw=(?P<raw>\d+) deg=(?P<deg>-?\d+(?:\.\d+)?) "
    r"status=(?P<status>0x[0-9A-Fa-f]+) agc=(?P<agc>\d+) mag=(?P<mag>\d+) judge=(?P<judge>[A-Z_]+)"
)


def yaw_to_quat(yaw_rad: float):
    half = yaw_rad * 0.5
    return 0.0, 0.0, math.sin(half), math.cos(half)


def angular_diff_deg(curr: float, prev: float) -> float:
    return ((curr - prev + 180.0) % 360.0) - 180.0


class As5600RvizNode(Node):
    def __init__(self):
        super().__init__('as5600_rviz_node')

        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('frame_id', 'as5600_frame')
        self.declare_parameter('joint_name', 'as5600_joint')
        self.declare_parameter('jump_threshold_deg', 20.0)

        self.port = self.get_parameter('port').value
        self.baud = int(self.get_parameter('baud').value)
        self.frame_id = self.get_parameter('frame_id').value
        self.joint_name = self.get_parameter('joint_name').value
        self.jump_threshold_deg = float(self.get_parameter('jump_threshold_deg').value)

        self.angle_pub = self.create_publisher(Float32, '/as5600/angle_deg', 20)
        self.judge_pub = self.create_publisher(String, '/as5600/magnet_judge', 20)
        self.jump_pub = self.create_publisher(Bool, '/as5600/jump_detected', 20)
        self.joint_pub = self.create_publisher(JointState, '/joint_states', 20)
        self.marker_pub = self.create_publisher(Marker, '/as5600/marker', 20)

        self.ser = serial.Serial(self.port, self.baud, timeout=0.02)
        self.ser.reset_input_buffer()

        self.prev_angle_deg = None
        self.prev_stamp = None

        self.timer = self.create_timer(0.01, self._poll_serial)
        self.get_logger().info(f'Listening on {self.port} @ {self.baud}')

    def _poll_serial(self):
        try:
            line = self.ser.readline()
        except Exception as exc:
            self.get_logger().error(f'serial read error: {exc}')
            return

        if not line:
            return

        text = line.decode(errors='ignore').strip()
        match = LINE_RE.search(text)
        if not match:
            return

        angle_deg = float(match.group('deg'))
        judge = match.group('judge')

        now = self.get_clock().now()
        angle_rad = math.radians(angle_deg)

        angle_msg = Float32()
        angle_msg.data = angle_deg
        self.angle_pub.publish(angle_msg)

        judge_msg = String()
        judge_msg.data = judge
        self.judge_pub.publish(judge_msg)

        jump = False
        if self.prev_angle_deg is not None and self.prev_stamp is not None:
            delta_deg = abs(angular_diff_deg(angle_deg, self.prev_angle_deg))
            dt = (now - self.prev_stamp).nanoseconds / 1e9
            if dt > 0.0 and dt < 0.5 and delta_deg > self.jump_threshold_deg:
                jump = True
                self.get_logger().warn(
                    f'Angle jump detected: Δ={delta_deg:.2f} deg in {dt*1000:.1f} ms'
                )

        jump_msg = Bool()
        jump_msg.data = jump
        self.jump_pub.publish(jump_msg)

        joint_msg = JointState()
        joint_msg.header.stamp = now.to_msg()
        joint_msg.name = [self.joint_name]
        joint_msg.position = [angle_rad]
        self.joint_pub.publish(joint_msg)

        marker = Marker()
        marker.header.frame_id = self.frame_id
        marker.header.stamp = now.to_msg()
        marker.ns = 'as5600'
        marker.id = 0
        marker.type = Marker.ARROW
        marker.action = Marker.ADD
        marker.scale.x = 0.20
        marker.scale.y = 0.03
        marker.scale.z = 0.03
        marker.color.a = 1.0
        marker.color.r = 0.1
        marker.color.g = 0.8 if judge == 'GOOD' else 0.9
        marker.color.b = 0.1 if judge == 'GOOD' else 0.1
        marker.pose.position.x = 0.0
        marker.pose.position.y = 0.0
        marker.pose.position.z = 0.0
        qx, qy, qz, qw = yaw_to_quat(angle_rad)
        marker.pose.orientation.x = qx
        marker.pose.orientation.y = qy
        marker.pose.orientation.z = qz
        marker.pose.orientation.w = qw
        marker.lifetime = Duration(seconds=0.3).to_msg()
        self.marker_pub.publish(marker)

        self.prev_angle_deg = angle_deg
        self.prev_stamp = now

    def destroy_node(self):
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception:
            pass
        super().destroy_node()


def main():
    rclpy.init()
    node = As5600RvizNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
