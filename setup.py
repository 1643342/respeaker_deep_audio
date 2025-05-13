from setuptools import setup

package_name = 'respeaker_deep_audio'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Your Name',
    maintainer_email='you@yourdomain.com',
    description='ROS 2 driver wrapper for ReSpeaker Mic Array v2.0',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            # mic_node will map to respeaker_deep_audio/mic_node.py:main
            'mic_node = respeaker_deep_audio.mic_node:main',
        ],
    },
)
