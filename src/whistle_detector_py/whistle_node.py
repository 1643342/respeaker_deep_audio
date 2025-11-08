#!/usr/bin/env python3
import os, time, rclpy
from rclpy.node import Node
from std_msgs.msg import Int8

# EI runner (requires: pip install edge_impulse_linux; numpy<2; PyAudio; OpenCV via apt)
from edge_impulse_linux.audio import AudioImpulseRunner

class WhistleNode(Node):
    def __init__(self):
        super().__init__('whistle_detector')
        self.pub = self.create_publisher(Int8, '/whistle_detected', 10)

        # Parameters
        self.declare_parameter('model_path', os.path.join(
            os.path.dirname(__file__), '..', 'models', 'model.eim'))
        self.declare_parameter('label', 'whistle')
        self.declare_parameter('threshold', 0.8)

        model_path = self.get_parameter('model_path').get_parameter_value().string_value
        self.label = self.get_parameter('label').get_parameter_value().string_value
        self.thresh = float(self.get_parameter('threshold').value)

        self.runner = AudioImpulseRunner(model_path)
        info = self.runner.init()
        labels = info['model_parameters']['labels']
        if self.label not in labels:
            self.get_logger().warn(f"Label '{self.label}' not in {labels}")

        self.get_logger().info(f"Loaded EI model: {info['project']['name']} ({model_path})")
        self.timer = self.create_timer(0.0, self.loop)  # run as fast as audio callback produces

    def loop(self):
        try:
            for res, _ in self.runner.classifier():  # blocks until a frame is ready
                scores = res.get('result', {}).get('classification', {})
                s = float(scores.get(self.label, 0.0))
                msg = Int8()
                msg.data = 1 if s >= self.thresh else 0
                self.pub.publish(msg)
                # optional debug
                # self.get_logger().info(f"{self.label}={s:.2f} -> {msg.data}")
                break
        except KeyboardInterrupt:
            raise
        except Exception as e:
            self.get_logger().error(f"classifier error: {e}")

def main():
    rclpy.init()
    node = WhistleNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.runner.stop()
        node.destroy_node()
        rclpy.shutdown()
