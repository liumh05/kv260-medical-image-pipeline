# Marching Cubes C++

一个纯C++实现的 Marching Cubes 项目，输入为形状为 (1, D, H, W) 的 NPY 文件（C-order，dtype 支持 float32/int16/int32/uint8），输出 VTK legacy PolyData (.vtk)。

## 构建

```bash
mkdir build && cd build
cmake ..
cmake --build . -j
```

## 运行

```bash
./marching_cubes_c --input /path/to/ct.npy --iso 300.0 --vtk out.vtk
```

参数说明：
- `--input`: NPY 文件路径，形状必须为 `(1, D, H, W)`
- `--iso`: 等值（浮点数），例如CT可选 300.0（根据你的窗宽窗位或分割阈值调整）
- `--vtk`: 输出 VTK legacy PolyData 文件路径（必选）

## 说明
- 体素坐标采用单位间距，点坐标即为体素格点索引。
- 如需各向异性体素间距，可在 `marching_cubes.cpp` 内对插值坐标乘以 spacing。
- 若NPY为 Fortran-order 或不受支持的 dtype，将报错。

## 依赖
- 无第三方库依赖。

## 注意
- triTable 需保证完整，否则算法不完整。当前代码包含完整的 edgeTable 和 triTable。

