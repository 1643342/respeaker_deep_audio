# doa_node.py
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32
from .tuning.py import find

class DOANode(Node):
    def __init__(self):
        super().__init__('doa_publisher')
        self.pub = self.create_publisher(Int32, 'doa_angle', 10)

        # initialize device
        self.mic = find()
        if not self.mic:
            self.get_logger().error("ReSpeaker Mic Array v2.0 not found!")
            exit()

        self.timer = self.create_timer(0.1, self.timer_callback)

    def timer_callback(self):
        angle = self.mic.direction
        msg = Int32()
        msg.data = angle
        self.pub.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = DOANode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
