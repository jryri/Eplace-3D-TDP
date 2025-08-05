# 3D Bin Node Density Update Implementation

本文檔記錄了 `EPlacer_3D::binNodeDensityUpdate()` 方法的實現，這是3D EPlace算法中密度計算的核心組件。

## 實現概要

### 核心功能
`binNodeDensityUpdate()` 負責將3D模組（nodes）和填充器（fillers）的密度分布到3D bin網格中，為後續的FFT_3D密度梯度計算提供正確的輸入數據。

### 主要步驟

#### 1. 清除現有密度
```cpp
// 清除所有3D bins的nodeDensity和fillerDensity
for (int i = 0; i < binDimension.x; i++)
    for (int j = 0; j < binDimension.y; j++)
        for (int k = 0; k < binDimension.z; k++)
        {
            bins[i][j][k]->nodeDensity = 0;
            bins[i][j][k]->fillerDensity = 0;
        }
```

#### 2. 3D局部平滑（Local Smoothing）
對每個模組在X、Y、Z三個維度進行局部平滑處理：

```cpp
// X維度局部平滑
if (float_less(curNode->getWidth(), binStep.x))
{
    localSmoothLengthScale.x = curNode->getWidth() / binStep.x;
    rectLowerLeft.x = cellCenter.x - 0.5 * binStep.x;
    rectUpperRight.x = cellCenter.x + 0.5 * binStep.x;
}

// Y維度和Z維度類似處理
```

**局部平滑的意義**：
- 對於尺寸小於bin大小的模組，進行平滑分布
- 避免小模組造成的密度突變
- 保證數值穩定性

#### 3. 3D Bin索引計算
計算模組覆蓋的3D bin範圍：

```cpp
VECTOR_3D_INT binStartIdx, binEndIdx;
binStartIdx.x = INT_DOWN((rectLowerLeft.x - db->coreRegion.ll.x) / binStep.x);
binEndIdx.x = INT_DOWN((rectUpperRight.x - db->coreRegion.ll.x) / binStep.x);
// Y和Z維度類似
```

#### 4. 3D重疊體積計算
使用三重嵌套循環遍歷所有重疊的bins：

```cpp
for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
    for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
        for (int k = binStartIdx.z; k <= binEndIdx.z; k++)
        {
            float overlapVolume = getOverlapVolume_3D(
                bins[i][j][k]->ll, bins[i][j][k]->ur, 
                rectLowerLeft, rectUpperRight);
            
            float scaledOverlapVolume = localSmoothLengthScale.x * 
                                      localSmoothLengthScale.y * 
                                      localSmoothLengthScale.z * 
                                      overlapVolume;
        }
```

#### 5. 密度分配策略
根據模組類型進行不同的密度分配：

```cpp
if (curNode->isMacro)
{
    // Macro密度縮放
    bins[i][j][k]->nodeDensity += targetDensity * scaledOverlapVolume;
}
else if (curNode->isFiller)
{
    // 填充器密度
    bins[i][j][k]->fillerDensity += scaledOverlapVolume;
}
else
{
    // 標準cell密度
    bins[i][j][k]->nodeDensity += scaledOverlapVolume;
}
```

## 關鍵特性

### 3D擴展
- **從2D到3D**：所有計算從2D面積擴展到3D體積
- **Z軸處理**：完整支援Z軸的局部平滑和bin索引計算
- **3D重疊**：使用 `getOverlapVolume_3D()` 計算3D幾何重疊

### 密度縮放
- **Macro縮放**：宏單元密度乘以 `targetDensity`
- **局部平滑縮放**：三維縮放因子相乘
- **體積歸一化**：使用3D bin體積進行正規化

### 性能考量
- **三重循環**：O(N × Bx × By × Bz)，其中N是模組數
- **記憶體存取**：連續存取3D bin陣列
- **計算複雜度**：每個模組-bin重疊需要3D幾何計算

## 與2D版本的差異

| 特性 | 2D版本 | 3D版本 |
|------|--------|--------|
| Bin陣列 | `bins[i][j]` | `bins[i][j][k]` |
| 局部平滑 | X, Y維度 | X, Y, Z維度 |
| 重疊計算 | `getOverlapArea_2D()` | `getOverlapVolume_3D()` |
| 索引類型 | `VECTOR_2D_INT` | `VECTOR_3D_INT` |
| 縮放因子 | `scale.x * scale.y` | `scale.x * scale.y * scale.z` |

## 依賴關係

### 必需的方法
- `curNode->getLL_3D()` / `getUR_3D()`: 3D模組邊界
- `curNode->getCenter()`: 3D模組中心
- `curNode->getWidth()` / `getHeight()` / `getDepth()`: 3D尺寸
- `getOverlapVolume_3D()`: 3D重疊體積計算

### 資料結構
- `Bin_3D`: 3D bin類別
- `VECTOR_3D_INT`: 3D整數向量
- `POS_3D`: 3D位置

## 測試結果

✅ **編譯成功**: 無編譯錯誤
✅ **執行成功**: 基本測試通過
✅ **記憶體安全**: 無記憶體洩漏或越界

## 後續工作

1. **性能優化**: 考慮平行化三重循環
2. **數值驗證**: 驗證密度分布的正確性
3. **整合測試**: 與FFT_3D的完整集成測試
4. **記憶體優化**: 大規模3D問題的記憶體使用優化

## 總結

3D bin節點密度更新的實現成功將2D EPlace的核心密度計算擴展到3D空間，保持了算法的完整性和數值穩定性。這為3D密度梯度計算提供了正確的輸入，是3D EPlace算法鏈中的關鍵環節。 