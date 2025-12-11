# 3D-DC: Volumetric Entropy Reduction Algorithm

![C++](https://img.shields.io/badge/C++-17%2F20-blue.svg)
![Status](https://img.shields.io/badge/Status-Research%20Prototype-orange.svg)
![License](https://img.shields.io/badge/License-BSD3-green.svg)
<!-- [![wakatime](https://wakatime.com/badge/user/86ddae99-e534-418e-b63d-b835643b604e/project/b44d7730-7132-4fec-a4d8-a7e91b8d7797.svg)](https://wakatime.com/badge/user/86ddae99-e534-418e-b63d-b835643b604e/project/b44d7730-7132-4fec-a4d8-a7e91b8d7797) -->

## 🔬 Abstract

**3D-DC** is a research project exploring a novel approach to data compression. Unlike traditional linear compressors (like LZ77 or Huffman), 3D-DC transforms 1D binary streams into **3D volumetric structures** (cubes of matrices).

The core hypothesis is that latent correlations exist not just sequentially, but spatially and "temporally" (depth-wise) within binary data. By projecting data into 3D space, this algorithm utilizes geometric predictors and probability distribution sorting to significantly **reduce Shannon Entropy**, preparing the data for highly efficient final compression.

## 🚀 Key Features

*   **Volumetric Projection:** Turns flat files into `[Matrix, Row, Column]` coordinates.
*   **Hybrid Filtering Pipeline:**
    *   **2D Filters:** Implementation of linear filters (Sub, Up, Average, Paeth, etc) optimized via multithreading.
    *   **3D Filters:** Novel depth-based predictors using Z-axis neighbors to predict current voxel values.
*   **Intelligent Sorting:** Utilizes **Bhattacharyya Distance** to calculate similarity between matrix probability distributions, reordering the queue to maximize Z-axis correlation.
*   **Geometric Abstraction:** Features a `DCMap` layer allowing mathematical traversal (Bresenham's algorithm) and coordinate system transformations (Local vs. User space) over raw memory.
*   **Entropy Analysis:** Built-in tools to measure entropy and standard deviation evolution at each stage of the pipeline.

## 🛠️ Architecture

The project is built around three core components:

1.  **`DCBuffer`**: A flexible I/O handler that manages data streaming from files or memory blocks using smart pointers and RAII.
2.  **`DCQueue`**: The central data structure acting as a "window" or "cube" of matrices. It handles the 2D/3D filtering logic and manages the filter dictionary.
3.  **`DCMap`**: A geometric overlay providing an orthonormal Cartesian system $(O, I, J, K)$ to navigate the `DCQueue` using coordinate mathematics.

## 📦 Installation & Build

### Prerequisites
*   A C++ Compiler supporting C++17 or later (GCC, Clang, MSVC).
*   Standard Template Library (STL).

### Compilation
You can compile the project using `g++` directly:

```bash
g++ -std=c++17 -O3 src/*.cpp src/DCompress/*.cpp -o 3d-dc
```

## 🧠 Algorithmic Logic

1.  **Buffering:** Data is read in chunks via `DCBuffer`.
2.  **Construction:** `DCQueue` builds a sequence of matrices.
3.  **Sorting (Optional):** Matrices are reordered based on statistical similarity (Bhattacharyya distance) to group similar data patterns together.
4.  **3D Filtering:** The algorithm iterates through the volume, selecting the best predictor (e.g., `PAETH_3D`) for each voxel based on its neighbors in $(x, y, z-1)$.
5.  **2D Filtering:** Residuals are processed via standard 2D filters.
6.  **Entropy Calculation:** The resulting entropy is computed to measure theoretical compression gain.

## ⚠️ Current Status

This is a **Research Prototype**.
*   **Performance:** The code prioritizes algorithmic correctness and readability over raw execution speed. It uses heavy STL containers (`std::vector<std::vector>`) which may result in memory overhead.
*   **Goal:** The primary metric is **entropy reduction**, not execution time.

## 📄 License

This project is open-source. Please refer to the source code for specific usage rights.