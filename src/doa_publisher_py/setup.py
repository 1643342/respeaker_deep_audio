from setuptools import setup
package_name = 'doa_publisher_py'

setup(
    name=package_name,
    version='0.0.1',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='you',
    maintainer_email='Gonzab18@my.erau.edu',
    description='Publish DOA from ReSpeaker via USB',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'doa_node = doa_publisher_py.doa_node:main',
        ],
    },
)

