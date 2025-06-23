# UDP-Webrtc-Demo ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

**UDP-Webrtc-Demo** is a small project designed to explore and learn the fundamentals of webrtc with c++

## Getting Started

Follow the steps below to build and run **UDP-Webrtc-Demo**.

### Prerequisites

- **CMake** (version 3.16 or later recommended)
- **C++ 23-compatible compiler**
- **x264 and gtk installed**

### Building the Project

To build **UDP-Webrtc-Demo** in Debug mode, follow these steps

1. **Configure the build**:
   ```bash
   cmake -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug -H.
   ```
2. **Compile the project** (replace 1 with the number of threads you want to use for a faster build)::
   ```bash
    cmake --build build -j1
   ```

### Run Sender

```bash
.\build\wrt_receiver
```

### Run Receiver

```bash
.\build\wrt_sender --endpoint "localhost" --port 8080
```