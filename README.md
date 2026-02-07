# Build (Windows)

### 1. Install vcpkg

```bat
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

### 2. Install dependencies

```bat
vcpkg install glfw3 glm assimp
```

(Optional but recommended)

```bat
vcpkg integrate install
```

---

### 3. CLion / Visual Studio settings

* **Toolchain:** MSVC
* **CMake generator:** Ninja
* **Build type:** `Release` or `RelWithDebInfo` (don’t use Debug)

If not using `vcpkg integrate install`, set:

```
CMAKE_TOOLCHAIN_FILE = <vcpkg-root>/scripts/buildsystems/vcpkg.cmake
```

---

### 4. Build

Open the project in CLion and build normally.

First build downloads deps, after that builds are fast.

---
