# AgilisPiezo Library

A C++ library for controlling [Newport Agilis AG-UC2 and AG-UC8 Piezo Controllers](https://www.newport.com/f/agilis-piezo-motion-controllers) on Windows and Linux environments.

## Features

- USB and RS232 serial communication with [Newport Agilis Piezo controllers](https://www.newport.com/f/agilis-piezo-motion-controllers)
- Support for the entire Agilis UC command set
- Thread-safe implementation
- Asynchronous operation support
- Comprehensive error handling and logging

## Requirements

- C++14 compatible compiler
- Asio library (standalone version, not Boost Asio)
- FTDI USB driver (for USB communication)
- CMake 3.10 or higher (for building)

## Installation

### Installing Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential git cmake libasio-dev

# Fedora/RHEL/CentOS
sudo dnf install -y gcc-c++ git cmake asio-devel

# macOS (with Homebrew)
brew install cmake asio

# Windows (with vcpkg)
vcpkg install asio
```

### Building and Installing the Library

```bash
# Clone the repository
git clone https://github.com/HILLAB-Software/libagilispiezo-cpp.git
cd libagilispiezo-cpp

# Create and navigate to build directory
mkdir -p build
cd build

# Configure, build and install
cmake ..
cmake --build .
sudo cmake --install .    # On Windows, remove 'sudo'
```

## Usage

### Building the Examples

Examples are included in the `examples` directory and are automatically built when you build the library:

```bash
# After building the library
cd build
./examples/basic_example /dev/ttyUSB0  # Replace with your device port
```

### Using the Library in Your Project

#### With CMake

Create a `CMakeLists.txt` file in your project:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp)

# Find the AgilisPiezo package
find_package(agilispiezo REQUIRED)

# Add your executable
add_executable(myapp main.cpp)

# Link with the AgilisPiezo library
target_link_libraries(myapp PRIVATE agilispiezo::agilispiezo)
```

#### Example Code

```cpp
#include <libagilispiezo/agilispiezo.h>
#include <iostream>

int main() {
  agilispiezo::AgilisPiezo piezo;
  
  // Set log level
  piezo.SetLogLevel(agilispiezo::AgilisPiezo::LOG_INFO);
  
  // Connect to the device
  if (!piezo.ConnectDeviceUSB("/dev/ttyUSB0")) {
    std::cerr << "Failed to connect to device" << std::endl;
    return 1;
  }
  
  // Set to remote mode
  piezo.SetToRemoteMode();
  
  // Get device information
  std::string firmware;
  piezo.GetControllerFirmwareVersion(&firmware);
  std::cout << "Firmware version: " << firmware << std::endl;
  
  // Move axis 1 by 10 steps in positive direction
  piezo.RelativeMove(1, true, 10);
  
  // Disconnect from the device
  piezo.DisconnectDevice();
  
  return 0;
}
```

### Building Your Application

```bash
# Create a build directory
mkdir -p build
cd build

# Configure and build
cmake ..
cmake --build .

# Run your application
./myapp
```

## Troubleshooting

### Common Issues

#### Cannot Find Package

If CMake cannot find the agilispiezo package, you may need to specify the path:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/installation ..
```

#### Device Not Found
- Ensure the device is properly connected
- Check that the user has access to the serial port
  - On Linux: Add user to the dialout group: `sudo usermod -a -G dialout $USER`
  - On Windows: Check port permissions in Device Manager
- Use `lsusb` on Linux to verify the FTDI device is detected
- Check if ftdi_sio kernel module is loaded with `lsmod | grep ftdi_sio`

#### Communication Errors
- Check for serial port permission issues
- Ensure the correct port name is used
- Try different baud rates if communication fails

## API Documentation

### Main Classes

#### AgilisPiezo

This class provides access to all Agilis Piezo controller functionalities.

Key methods:
- `ConnectDeviceUSB(port_name)` - Connect to device via USB
- `ConnectDeviceRS232(port_name)` - Connect to device via RS232
- `DisconnectDevice()` - Disconnect from device
- `IsConnected()` - Check connection status
- `SetToRemoteMode()` - Set controller to remote mode
- `RelativeMove(axis, sign, steps)` - Move axis by specified steps
- `AbsoluteMove(axis, position)` - Move to absolute position
- `GetAxisStatus(axis, out_status)` - Get axis status
- `StopMotion(axis)` - Stop motion on specified axis

#### Serial

The Serial class handles low-level communication with the device.

### Enum Types

#### LogLevel
- `LOG_DEBUG` - Detailed debug information
- `LOG_INFO` - General information
- `LOG_WARNING` - Warnings
- `LOG_ERROR` - Errors
- `LOG_NONE` - Disable logging

#### JogSpeed
- `JOGSPEED_0` - Stop
- `JOGSPEED_5` - 5 steps/s at defined step amplitude
- `JOGSPEED_100` - 100 steps/s at maximum step amplitude
- `JOGSPEED_1700` - 1700 steps/s at maximum step amplitude
- `JOGSPEED_666` - 666 steps/s at defined step amplitude

#### AxisStatus
- `AXISSTATUS_READY` - Ready (not moving)
- `AXISSTATUS_STEPPING` - Currently executing a PR command
- `AXISSTATUS_JOGGING` - Currently executing a JA command
- `AXISSTATUS_MOVINGTOLIMIT` - Currently executing MV, MA, PA commands

## License

This project is licensed under the GPL v3.0 License - see the LICENSE file for details.

## Acknowledgments

- Newport Agilis AG-UC2 and AG-UC8 Piezo Controllers documentation
- Asio library for cross-platform serial communication