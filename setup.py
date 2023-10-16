#!/usr/bin/env python
import platform
import pathlib
import subprocess
import sys
import shutil
import inspect

UBUNTU_DEPS = 'build-essential libffi-dev libreadline-dev zlib1g-dev libssl-dev libglu1-mesa-dev xorg-dev libopenal-dev'
MACOS_DEPS = 'cmake'

def pip_install(packages):
	print(f'==== {inspect.currentframe().f_code.co_name} ====')

	for package in packages:
		try:
			subprocess.check_call([sys.executable, "-m", "pip", "install", package])
		except subprocess.CalledProcessError as e:
			print(f'Error installing package: {package}')
			raise e

def install_os_packages():
	print(f'==== {inspect.currentframe().f_code.co_name} ====')
	uname = platform.uname()
	print(uname)

	if platform.system() == 'Windows':
		pass # need to find a way to install windows deps
	elif 'Ubuntu' in uname.version:
		try:
			subprocess.check_call(['sudo', 'apt-get', 'install', '-y'] + UBUNTU_DEPS.split())
		except subprocess.CalledProcessError as e:
			print('Error installing Ubuntu dependencies')
			raise e
	elif 'Darwin' in uname:
		# do we need to install brew first?
		if shutil.which('brew') is None:
			try:
				subprocess.check_call(['/bin/bash', '-c', '$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)'])
			except subprocess.CalledProcessError as e:
				print('Error installing brew')
				raise e
		try:
			subprocess.check_call(['brew', 'install'] + MACOS_DEPS.split())
		except subprocess.CalledProcessError as e:
			print('Error installing macOS dependencies')
			raise e
	else:
		raise Exception('Unsupported OS')

def cmake_configure(type='Debug'):
	print(f'==== {inspect.currentframe().f_code.co_name} ====')

	try:
		subprocess.check_call(['cmake', '-B', 'build', f'-DCMAKE_BUILD_TYPE={type}', '-DG_BUILD_EXAMPLES=1', '-DG_BUILD_TESTS=1'])
	except subprocess.CalledProcessError as e:
		print('Error configuring cmake')
		raise e

def cmake_build():
	print(f'==== {inspect.currentframe().f_code.co_name} ====')

	try:
		subprocess.check_call(['cmake', '--build', 'build'])
	except subprocess.CalledProcessError as e:
		print('Error building cmake')
		raise e

def check_python_version():
	print(f'==== {inspect.currentframe().f_code.co_name} ====')

	major_str, minor_str, _ = platform.python_version_tuple()
	major, minor = int(major_str), int(minor_str)
	if major_str != 3 and minor < 9:
		raise Exception('Python 3.9+ is required')

	print(f'Python version: {major}.{minor}: OK')

if __name__ == '__main__':
	# make sure we have the correct version(s) of Python
	check_python_version()

	# make sure we have pip installed, if not install it
	try:
		import pip
	except ImportError as e:
		subprocess.check_call([sys.executable, "-m", "ensurepip", "--upgrade", "--default-pip"])

	# install the necessary OS packages
	install_os_packages()

	# install the necessary python packages
	pip_install(['gitman', 'jinja2'])

	# configure cmake
	cmake_configure()

	# build the project
	cmake_build()