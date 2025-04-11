# TxtLogParser

Welcome to **TxtLogParser**, an innovative log parsing tool designed to simplify log analysis. This project is currently in its early stages, and I’m excited to share it with the community!

## Features
- Fast and lightweight log parsing
- Customizable filters
- Cross-platform support

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/paneltree/TxtLogParser.git
   ```
2. Install dependencies:
   ```bash
   [安装命令，例如 npm install 或 pip install -r requirements.txt]
   ```
3. Run the app:
   ```bash
   [运行命令，例如 npm start 或 python main.py]
   ```

## Build

### MacOS

* Prerequisites: Xcode, CMake, Qt6
* Build steps:
   ```bash
   git clone https://github.com/paneltree/TxtLogParser.git
   cd TxtLogParser
   mkdir build
   cd build
   cmake .. -DQT_DIR=~/Qt/6.8.3
   make
   ```

### Linux

* Prerequisites: CMake, Qt6, gcc
* Build Steps:
   ```bash
   git clone https://github.com/paneltree/TxtLogParser.git
   cd TxtLogParser
   mkdir build
   cd build
   cmake .. -DQT_DIR=~/Qt/6.8.3
   make
   ```

### Windows

* Prerequisites: CMake, Qt6, Visual Studio
* Build Steps:
   ```cmd
   git clone https://github.com/paneltree/TxtLogParser.git
   cd TxtLogParser
   mkdir build
   cd build
   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
   cmake .. -G "Visual Studio 17 2022" -DQT_DIR=C:\Qt\6.8.2
   cmake --build . --config Release
   ```
* For NMake:
   ```cmd
   git clone https://github.com/paneltree/TxtLogParser.git
   cd TxtLogParser
   mkdir build
   cd build
   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
   cmake .. -G "NMake Makefiles" -DQT_DIR=C:\Qt\6.8.2
   nmake
   ```


## Usage
[简要说明如何使用，例如 "Run `python main.py --file log.txt` to parse a log file."]

## Contributing
I welcome contributions from the community! Please check out the [CONTRIBUTING.md](CONTRIBUTING.md) file for guidelines. By contributing, you agree that your contributions may be used in future versions, including potential commercial releases.

## License
This project is licensed under the [MIT License](LICENSE). You’re free to use, modify, and distribute it as per the terms of the license. As the copyright owner, I reserve the right to release future versions of this project under different licenses, including proprietary ones for commercial purposes.

## Future Plans
TxtLogParser is currently open-source and free to use. In the future, I may introduce additional features or services, some of which might be offered under a paid, proprietary license. Stay tuned for updates!

## Contact
Feel free to reach out via paneltree@outlook.com or open an issue on GitHub if you have questions, suggestions, or feedback.

Happy coding!
Deli