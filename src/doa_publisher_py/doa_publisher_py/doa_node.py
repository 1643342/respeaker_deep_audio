#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32
import usb.core, usb.util

# local tuning module (your Python3-fixed version)
from .tuning import Tuning

class DoaNode(Node):
    def __init__(self):
        super().__init__('doa_publisher')
        self.pub = self.create_publisher(Float32, '/doa_deg', 10)
        self.timer = self.create_wall_timer(0.1, self.tick)  # 10Hz
        self.dev = usb.core.find(idVendor=0x2886, idProduct=0x0018)
        if not self.dev:
            self.get_logger().fatal("ReSpeaker 0x2886:0x0018 not found")
            raise SystemExit(1)
        self.tuning = Tuning(self.dev)
        self.get_logger().info("DOA publisher started")

    def tick(self):
        try:
            angle = float(self.tuning.direction)  # 0..359
            m = Float32()
            m.data = angle
            self.pub.publish(m)
        except Exception as e:
            self.get_logger().warn(f"DOA read error: {e}")

def main():
    rclpy.init()
    node = DoaNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()
