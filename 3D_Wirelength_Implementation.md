# 3D線長模型實現文檔

## 概述
成功實現了EPlace的3D線長模型，包含LSE (Log-Sum-Exp) 和 WA (Weighted-Average) 兩種模型的3D版本。

## 已實現的功能

### 1. 核心3D線長模型方法

#### Net類別新增方法
- `calcWirelengthLSE_3D(VECTOR_3D invertedGamma)` - 3D LSE線長計算
- `calcWirelengthWA_3D(VECTOR_3D invertedGamma)` - 3D WA線長計算
- `getWirelengthGradientLSE_3D(VECTOR_3D invertedGamma, Pin* curPin)` - 3D LSE梯度計算
- `getWirelengthGradientWA_3D(VECTOR_3D invertedGamma, Pin* curPin)` - 3D WA梯度計算
- `calcBoundPin_3D()` - 真正的3D邊界Pin計算（移除Z=0限制）

#### PlaceDB類別新增方法
- `calcLSE_Wirelength_3D(VECTOR_3D invertedGamma)` - 整體3D LSE線長
- `calcWA_Wirelength_3D(VECTOR_3D invertedGamma)` - 整體3D WA線長
- `calcNetBoundPins_3D()` - 整體3D邊界Pin計算

#### EPlacer_3D類別實現
- `wirelengthGradientUpdate()` - 完整的3D線長梯度更新
- `densityOverflowUpdate()` - 3D密度溢出計算

### 2. 3D線長模型特點

#### LSE模型 (Log-Sum-Exp)
```cpp
// 3D LSE線長計算公式：
HPWL_3D = (Xmax - Xmin + log(sumMaxX)/γx + log(sumMinX)/γx) +
          (Ymax - Ymin + log(sumMaxY)/γy + log(sumMinY)/γy) +
          (Zmax - Zmin + log(sumMaxZ)/γz + log(sumMinZ)/γz)
```

#### WA模型 (Weighted-Average)
```cpp
// 3D WA線長計算公式：
HPWL_3D = (numeratorMaxX/denominatorMaxX - numeratorMinX/denominatorMinX) +
          (numeratorMaxY/denominatorMaxY - numeratorMinY/denominatorMinY) +
          (numeratorMaxZ/denominatorMaxZ - numeratorMinZ/denominatorMinZ)
```

### 3. 3D gamma係數計算
支援針對X、Y、Z三個維度的獨立gamma係數：
```cpp
VECTOR_3D baseWirelengthCoef;
baseWirelengthCoef.x = 0.125 / binStep.x;
baseWirelengthCoef.y = 0.125 / binStep.y;
baseWirelengthCoef.z = 0.125 / binStep.z;  // 新增Z維度
```

### 4. 資料結構支援
- Pin類別已具備完整的3D資料結構：
  - `VECTOR_3D eMax_LSE`, `eMin_LSE` - LSE指數項
  - `VECTOR_3D eMax_WA`, `eMin_WA` - WA指數項
  - `VECTOR_3D_BOOL expZeroFlgMax_LSE`, `expZeroFlgMin_LSE` - LSE標誌
  - `VECTOR_3D_BOOL expZeroFlgMax_WA`, `expZeroFlgMin_WA` - WA標誌

- Net類別的3D支援：
  - `VECTOR_3D numeratorMax_WA`, `denominatorMax_WA` - WA分子分母
  - `VECTOR_3D numeratorMin_WA`, `denominatorMin_WA`
  - `VECTOR_3D sumMax_LSE`, `sumMin_LSE` - LSE總和

## 使用方式

### EPlacer_3D中的線長梯度更新
```cpp
// 在EPlacer_3D::wirelengthGradientUpdate()中
if (gArg.CheckExist("LSE"))
{
    double LSE = db->calcLSE_Wirelength_3D(invertedGamma);
    // 計算3D LSE梯度
}
else
{
    double WA = db->calcWA_Wirelength_3D(invertedGamma);
    // 計算3D WA梯度
}
```

### 3D優化器整合
已與EplaceNesterovOpt完全整合，支援3D向量的：
- 位置獲取 `getPosition()`
- 梯度獲取 `getGradient()`
- 位置設定 `setPosition()`

## 測試驗證

### 測試程式
`test_3d_basic.cpp` 驗證了：
- 3D資料結構正常運作
- EPlacer_3D基本功能
- 3D線長模型資料結構初始化

### 編譯測試
```bash
cd Eplace-3D-TDP/build
make ePlace3DTest
./EPlace/ePlace3DTest
```

## 下一步開發建議

### 1. 高優先級
- 實現3D密度梯度計算 (使用FFT_3D)
- 實現3D bin密度更新
- 完善3D filler初始化

### 2. 中優先級
- 3D視覺化支援
- 3D佈局品質評估指標
- 3D時序分析整合

### 3. 低優先級
- 3D P2P attraction梯度（可先忽略）
- 3D placement legalization
- 3D routing awareness

## 技術要點

### Gamma係數調整
3D環境中，Z維度的gamma係數需要根據實際的3D設計特性調整：
- 如果Z維度的span較小，可能需要較大的gamma
- 如果有多層設計，需考慮層間連接的特性

### 效能考量
3D線長計算的複雜度比2D略高，但主要瓶頸仍在密度計算的3D FFT部分。

### 兼容性
完全向後兼容2D設計，當Z座標都為0時，3D模型退化為2D模型。 