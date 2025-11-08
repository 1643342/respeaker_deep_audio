from setuptools import setup

package_name = 'whistle_detector_py'

setup(
    name=package_name,
    version='0.0.1',
    packages=[package_name],
    package_data={package_name: ['../models/*']},
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='you',
    maintainer_email='you@example.com',
    description='Whistle detector using Edge Impulse (Python)',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'whistle_node = whistle_detector_py.whistle_node:main',
        ],
    },
)
