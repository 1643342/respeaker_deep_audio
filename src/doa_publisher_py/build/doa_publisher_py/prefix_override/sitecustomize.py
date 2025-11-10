import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/dev/ros2_ws/src/respeaker_deep_audio/src/doa_publisher_py/install/doa_publisher_py'
