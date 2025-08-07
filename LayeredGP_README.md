# 平面分層全域佈局 (Planar Layered Global Placement) 使用說明

## 概述

平面分層全域佈局功能將3D集成電路中的不同層級視為獨立的2D佈局問題進行交替最佳化。

## 主要特性

1. **完全2D化**: 每層都視為獨立的2D佈局問題
2. **完全凍結**: 非最佳化層完全凍結（XYZ全部固定）
3. **純平面最佳化**: 只在XY平面內進行最佳化，Z軸完全固定
4. **交替最佳化**: 奇數迭代最佳化上層，偶數迭代最佳化下層
5. **更高效率**: 避免不必要的3D計算開銷

## 使用方法

### 基本用法

```bash
# 平面分層GP
./eplace -aux your_design.aux -3DIC -layeredGP

# 自定義迭代次數
./eplace -aux your_design.aux -3DIC -layeredGP -layeredIters 15

# 完整範例
./eplace -aux testcase/aes_cipher_top/aes_cipher_top.aux -3DIC -layeredGP -layeredIters 10 -targetDensity 0.8
```

## 工作流程

1. **3D初始化**: 執行標準的3D初始化
2. **標準mGP**: 執行標準的全域佈局
3. **Z-fixed mGP**: 將節點分離到各層並固定Z座標
4. **平面分層GP**: 
   - 迭代1: 最佳化上層（下層完全凍結）
   - 迭代2: 最佳化下層（上層完全凍結）
   - 迭代3: 最佳化上層（下層完全凍結）
   - ...持續交替

## 參數說明

- `-3DIC`: 啟用3D模式
- `-layeredGP`: 啟用平面分層GP
- `-layeredIters <數字>`: 設定分層迭代次數（預設10次）
- `-targetDensity <數字>`: 設定目標密度（預設0.8）
- `-fullPlot`: 啟用每次迭代的圖形輸出

## 技術細節

### 梯度控制策略

**平面方法的梯度控制**：
```cpp
// 所有模組的Z軸梯度都設為0
totalGradient[index].z = 0;

// 非最佳化層的XY梯度也設為0（完全凍結）
if (currentOptimizingLayer == 1 && isBottomLayerModule) {
    // 最佳化頂層時，完全凍結底層
    totalGradient[index].x = 0;
    totalGradient[index].y = 0;
    totalGradient[index].z = 0;
}
```

### 層級分離邏輯

- **底層範圍**: Z ∈ [0, layerThickness]
- **頂層範圍**: Z ∈ [layerThickness, 2×layerThickness]
- **分離閾值**: Z = layerThickness

### 最佳化策略

1. **奇數迭代（最佳化頂層）**:
   - 頂層模組: 允許XY移動，Z固定
   - 底層模組: XYZ全部凍結

2. **偶數迭代（最佳化底層）**:
   - 底層模組: 允許XY移動，Z固定  
   - 頂層模組: XYZ全部凍結

## 優點

1. **概念清晰**: 每層都是純粹的2D問題
2. **避免層間干擾**: 非最佳化層完全不移動
3. **計算效率**: 減少不必要的3D計算
4. **物理合理**: 符合實際製造約束

## 範例輸出

```
=== Planar Layered GP Mode Activated ===
Using 2D planar placement approach for each layer
Top layer modules: 9286
Bottom layer modules: 9287

--- Layered Iteration 1 ---
=== Optimizing TOP layer using 2D planar approach ===
Bottom layer frozen, optimizing 9286 top layer modules in 2D

--- Layered Iteration 2 ---  
=== Optimizing BOTTOM layer using 2D planar approach ===
Top layer frozen, optimizing 9287 bottom layer modules in 2D
``` 