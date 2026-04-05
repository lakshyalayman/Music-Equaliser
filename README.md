# 🎧 Raylib Audio Visualizer

A real-time audio visualizer and DSP playground written in C using raylib, hot-reloadable plugins, FFT, and interactive audio filters (EQ, LPF, HPF, etc.).

---

## 🚀 Features

- **Hot Reloading (dlopen/dlsym)**
  - Modify DSP logic without restarting
  - State preserved across reloads

- **Real-Time Audio Processing**
  - Custom audio stream callbacks
  - Filters:
    - Low-pass / High-pass
    - Bass Boost / Treble Boost
    - Mid clarity enhancement
    - Stereo panning
    - Full EQ mode

- **FFT-Based Visualization**
  - Custom recursive FFT implementation
  - Log-scaled frequency bins
  - Smoothed spectrum output
  - Multiple visualization modes:
    - Multi-wave (default)
    - Sine wave
    - Bar spectrum

- **Drag & Drop Audio**
  - Drop `.mp3` file directly into window

- **Interactive Controls**
  - Live tuning of EQ parameters
  - Volume + playback control

---

## 🧠 Architecture Overview

```
main.c
 ├── Dynamic loader (dlopen)
 ├── Plugin symbol binding (X-macros)
 └── Hot reload trigger (R key)

plug.so
 ├── Audio processing (callbacks)
 ├── DSP filters
 ├── FFT + visualization
 └── Input handling
```

---

## 🔌 Plugin System

Defined via X-macro in `plug.h`:

```c
#define LIST_OF_PLUGS \
  PLUG(plug_init, void, void) \
  PLUG(plug_update, void, void) \
  PLUG(plug_pre_reload, void *, void) \
  PLUG(plug_post_reload, void, void *) \
  PLUG(plug_unload_stream, void, void) \
  PLUG(plug_src_init, void, const char *src) \
  PLUG(plug_key_functions, void, void) \
  PLUG(plug_eq_reset, void, void)
```

Enables dynamic binding of plugin functions at runtime.

---

## 🛠️ Build Instructions

### Dependencies

- `raylib`
- `gcc` or `clang`
- Linux (uses `dlopen`)

### Install raylib

**Arch Linux**
```bash
sudo pacman -S raylib
```
By default pacman installs the dynamic version of raylib (if installing from source change the 
CMake file)

**Ubuntu / Debian**
```bash
sudo apt install libraylib-dev
```

---

### Build Plugin (`libplug.so`)
Or just `./run.sh`
```bash
gcc -shared -fPIC plug.c -o libplug.so -lraylib -lm
```

---

### Build Main Application

```bash
gcc main.c -o visualizer -ldl -lraylib -lm
```

---

### Run

```bash
./visualizer
```

---

## 🎮 Controls

### General

| Key | Action |
|-----|--------|
| R | Hot reload plugin |
| Q | Quit |
| SPACE | Play / Pause |
| ↑ / ↓ | Volume |

---

### EQ Controls

| Key | Action |
|-----|--------|
| Z / X | Bass - / + |
| C / V | Mid - / + |
| B / N | Treble - / + |

---

### Filter Modes

| Key | Mode |
|-----|------|
| , | Low Pass |
| . | Normal |
| / | High Pass |
| P | Panning |
| E | Bass Boost |
| T | Treble Boost |
| A | Clarity |

---

## 🔬 DSP Highlights

### FFT

- Recursive Cooley-Tuk FFT
- Size: `16384`
- Hann window applied
- Log-frequency binning

### Filters

- **LPF / HPF**: single-pole filters  
- **Bass Boost**: enhances low frequencies  
- **Treble Boost**: enhances high frequencies  
- **Clarity Mode**: mid-band enhancement  
- **EQ Mode**: independent bass/mid/treble control  

---

## 🔁 Hot Reloading Flow

1. Press `R`  
2. Save state via `plug_pre_reload()`  
3. Reload `.so` using `dlopen`  
4. Restore state via `plug_post_reload()`  

No restart required.

---

## 📂 Project Structure

```
.
├── run.sh
├── libplug.so
├── musicEqualiser
├── README.md
└── src
     ├── main.c
     ├── plug.c
     ├── fft.c
     └── plug.h

```

---

## ⚠️ Notes

- Linux only (uses POSIX `dlopen`)  
- FFT size is large → CPU intensive - (2<sup>14</sup> samples)


---

