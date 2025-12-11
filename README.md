# 3D-DC: Volumetric Entropy Reduction Algorithm

![C++](https://img.shields.io/badge/C++-17%2F20-blue.svg)
![Status](https://img.shields.io/badge/Status-Research%20Prototype-orange.svg)
![Field](https://img.shields.io/badge/Field-Data_Compression-purple.svg)
![License](https://img.shields.io/badge/License-BSD_3--Clause-green.svg)

## 🔬 Abstract

**3D-DC** is a experimental research project exploring a novel approach to lossless data compression. Unlike traditional compressors (LZ77, Huffman) that treat files as linear streams, 3D-DC transforms binary data into **3D volumetric structures** (voxel grid).

The core hypothesis is that latent **topological correlations** exist not just sequentially, but spatially and "temporally" (depth-wise) within binary data. By projecting data into a 3D manifold, this algorithm utilizes geometric predictors and probability distribution sorting to significantly **reduce Shannon Entropy**, preparing the data for highly efficient final encoding.

## 💡 The Paradigm Shift

Traditional compression algorithms perceive data as a **1-Dimensional stream** of bytes, looking for repetitions over a linear sliding window. **3D-DC challenges this limitation** by treating a file not as a line, but as a **volume**.

```text
Linear View (Traditional):
[00][A1][B2][00][A1][B2]... (Match Distance = 3)

Volumetric View (3D-DC):
Layer Z=0      Layer Z=1
[00][A1]       [00][A1]
[B2][..]       [B2][..]
   ^              ^
   |______________|
    Z-Correlation (Distance = 1)
```

By stacking data into layers (Matrices along the Z-axis), we can exploit correlations on three axes simultaneously.

## ⚙️ The Core Pipeline

The algorithm defines a transformation sequence $\mathcal{T}$ applied to a discrete input signal $S$. The objective is to minimize the Shannon entropy of the output residual $R$, such that $H(R) \ll H(S)$.

### 1. Volumetric Injection (Tensor Mapping)

Let $S = \{x_0, x_1, \dots, x_{L-1}\}$ be a sequence where $x_i \in \mathbb{F}_{2^8}$.
We define a bijective mapping $\Phi : \mathbb{N} \to \mathbb{N}^3$ projecting the linear index $i$ into a cubic lattice $(k, u, v)$ with matrix order $N$:

$$
\Phi(i) = \left( \lfloor \frac{i}{N^2} \rfloor, \quad \lfloor \frac{i \pmod{N^2}}{N} \rfloor, \quad i \pmod N \right)
$$

The data is restructured into a 3rd-order tensor 

$$\mathbf{T} \in \mathcal{M}_{D,N,N}(\mathbb{F}_{2^8}),$$ 

where 

$$D = \lceil L/N^2 \rceil.$$

### 2. Manifold Optimization (Statistical Alignment)

To maximize inter-layer correlation, we treat each layer $\mathbf{T}_k$ as a stochastic process with PMF $P_k$.
We define a divergence metric $d(\mathbf{T}_i, \mathbf{T}_j)$ using the **Bhattacharyya coefficient** to quantify the statistical overlap:

$$ d(\mathbf{T}_i, \mathbf{T}_j) = - \ln \left( \sum_{n=0}^{255} \sqrt{P_i(n) \cdot P_j(n)} \right) $$

The algorithm approximates the solution to a Hamiltonian Path problem to find a permutation $\sigma$ of $\{0, \dots, D-1\}$ minimizing total Z-axis divergence:

$$ \min_{\sigma} \sum_{k=0}^{D-2} d(\mathbf{T}_{\sigma(k)}, \mathbf{T}_{\sigma(k+1)}) $$

### 3. Discrete Differential Operators (Predictors)

We define a set of differential operators $\mathcal{D} = \{ \nabla_{id}, \nabla_{sub}, \nabla_{up}, \nabla_{paeth} \}$ operating on the voxel $v_{k,u,v}$ and its causal neighborhood $\mathcal{N}_{k,u,v}$. The 3D predictors utilize the aligned previous layer $k-1$:

* **Identity:** $\nabla_{id}(v) = v_{k,u,v}$
* **Sub (Gradient X):** $\nabla_{sub}(v) = v_{k,u,v} - v_{k,u, v-1}$
* **Up (Gradient Z):** $\nabla_{up}(v) = v_{k,u,v} - v_{k-1,u,v}$
* **Paeth-3D (Planar Estimation):**
    Let $a = v_{k,u,v-1}$ (Left), $b = v_{k-1,u,v}$ (Back), $c = v_{k-1,u,v-1}$ (Back-Left).

    $$\nabla_{paeth}(v) = v_{k,u,v} - \text{PaethPredictor}(a, b, c)$$

    Where $\text{PaethPredictor}$ selects the neighbor closest to $p = a + b - c$.

### 4. Adaptive Kernel Selection

For each voxel (or row vector), the algorithm selects the optimal operator $\delta_{opt} \in \mathcal{D}$ that minimizes the local residual magnitude. The final residual tensor $\mathbf{R}$ is computed in $\mathbb{F}_{2^8}$:

$$ \mathbf{R}_{k,u,v} = (\delta_{opt}(v_{k,u,v})) \pmod{256} $$

### 5. Objective Function (Entropy Minimization)

The efficiency of the transform is measured by the reduction of the zero-order empirical Shannon entropy:

$$ \text{Gain} = H(S) - H(\mathbf{R}) $$

$$ H(\mathbf{X}) = - \sum_{i=0}^{255} p(x_i) \log_2 p(x_i) $$

Where $p(x_i)$ is the probability of symbol $i$ occurring in the tensor. The hypothesis is that the distribution of $\mathbf{R}$ follows a Laplacian distribution centered at 0 (peaked), whereas $S$ follows a uniform distribution.

### 6. Generalization to High-Dimensional Spaces

The 3D approach used in this prototype is a specific case ($d=3$) of a generalized mapping to an $n$-dimensional Hyper-Torus.
Ideally, the index mapping function $\Phi_d(i)$ for a dimension $d$ corresponds to the decomposition of the index $i$ in base $N$:

$$ c_k = \left\lfloor \frac{i}{N^k} \right\rfloor \pmod N $$

This implies that the projection logic is effectively equivalent to writing the linear index modulo powers of the base ($256^k$).

* **1D:** $i \pmod{256^1}$ (Stream)
* **2D:** $i \pmod{256^2}$ (Image/Matrix)
* **3D:** $i \pmod{256^3}$ (Volumetric/3D-DC)

## 🛠️ Software Architecture

The project is built around three core C++ components:

1. **`DCBuffer`**: A flexible I/O handler that manages data streaming from files or memory blocks using smart pointers and RAII.
2. **`DCQueue` (The Volume)**: The central data structure acting as a "cube" of matrices. It handles the 2D/3D filtering logic and manages the filter dictionary.
3. **`DCMap` (The Geometry)**: A geometric overlay providing an orthonormal Cartesian system to navigate the `DCQueue` using coordinate mathematics without duplicating memory.

## 📦 Installation & Usage

### Prerequisites

* A C++ Compiler supporting C++17 or later (GCC, Clang, MSVC).
* Standard Template Library (STL).

### Compilation

You can compile the project using `make` directly:

```bash
make
```

## ⚠️ Current Status & Limitations

This project is an **experimental proof-of-concept** and remains **incomplete**.

* **Experimental Nature:** The primary goal is to validate the mathematical hypothesis (Entropy Reduction via 3D projection). As such, the code prioritizes flexibility and analysis tools over raw performance or memory efficiency.
* **Work in Progress:** While the core logic for entropy reduction is functional, the full pipeline (including the final bitstream serialization and the decoding step) is not fully polished.
* **Memory Overhead:** The current implementation uses Jagged Arrays (`std::vector<std::vector>`) for experimental flexibility, which incurs CPU cache misses compared to a flat-array implementation.

## ✨ Future Work Focus

The `DCMap` abstraction extends beyond simple coordinate mapping; it establishes an **isometric embedding** of the linear storage space $\mathcal{L}$ into a discrete Euclidean metric space $\mathcal{E}^3$.

$$ \Psi : \mathcal{L} \to \mathbb{Z}^3 \subset \mathbb{R}^3 $$

This mathematical decoupling allows us to substitute the scalar memory pointer increment ($p \leftarrow p + 1$) with a continuous vector field traversal. We define a sampling trajectory $\gamma(t)$ within the continuous volume:

$$ \gamma : [0, 1] \to \mathbb{R}^3, \quad \gamma(t) = \mathbf{P}_0 + t \cdot \vec{v} $$

Since the dataset is discrete, we apply a **discretization operator** $\mathcal{D}$ (implemented via **Bresenham’s algorithm** or **Ray-Marching**) to map the continuous manifold back to valid memory indices:

$$ S_{sampled} = \{ \Phi^{-1}(\mathcal{D}(\gamma(t))) \mid t \in [0, 1] \} $$

This formalism enables **Anisotropic Correlation Extraction**. Instead of being constrained to isotropic linear reads, the engine can extract correlations along arbitrary correlation vectors $\vec{v}$ (e.g., spectral diagonals, helical scans) to detect hidden structural periodicities orthogonal to the primary storage axis.


## 📄 License

This project is licensed under the **BSD 3-Clause License**. See the `LICENSE` file for details.