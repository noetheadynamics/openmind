# OpenMind Build Guide

## Prerequisites

### For Native Build (Testing)
- **C++17 compiler:** GCC 9+, Clang 10+, or MSVC 19.20+
- **Make** (Linux/macOS) or **Visual Studio Build Tools** (Windows)

### For WASM Build (Web)
- **Emscripten SDK** (emsdk)
- **Python 3** (for HTTP server)

### For Development
- **Git**
- A modern web browser

---

## Native Build (Linux/macOS)

### Quick Build
```bash
# Clone the repository
git clone https://github.com/noetheadynamics/openmind.git
cd openmind

# Build all tests
bash build/build_native.sh

# Run all tests
bash build/run_tests.sh
```

### Manual Build
```bash
# Compile a single test
g++ -std=c++17 -O2 \
    tests/test_lighting.cpp \
    src/engine/VoxelOctree.cpp \
    src/engine/PhysicsEngine.cpp \
    src/engine/openmind_engine.cpp \
    src/llm/JSONValidator.cpp \
    -Isrc/engine -Isrc/llm -Isrc/agents \
    -o build/native/test_lighting \
    -lpthread

# Run it
./build/native/test_lighting
```

---

## Native Build (Windows)

### Using BuildTools
```batch
REM Open Developer Command Prompt
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

REM Compile
cl /EHsc /std:c++17 /O2 ^
    tests\test_lighting.cpp ^
    src\engine\VoxelOctree.cpp ^
    src\engine\PhysicsEngine.cpp ^
    src\engine\openmind_engine.cpp ^
    src\llm\JSONValidator.cpp ^
    /Isrc\engine /Isrc\llm /Isrc\agents ^
    /Fe:build\native\test_lighting.exe

REM Run
build\native\test_lighting.exe
```

### Using the batch file
```batch
build.bat
```

---

## WASM Build

### Install Emscripten
```bash
# Clone emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest
./emsdk install latest
./emsdk activate latest

# Source environment
source ./emsdk_env.sh
```

### Build
```bash
# Run the WASM build script
bash build/build_wasm.sh

# Output files in build/wasm/
# - openmind.js (JavaScript glue)
# - openmind.wasm (WebAssembly binary)
```

### Test Locally
```bash
# Copy to dist
bash build/build_web.sh

# Start HTTP server
python -m http.server 8080 -d build/dist

# Open browser
# http://localhost:8080
```

---

## Web Build

### Build for Deployment
```bash
# Build web assets
bash build/build_web.sh

# Output in build/dist/
# - index.html
# - omni_console.css
# - omni_console.js
# - ui_connection.js
# - openmind.js (if WASM built)
# - openmind.wasm (if WASM built)
```

### Deploy
```bash
# Option 1: Local server
python -m http.server 8080 -d build/dist

# Option 2: Netlify (drag & drop)
# Upload build/dist/ to https://app.netlify.com/drop

# Option 3: Vercel
npx vercel build/dist

# Option 4: Cloudflare Pages
npx wrangler pages deploy build/dist --project-name=openmind
```

---

## Project Structure

```
openmind/
├── src/
│   ├── engine/           # Core engine (57 features)
│   ├── llm/              # LLM Connector (5 providers)
│   ├── agents/           # Agent Cognitive Layer
│   ├── ui/               # Omni-Console Frontend
│   └── bridge/           # WASM Bridge
├── tests/                # All test files
├── docs/                 # Documentation
├── build/                # Build scripts + output
├── demo_world.json       # Demo world
├── package.json          # Project config
└── README.md             # Project overview
```

---

## Compiler Flags

### Recommended Optimization Levels

| Level | Flag | Use Case |
|-------|------|----------|
| Debug | `-g -O0` | Development, debugging |
| Release | `-O2` | Native testing |
| Max | `-O3 -flto` | Production WASM |
| Size | `-Os` | WASM size-constrained |

### Required Flags

| Flag | Purpose |
|------|---------|
| `-std=c++17` | C++17 standard |
| `-lpthread` | POSIX threads (native only) |
| `-s ALLOW_MEMORY_GROWTH=1` | WASM memory growth |
| `-s WASM=1` | Enable WebAssembly |

---

## Troubleshooting

### "emcc not found"
Install Emscripten SDK and source the environment:
```bash
source ~/emsdk/emsdk_env.sh
```

### "Module not found"
Ensure all source files are in the correct directories:
```bash
ls src/engine/VoxelOctree.cpp
ls src/llm/JSONValidator.cpp
ls src/bridge/openmind_bridge.cpp
```

### WASM load timeout
The WASM module may be too large. Try:
- Reducing `INITIAL_MEMORY` in build_wasm.sh
- Using `-Os` instead of `-O3`
- Checking browser console for errors

### Test failures
Ensure all files compile:
```bash
bash build/build_native.sh
bash build/run_tests.sh
```

---

## Continuous Integration

### GitHub Actions Example
```yaml
name: Build and Test
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: bash build/build_native.sh
      - name: Test
        run: bash build/run_tests.sh
```

---

## Performance Tips

1. **Use `-O2` for testing** — fast enough, doesn't obscure errors
2. **Use `-O3` for WASM** — maximum runtime performance
3. **Use `-Os` for size** — smaller WASM download
4. **Profile with `gprof`** — native performance analysis
5. **Use Chrome DevTools** — WASM debugging in browser
