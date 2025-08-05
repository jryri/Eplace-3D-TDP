#include "eplace.h"

void EPlacer_3D::setTargetDensity(float target)
{
    targetDensity = target;
    cout << padding << "Target density set at: " << targetDensity << padding << endl;
}

void EPlacer_3D::setPlacementStage(int stage)
{
    placementStage = stage;
}

void EPlacer_3D::initialization()
{
    double binInitTime;
    fillerInitialization();
    time_start(&binInitTime);
    binInitialization();
    time_end(&binInitTime);
    cout << "3D Bin init time: " << binInitTime << endl;
    gradientVectorInitialization();
    totalGradientUpdate(); // first update
    cout << "Initial 3D Overflow: " << globalDensityOverflow << endl;
    penaltyFactorInitilization();
}

void EPlacer_3D::fillerInitialization()
{
    segmentFaultCP("fillerInit3D");
    ////////////////////////////////////////////////////////////////
    // calculate whitespace volume for 3D
    ////////////////////////////////////////////////////////////////

    float whitespaceVolume = 0;
    float totalOverLapVolume = 0; // overlap volume between placement regions and terminals

    for (Module *curTerminal : db->dbTerminals)
    {
        if (curTerminal->isNI) //!
        {
            continue;
        }
        
        // For 3D, we need to consider the terminal's volume
        POS_3D terminalLL = curTerminal->getLL_3D();
        POS_3D terminalUR = curTerminal->getUR_3D();

        for (SiteRow curRow : db->dbSiteRows)
        {
            // Extend 2D site rows to 3D placement regions
            POS_3D rowLL = curRow.getLL_3D();
            POS_3D rowUR = curRow.getUR_3D();
            totalOverLapVolume += getOverlapVolume_3D(terminalLL, terminalUR, rowLL, rowUR);
        }
    }

    whitespaceVolume = db->totalRowVolume - totalOverLapVolume; // Need to implement totalRowVolume in PlaceDB

    ////////////////////////////////////////////////////////////////
    // calculate node volume (3D version)
    ////////////////////////////////////////////////////////////////

    float nodeVolumeScaled = 0; // node = std cells + movable macros
    float stdcellVolume = 0;
    float macroVolume = 0;

    for (Module *curNode : db->dbNodes)
    {
        assert(curNode->getVolume() > 0); // Need to implement getVolume() in Module
        if (curNode->isMacro)
        {
            macroVolume += curNode->getVolume();
        }
        else
        {
            stdcellVolume += curNode->getVolume();
        }
    }

    ePlaceStdCellArea = stdcellVolume; // Actually volume now
    ePlaceMacroArea = macroVolume;

    nodeVolumeScaled = stdcellVolume + macroVolume * targetDensity;

    ////////////////////////////////////////////////////////////////
    // calculate filler volume (3D version)
    ////////////////////////////////////////////////////////////////

    float totalFillerVolume = 0;
    totalFillerVolume = whitespaceVolume * targetDensity - nodeVolumeScaled;

    int nodeCount = db->dbNodes.size();

    vector<float> nodeVolume; // sort node according to volume
    nodeVolume.resize(nodeCount);

    for (int i = 0; i < nodeCount; i++)
    {
        nodeVolume[i] = db->dbNodes[i]->getVolume();
    }

    sort(nodeVolume.begin(), nodeVolume.end());

    float avg80TotalVolume = 0;
    float avg80NodeVolume = 0;
    int minIdx = (int)(0.05 * (float)nodeCount);
    int maxIdx = (int)(0.95 * (float)nodeCount);

    for (int i = minIdx; i < maxIdx; i++)
    {
        avg80TotalVolume += nodeVolume[i];
    }

    avg80NodeVolume = avg80TotalVolume / ((float)(maxIdx - minIdx));

    float fillerVolume = avg80NodeVolume;
    float fillerHeight = db->commonRowHeight;
    float fillerWidth = fillerVolume / (fillerHeight * db->defaultModuleDepth); // Need defaultModuleDepth

    ////////////////////////////////////////////////////////////////
    // add 3D fillers and set filler locations randomly
    ////////////////////////////////////////////////////////////////

    int fillerCount = (int)(totalFillerVolume / fillerVolume + 0.5);

    ePlaceFillers.resize(fillerCount);

    for (int i = 0; i < fillerCount; i++)
    {
        string name = "f3d" + to_string(i);
        Module *curFiller = new Module(i + nodeCount, name, fillerWidth, fillerHeight, false, false);
        curFiller->isFiller = true;
        
        // 修復：明確設定 filler 的 depth 為 defaultModuleDepth，避免 Z 座標越界
        curFiller->depth = db->defaultModuleDepth;
        curFiller->calcVolume(); // 重新計算體積
        
        ePlaceFillers[i] = curFiller;

        db->setModuleLocation_3D_random(curFiller, (i%2) * db->defaultModuleDepth ); // Need to implement 3D version
    }

    ePlaceNodesAndFillers = db->dbNodes;
    ePlaceNodesAndFillers.insert(ePlaceNodesAndFillers.end(), ePlaceFillers.begin(), ePlaceFillers.end());

    for (Module *curCellOrFiller : ePlaceNodesAndFillers)
    {
        if (curCellOrFiller->isFiller)
        {
            ePlaceCellsAndFillers.push_back(curCellOrFiller);
        }
        else if (!curCellOrFiller->isMacro)
        {
            ePlaceCellsAndFillers.push_back(curCellOrFiller);
        }
    }
}

void EPlacer_3D::binInitialization()
{
    segmentFaultCP("binInit3D");
    ////////////////////////////////////////////////////////////////
    // calculate 3D bin dimension and size
    ////////////////////////////////////////////////////////////////
    
    int nodeCount = db->dbNodes.size();
    float nodeVolume = (ePlaceStdCellArea + ePlaceMacroArea); // Actually volume

    float coreRegionWidth = db->coreRegion.getWidth();
    float coreRegionHeight = db->coreRegion.getHeight();
    float coreRegionDepth = db->coreRegion.getDepth(); // Need to implement getDepth()
    float coreRegionVolume = coreRegionWidth * coreRegionHeight * coreRegionDepth;

    float averageNodeVolume = 1.0 * nodeVolume / nodeCount;
    float idealBinVolume = averageNodeVolume / targetDensity;

    int idealBinCount = INT_CONVERT(coreRegionVolume / idealBinVolume);

    // For 3D, we use cubic root to get bin dimensions
    int idealBinDimension = INT_CONVERT(pow(idealBinCount, 1.0/3.0));
    
    bool isUpdate = false;
    // Find the nearest power of 2 for 3D
    for (int i = 1; i < 8; i++) // up to 128^3 = 2M bins
    {
        int dimSize = 2 << i; // 4, 8, 16, 32, 64, 128
        if (dimSize * dimSize * dimSize <= idealBinCount &&
            (2 << (i + 1)) * (2 << (i + 1)) * (2 << (i + 1)) > idealBinCount)
        {
            binDimension.x = binDimension.y = binDimension.z = dimSize;
            isUpdate = true;
            break;
        }
    }
    if (!isUpdate)
    {
        binDimension.x = binDimension.y = binDimension.z = 64; // Default 3D bin size
    }

    cout << BLUE << "3D Bin dimension: " << binDimension << "\ncoreRegion: " 
         << coreRegionWidth << "x" << coreRegionHeight << "x" << coreRegionDepth << RESET << endl;

    binStep.x = coreRegionWidth / binDimension.x;
    binStep.y = coreRegionHeight / binDimension.y;
    binStep.z = coreRegionDepth / binDimension.z;

    cout << BLUE << "3D Bin step: " << binStep << RESET << endl;
    
    ////////////////////////////////////////////////////////////////
    // add 3D bins
    ////////////////////////////////////////////////////////////////
    
    bins.resize(binDimension.x);

    double addBinTime;
    time_start(&addBinTime);

    for (int i = 0; i < binDimension.x; i++)
    {
        bins[i].resize(binDimension.y);
        for (int j = 0; j < binDimension.y; j++)
        {
            bins[i][j].resize(binDimension.z);
            for (int k = 0; k < binDimension.z; k++)
            {
                bins[i][j][k] = new Bin_3D();
                
                bins[i][j][k]->ll.x = i * binStep.x + db->coreRegion.ll.x;
                bins[i][j][k]->ll.y = j * binStep.y + db->coreRegion.ll.y;
                bins[i][j][k]->ll.z = k * binStep.z + db->coreRegion.ll_z; // Use ll_z instead of ll.z

                bins[i][j][k]->width = binStep.x;
                bins[i][j][k]->height = binStep.y;
                bins[i][j][k]->depth = binStep.z;

                bins[i][j][k]->ur.x = bins[i][j][k]->ll.x + bins[i][j][k]->width;
                bins[i][j][k]->ur.y = bins[i][j][k]->ll.y + bins[i][j][k]->height;
                bins[i][j][k]->ur.z = bins[i][j][k]->ll.z + bins[i][j][k]->depth;

                bins[i][j][k]->volume = binStep.x * binStep.y * binStep.z;

                bins[i][j][k]->center.x = bins[i][j][k]->ll.x + 0.5f * bins[i][j][k]->width;
                bins[i][j][k]->center.y = bins[i][j][k]->ll.y + 0.5f * bins[i][j][k]->height;
                bins[i][j][k]->center.z = bins[i][j][k]->ll.z + 0.5f * bins[i][j][k]->depth;
            }
        }
    }

    time_end(&addBinTime);
    cout << "3D Bin add time: " << addBinTime << endl;

    ////////////////////////////////////////////////////////////////
    // 3D terminal density calculation
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("terminalDensity3D");
    VECTOR_3D_INT binStartIdx;
    VECTOR_3D_INT binEndIdx;

    double terminalDensityTime;
    time_start(&terminalDensityTime);

    for (Module *curTerminal : db->dbTerminals)
    {
        //! only consider terminals inside the 3D coreRegion
        //! assume no terminal would have a part inside and a part outside the coreRegion
        POS_3D terminalLL = curTerminal->getLL_3D();
        POS_3D terminalUR = curTerminal->getUR_3D();
        
        // 3D boundary checks
        if (terminalLL.x < db->coreRegion.ll.x || terminalUR.x > db->coreRegion.ur.x)
        {
            continue;
        }
        if (terminalLL.y < db->coreRegion.ll.y || terminalUR.y > db->coreRegion.ur.y)
        {
            continue;
        }
        if (terminalLL.z < db->coreRegion.ll_z || terminalUR.z > db->coreRegion.ur_z)
        {
            continue;
        }

        // Calculate 3D bin indices
        binStartIdx.x = INT_DOWN((terminalLL.x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((terminalUR.x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((terminalLL.y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((terminalUR.y - db->coreRegion.ll.y) / binStep.y);

        binStartIdx.z = INT_DOWN((terminalLL.z - db->coreRegion.ll_z) / binStep.z);
        binEndIdx.z = INT_DOWN((terminalUR.z - db->coreRegion.ll_z) / binStep.z);

        assert(binStartIdx.x >= 0);
        assert(binEndIdx.x >= 0);
        assert(binStartIdx.y >= 0);
        assert(binEndIdx.y >= 0);
        assert(binStartIdx.z >= 0);
        assert(binEndIdx.z >= 0);

        // Boundary checks for 3D
        if (binEndIdx.x >= binDimension.x)
        {
            binEndIdx.x = binDimension.x - 1;
        }
        if (binEndIdx.y >= binDimension.y)
        {
            binEndIdx.y = binDimension.y - 1;
        }
        if (binEndIdx.z >= binDimension.z)
        {
            binEndIdx.z = binDimension.z - 1;
        }

        // 3D terminal density distribution
        for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
        {
            for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
            {
                for (int k = binStartIdx.z; k <= binEndIdx.z; k++)
                {
                    //! beware: 3D density scaling!
                    float overlapVolume = getOverlapVolume_3D(bins[i][j][k]->ll, bins[i][j][k]->ur, 
                                                            terminalLL, terminalUR);
                    bins[i][j][k]->terminalDensity += targetDensity * overlapVolume;
                }
            }
        }
    }

    time_end(&terminalDensityTime);
    cout << "3D Terminal density time: " << terminalDensityTime << endl;
    
    ////////////////////////////////////////////////////////////////
    // 3D base density calculation
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("baseDensity3D");

    double baseDensityTime;
    time_start(&baseDensityTime);

    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            for (int k = 0; k < binDimension.z; k++)
            {
                float curBinAvailableVolume = 0; // overlap volume between current 3D bin and placement regions
                
                // Calculate available volume from site rows (extended to 3D)
                for (SiteRow curRow : db->dbSiteRows)
                {
                    // Get 3D boundaries of the site row
                    POS_3D rowLL = curRow.getLL_3D();
                    POS_3D rowUR = curRow.getUR_3D();
                    
                    curBinAvailableVolume += getOverlapVolume_3D(bins[i][j][k]->ll, bins[i][j][k]->ur, 
                                                               rowLL, rowUR);
                }
                
                // Calculate base density for 3D
                if (float_equal(bins[i][j][k]->volume, curBinAvailableVolume))
                {
                    bins[i][j][k]->baseDensity = 0;
                }
                else
                {
                    // 3D base density: targetDensity * (bin volume - available volume)
                    bins[i][j][k]->baseDensity = targetDensity * (bins[i][j][k]->volume - curBinAvailableVolume);
                }
            }
        }
    }
    
    time_end(&baseDensityTime);
    cout << "3D Base density time: " << baseDensityTime << endl;
    
    cout << "3D Bin initialization completed" << endl;
}

void EPlacer_3D::gradientVectorInitialization()
{
    wirelengthGradient.resize(db->dbNodes.size());
    p2pattractionGradient.resize(db->dbNodes.size());
    displacementGradient.resize(db->dbNodes.size());
    
    densityGradient.resize(ePlaceNodesAndFillers.size());
    totalGradient.resize(ePlaceNodesAndFillers.size());

    cGPGradient.resize(ePlaceCellsAndFillers.size());
    fillerGradient.resize(ePlaceFillers.size());
}

// Placeholder implementations for other methods - these would need full 3D implementations
void EPlacer_3D::binNodeDensityUpdate() 
{
    //!!!! clear nodeDensity and fillerDensity for each 3D bin before update!
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            for (int k = 0; k < binDimension.z; k++)
            {
                bins[i][j][k]->nodeDensity = 0;
                bins[i][j][k]->fillerDensity = 0;
            }
        }
    }

    segmentFaultCP("nodeDensity3D");
    for (Module *curNode : ePlaceNodesAndFillers) // ePlaceNodes: nodes and filler nodes
    {
        bool macroDensityScaling = false; // density scaling, see ePlace paper

        VECTOR_3D localSmoothLengthScale; // 3D local smooth factor
        localSmoothLengthScale.x = 1.0;
        localSmoothLengthScale.y = 1.0;
        localSmoothLengthScale.z = 1.0;

        // 3D rectangular region for current node
        POS_3D rectLowerLeft = curNode->getLL_3D();
        POS_3D rectUpperRight = curNode->getUR_3D();
        
        // Debug: Check for invalid Y coordinates  
        if (rectLowerLeft.y > rectUpperRight.y || curNode->getHeight() < 0) {
            cout << "ERROR: Module " << curNode->getName() << " has invalid coordinates!" << endl;
            cout << "  LL: " << rectLowerLeft << endl;
            cout << "  UR: " << rectUpperRight << endl;
            cout << "  Width: " << curNode->getWidth() << " Height: " << curNode->getHeight() << " Depth: " << curNode->getDepth() << endl;
            cout << "  Center: " << curNode->getCenter() << endl;
            cout << "  Location: " << curNode->getLocation() << endl;
        }
        
        // Debug: Check specific suspicious values
        if (rectUpperRight.y < 2000) {  // This looks like the problematic value
            cout << "SUSPICIOUS: Module " << curNode->getName() << " has suspiciously small top Y: " << rectUpperRight.y << endl;
            cout << "  LL: " << rectLowerLeft << endl;
            cout << "  UR: " << rectUpperRight << endl;
            cout << "  Height: " << curNode->getHeight() << endl;
            cout << "  Location: " << curNode->getLocation() << endl;
        }

        //! beware: local smooth on x, y, and z dimension!
        //! binStart and binEnd should be calculated with inflated cell dimensions
        //! local smooth: not only for std cells because there may be small macros
        POS_3D cellCenter = curNode->getCenter();

        // X dimension local smooth
        if (float_less(curNode->getWidth(), binStep.x))
        {
            localSmoothLengthScale.x = curNode->getWidth() / binStep.x;
            rectLowerLeft.x = cellCenter.x - 0.5 * binStep.x;
            rectUpperRight.x = cellCenter.x + 0.5 * binStep.x;
        }
        
        // Y dimension local smooth
        if (float_less(curNode->getHeight(), binStep.y))
        {
            localSmoothLengthScale.y = curNode->getHeight() / binStep.y;
            rectLowerLeft.y = cellCenter.y - 0.5 * binStep.y;
            rectUpperRight.y = cellCenter.y + 0.5 * binStep.y;
        }
        
        // Z dimension local smooth
        if (float_less(curNode->getDepth(), binStep.z))
        {
            localSmoothLengthScale.z = curNode->getDepth() / binStep.z;
            rectLowerLeft.z = cellCenter.z - 0.5 * binStep.z;
            rectUpperRight.z = cellCenter.z + 0.5 * binStep.z;
        }

        if (curNode->isMacro)
        {
            macroDensityScaling = true;
        }

        // Calculate 3D bin indices
        VECTOR_3D_INT binStartIdx; 
        VECTOR_3D_INT binEndIdx;
        binStartIdx.x = INT_DOWN((rectLowerLeft.x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((rectUpperRight.x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((rectLowerLeft.y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((rectUpperRight.y - db->coreRegion.ll.y) / binStep.y);

        binStartIdx.z = INT_DOWN((rectLowerLeft.z - db->coreRegion.ll_z) / binStep.z);
        binEndIdx.z = INT_DOWN((rectUpperRight.z - db->coreRegion.ll_z) / binStep.z);

        // Z 軸邊界檢查 - 關鍵限制：Z 座標不能小於 coreRegion.ll_z
        if (!(binStartIdx.z >= 0))
        {
            cout << "ERROR: Module " << curNode->getName() << " Z coordinate out of bounds!" << endl;
            cout << "  LL.z: " << rectLowerLeft.z << ", UR.z: " << rectUpperRight.z << endl;
            cout << "  coreRegion.ll_z: " << db->coreRegion.ll_z << endl;
            cout << "  binStartIdx.z: " << binStartIdx.z << " (must >= 0)" << endl;
        }
        if (!(binStartIdx.x >= 0))
        {
            cout << "3D Module pos: " << rectLowerLeft << " " << db->coreRegion.ll << endl;
        }
        assert(binStartIdx.x >= 0);
        assert(binEndIdx.x >= 0);
        assert(binStartIdx.y >= 0);
        assert(binEndIdx.y >= 0);
        assert(binStartIdx.z >= 0);  // Z 座標必須 >= 0（象限限制）
        assert(binEndIdx.z >= 0);

        // Boundary checks for 3D
        if (binEndIdx.x >= binDimension.x)
        {
            binEndIdx.x = binDimension.x - 1;
        }
        if (binEndIdx.y >= binDimension.y)
        {
            binEndIdx.y = binDimension.y - 1;
        }
        if (binEndIdx.z >= binDimension.z)
        {
            binEndIdx.z = binDimension.z - 1;
        }

        //! beware: 3D local smooth and density scaling!
        for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
        {
            for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
            {
                for (int k = binStartIdx.z; k <= binEndIdx.z; k++)
                {
                    // Calculate 3D overlap volume
                    float overlapVolume = getOverlapVolume_3D(bins[i][j][k]->ll, bins[i][j][k]->ur, 
                                                            rectLowerLeft, rectUpperRight);
                    
                    // Apply 3D local smooth scaling
                    float scaledOverlapVolume = localSmoothLengthScale.x * 
                                              localSmoothLengthScale.y * 
                                              localSmoothLengthScale.z * 
                                              overlapVolume;
                    
                    if (curNode->isMacro)
                    {
                        // Macro density scaling for 3D
                        bins[i][j][k]->nodeDensity += targetDensity * scaledOverlapVolume;
                    }
                    else
                    {
                        if (curNode->isFiller)
                        {
                            //? does filler need localSmooth in 3D?
                            bins[i][j][k]->fillerDensity += scaledOverlapVolume;
                        }
                        else
                        {
                            // Regular std cell
                            bins[i][j][k]->nodeDensity += scaledOverlapVolume;
                        }
                    }
                }
            }
        }
    }
}

void EPlacer_3D::densityOverflowUpdate() 
{
    segmentFaultCP("densityOverflow3D");
    float globalOverflowVolume = 0;
    float globalOverflowVolume_top = 0;
    float globalOverflowVolume_bottom = 0;
    float nodeVolumeScaled = ePlaceStdCellArea + ePlaceMacroArea * targetDensity; // Vm

    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            for (int k = 0; k < binDimension.z; k++)
            {
                // Vb^m (movable volume in bin) is approximated by nodeDensity + fillerDensity
                float movableVolumeInBin = bins[i][j][k]->nodeDensity + bins[i][j][k]->fillerDensity;

                // Vb^WS (whitespace in bin) is approximated by the remaining volume
                float whitespaceInBin = bins[i][j][k]->volume - (bins[i][j][k]->terminalDensity + bins[i][j][k]->baseDensity);
                whitespaceInBin = max(0.0f, whitespaceInBin); // Ensure whitespace is not negative

                // Overflow based on paper's formula (4): max(Vb^m - ρt * Vb^WS, 0)
                float overflowInBin = max(0.0f, movableVolumeInBin - targetDensity * whitespaceInBin);

                // Determine if the bin is in the top or bottom layer based on its Z center
                if (bins[i][j][k]->center.z < db->coreRegion.ll_z + db->coreRegion.getDepth() * 0.5f)
                {
                    globalOverflowVolume_bottom += overflowInBin;
                }
                else
                {
                    globalOverflowVolume_top += overflowInBin;
                }
            }
        }
    }

    globalOverflowVolume = globalOverflowVolume_top + globalOverflowVolume_bottom;
    globalDensityOverflow = globalOverflowVolume / nodeVolumeScaled; // τ_total = Σ overflow / Vm
    //separate overflow for bottom and top
    globalDensityOverflow_top = globalOverflowVolume_top / nodeVolumeScaled;
    globalDensityOverflow_bottom = globalOverflowVolume_bottom / nodeVolumeScaled;
}

void EPlacer_3D::wirelengthGradientUpdate() 
{
    ////////////////////////////////////////////////////////////////
    //! Step1: calculate gamma for 3D, see ePlace paper equation 38
    ////////////////////////////////////////////////////////////////
    // first, calculate tau(density overflow)
    densityOverflowUpdate();
    segmentFaultCP("wireLengthGradient3D");
    
    //! now calculate gamma with the updated tau, here we actually calculate 1/gamma for further calculation
    VECTOR_3D baseWirelengthCoef;

    // 3D wirelength係數計算：大幅增加係數以平衡density gradient的巨大量級
    // 由於3D中density gradient量級極大(~10^5)，需要對應增大wirelength係數
    float base3DCoeff = 125000.0f;  // 增大1000倍至125000以達到量級平衡
    baseWirelengthCoef.x = base3DCoeff / binStep.x;  // 直接除以binStep，不使用normalized
    baseWirelengthCoef.y = base3DCoeff / binStep.y; 
    baseWirelengthCoef.z = base3DCoeff / binStep.z;

    if (globalDensityOverflow > 1.0)
    {
        baseWirelengthCoef.x *= 0.1;
        baseWirelengthCoef.y *= 0.1;
        baseWirelengthCoef.z *= 0.1;
    }
    else if (globalDensityOverflow < 0.1)
    {
        baseWirelengthCoef.x *= 10.0;
        baseWirelengthCoef.y *= 10.0;
        baseWirelengthCoef.z *= 10.0;
    }
    else
    {
        float temp;
        temp = 1.0 / pow(10.0, (globalDensityOverflow - 0.1) * 20 / 9.0 - 1.0); // see eplace paper equation 38
        baseWirelengthCoef.x *= temp;
        baseWirelengthCoef.y *= temp;
        baseWirelengthCoef.z *= temp;
    }

    invertedGamma = baseWirelengthCoef;

    ////////////////////////////////////////////////////////////////
    //! Step2: calculate wirelength density for each nodes (not filler nodes)
    ////////////////////////////////////////////////////////////////
    // Update X/Y/Z max and min in all nets first for 3D
    double HPWL = db->calcNetBoundPins_3D();

    if (gArg.CheckExist("LSE"))
    {
        double LSE = db->calcLSE_Wirelength_3D(invertedGamma);
        int index = 0;
        for (Module *curNode : db->dbNodes)
        {
            assert(curNode->idx == index);
            wirelengthGradient[index].SetZero(); //! clear before updating
            for (Pin *curPin : curNode->modulePins)
            {
                VECTOR_3D gradient;
                gradient = curPin->net->getWirelengthGradientLSE_3D(invertedGamma, curPin);
                wirelengthGradient[index].x += gradient.x;
                wirelengthGradient[index].y += gradient.y;
                wirelengthGradient[index].z += gradient.z; // Add Z component
            }
            index++;
        }
    }
    else
    {
        double WA = db->calcWA_Wirelength_3D(invertedGamma);
        int index = 0;
        for (Module *curNode : db->dbNodes)
        {
            assert(curNode->idx == index);
            wirelengthGradient[index].SetZero(); //! clear before updating
            for (Pin *curPin : curNode->modulePins)
            {
                VECTOR_3D gradient;
                gradient = curPin->net->getWirelengthGradientWA_3D(invertedGamma, curPin);
                wirelengthGradient[index].x += gradient.x;
                wirelengthGradient[index].y += gradient.y;
                wirelengthGradient[index].z += gradient.z; // Add Z component
            }
            index++;
        }
    }
}

void EPlacer_3D::p2pattractionGradientUpdate() 
{
    segmentFaultCP("p2pAttractionGradient3D");
    int index = 0;
    for (Module *curNode : db->dbNodes)
    {
        assert(curNode->idx == index);
        p2pattractionGradient[index].SetZero(); //! clear before updating
        
        // TODO: Implement 3D P2P attraction gradient
        // For now, p2p is ignored as mentioned by user ("p2p可以先忽略")
        // The gradient remains zero
        
        // Future implementation would call:
        // for (Pin *curPin : curNode->modulePins)
        // {   
        //     VECTOR_3D gradient;
        //     gradient = curPin->net->getP2pAttractionGradient_3D(curPin, db);        
        //     p2pattractionGradient[index].x += gradient.x;
        //     p2pattractionGradient[index].y += gradient.y;
        //     p2pattractionGradient[index].z += gradient.z;
        // }
        
        index++;
    }
}

void EPlacer_3D::displacementGradientUpdate() 
{
    segmentFaultCP("displacementGradient3D");
    int index = 0;
    for (Module *curNode : db->dbNodes)
    {
        assert(curNode->idx == index);
        displacementGradient[index].SetZero(); //! clear before updating
        
        // VECTOR_3D curCenter = curNode->getCenter();
        // VECTOR_3D initialCenter = curNode->getInitialCenter(); 

        // // Calculate 3D displacement gradient
        // displacementGradient[index].x = curCenter.x - initialCenter.x;
        // displacementGradient[index].y = curCenter.y - initialCenter.y;
        // displacementGradient[index].z = curCenter.z - initialCenter.z; // Add Z component
        
        // Alternative implementation with plateau (commented out):
        // float plateau = pow(540, 2);
        // displacementGradient[index].x = 4 * pow((curCenter.x - initialCenter.x), 3) / plateau;
        // displacementGradient[index].y = 4 * pow((curCenter.y - initialCenter.y), 3) / plateau;
        // displacementGradient[index].z = 4 * pow((curCenter.z - initialCenter.z), 3) / plateau;
        
        index++;
    }
}

void EPlacer_3D::densityGradientUpdate() 
{
    ////////////////////////////////////////////////////////////////
    //! Step1: obtain 3D electric field(e) through FFT_3D
    ////////////////////////////////////////////////////////////////
    segmentFaultCP("densityGradient3D");
    replace::FFT_3D fft(binDimension.x, binDimension.y, binDimension.z, 
                        binStep.x, binStep.y, binStep.z);
    float invertedBinVolume = 1.0 / (binStep.x * binStep.y * binStep.z);
    
    // Update density in all 3D bins
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            for (int k = 0; k < binDimension.z; k++)
            {
                float eDensity = bins[i][j][k]->nodeDensity + 
                                bins[i][j][k]->baseDensity + 
                                bins[i][j][k]->fillerDensity + 
                                bins[i][j][k]->terminalDensity;
                eDensity *= invertedBinVolume;
                fft.updateDensity(i, j, k, eDensity);
            }
        }
    }
    
    // Perform 3D FFT
    fft.doFFT();
    
    // Extract electric field and potential from FFT results
    for (int i = 0; i < binDimension.x; i++)
    {
        for (int j = 0; j < binDimension.y; j++)
        {
            for (int k = 0; k < binDimension.z; k++)
            {
                auto eForce = fft.getElectroForce(i, j, k);
                bins[i][j][k]->E.x = std::get<0>(eForce);
                bins[i][j][k]->E.y = std::get<1>(eForce);
                bins[i][j][k]->E.z = std::get<2>(eForce);
                
                float electroPhi = fft.getElectroPhi(i, j, k);
                bins[i][j][k]->phi = electroPhi;
            }
        }
    }

    ////////////////////////////////////////////////////////////////
    //! Step2: calculate 3D density(potential) gradient, see ePlace paper equation 16
    ////////////////////////////////////////////////////////////////
    int nodeCount = db->dbNodes.size();
    int index = 0;
    for (Module *curNode : ePlaceNodesAndFillers)
    {
        assert(index == curNode->idx);
        //! clear before updating
        densityGradient[index].SetZero();

        VECTOR_3D localSmoothLengthScale; // 3D local smooth factor
        localSmoothLengthScale.x = 1.0;
        localSmoothLengthScale.y = 1.0;
        localSmoothLengthScale.z = 1.0;

        // 3D rectangular region for current node
        POS_3D nodeLowerLeft = curNode->getLL_3D();
        POS_3D nodeUpperRight = curNode->getUR_3D();
        
        //! beware: local smooth on x, y, and z dimension
        POS_3D cellCenter = curNode->getCenter();

        // X dimension local smooth
        if (float_less(curNode->getWidth(), binStep.x))
        {
            localSmoothLengthScale.x = curNode->getWidth() / binStep.x;
            nodeLowerLeft.x = cellCenter.x - 0.5 * binStep.x;
            nodeUpperRight.x = cellCenter.x + 0.5 * binStep.x;
        }
        
        // Y dimension local smooth
        if (float_less(curNode->getHeight(), binStep.y))
        {
            localSmoothLengthScale.y = curNode->getHeight() / binStep.y;
            nodeLowerLeft.y = cellCenter.y - 0.5 * binStep.y;
            nodeUpperRight.y = cellCenter.y + 0.5 * binStep.y;
        }
        
        // Z dimension local smooth
        if (float_less(curNode->getDepth(), binStep.z))
        {
            localSmoothLengthScale.z = curNode->getDepth() / binStep.z;
            nodeLowerLeft.z = cellCenter.z - 0.5 * binStep.z;
            nodeUpperRight.z = cellCenter.z + 0.5 * binStep.z;
        }

        // Calculate 3D bin indices
        VECTOR_3D_INT binStartIdx; 
        VECTOR_3D_INT binEndIdx;
        binStartIdx.x = INT_DOWN((nodeLowerLeft.x - db->coreRegion.ll.x) / binStep.x);
        binEndIdx.x = INT_DOWN((nodeUpperRight.x - db->coreRegion.ll.x) / binStep.x);

        binStartIdx.y = INT_DOWN((nodeLowerLeft.y - db->coreRegion.ll.y) / binStep.y);
        binEndIdx.y = INT_DOWN((nodeUpperRight.y - db->coreRegion.ll.y) / binStep.y);

        binStartIdx.z = INT_DOWN((nodeLowerLeft.z - db->coreRegion.ll_z) / binStep.z);
        binEndIdx.z = INT_DOWN((nodeUpperRight.z - db->coreRegion.ll_z) / binStep.z);

        assert(binStartIdx.x >= 0);
        assert(binEndIdx.x >= 0);
        assert(binStartIdx.y >= 0);
        assert(binEndIdx.y >= 0);
        assert(binStartIdx.z >= 0);
        assert(binEndIdx.z >= 0);

        // Boundary checks
        if (binEndIdx.x >= binDimension.x)
        {
            binEndIdx.x = binDimension.x - 1;
        }
        if (binEndIdx.y >= binDimension.y)
        {
            binEndIdx.y = binDimension.y - 1;
        }
        if (binEndIdx.z >= binDimension.z)
        {
            binEndIdx.z = binDimension.z - 1;
        }

        //! Apply 3D local smooth and calculate gradient
        for (int i = binStartIdx.x; i <= binEndIdx.x; i++)
        {
            for (int j = binStartIdx.y; j <= binEndIdx.y; j++)
            {
                for (int k = binStartIdx.z; k <= binEndIdx.z; k++)
                {
                    // 3D overlap volume calculation
                    POS_3D binLL = bins[i][j][k]->ll;
                    POS_3D binUR = bins[i][j][k]->ur;
                    float overlapVolume = localSmoothLengthScale.x * 
                                        localSmoothLengthScale.y * 
                                        localSmoothLengthScale.z * 
                                        getOverlapVolume_3D(binLL, binUR, nodeLowerLeft, nodeUpperRight);
                    
                    //! Add contribution from each bin's electric field
                    densityGradient[index].x += overlapVolume * bins[i][j][k]->E.x;
                    densityGradient[index].y += overlapVolume * bins[i][j][k]->E.y;
                    densityGradient[index].z += overlapVolume * bins[i][j][k]->E.z; // 3D Z component
                }
            }
        }

        index++;
    }
}

void EPlacer_3D::totalGradientUpdate()
{
    binNodeDensityUpdate();
    densityGradientUpdate();
    wirelengthGradientUpdate();
    p2pattractionGradientUpdate(); // p2p attraction gradient is only used in mGP

    double gWL = 0, gDEN = 0, gT = 0, gDIS = 0;

    segmentFaultCP("totalGradient3D");
    int index = 0;
    int cGPindex = 0;
    int fillerOnlyIndex = 0;
    beta = 0.00;
    for (Module *curNodeOrFiller : ePlaceNodesAndFillers)
    {
        totalGradient[index].SetZero();
        assert(index == curNodeOrFiller->idx);

        //! preconditioner calculation for 3D, according to paper's formula (13)
        // Hix ≈ λ * Vi
        float charge = curNodeOrFiller->getVolume(); // Vi
        float preconditioner = 1.0f / max(1.0f, lambda * charge);

        // Calculate 3D gradient magnitudes for statistics
        // Use sqrt(x^2 + y^2 + z^2) for 3D magnitude
        gWL  += preconditioner * sqrt(wirelengthGradient[index].x * wirelengthGradient[index].x + 
                                     wirelengthGradient[index].y * wirelengthGradient[index].y +
                                     wirelengthGradient[index].z * wirelengthGradient[index].z);
        gDEN += preconditioner * sqrt(densityGradient[index].x * densityGradient[index].x + 
                                     densityGradient[index].y * densityGradient[index].y +
                                     densityGradient[index].z * densityGradient[index].z);
        gT   += preconditioner * sqrt(p2pattractionGradient[index].x * p2pattractionGradient[index].x + 
                                     p2pattractionGradient[index].y * p2pattractionGradient[index].y +
                                     p2pattractionGradient[index].z * p2pattractionGradient[index].z);

        if (curNodeOrFiller->isFiller)
        {
            // wirelength gradient of fillers should == 0
            totalGradient[index].x = preconditioner * lambda * densityGradient[index].x;
            totalGradient[index].y = preconditioner * lambda * densityGradient[index].y;
            totalGradient[index].z = preconditioner * lambda * densityGradient[index].z; // Add Z component

            fillerGradient[fillerOnlyIndex] = totalGradient[index];
            fillerOnlyIndex++;

            cGPGradient[cGPindex] = totalGradient[index];
            cGPindex++;
        }
        else
        {
            // Combine all gradient components for 3D
            totalGradient[index].x = preconditioner * (lambda * densityGradient[index].x - 
                                                      wirelengthGradient[index].x - 
                                                      beta * p2pattractionGradient[index].x );
            totalGradient[index].y = preconditioner * (lambda * densityGradient[index].y - 
                                                      wirelengthGradient[index].y - 
                                                      beta * p2pattractionGradient[index].y );
            totalGradient[index].z = preconditioner * (lambda * densityGradient[index].z - 
                                                      wirelengthGradient[index].z - 
                                                      beta * p2pattractionGradient[index].z);
            

            if (!curNodeOrFiller->isMacro)
            {
                cGPGradient[cGPindex] = totalGradient[index];
                cGPindex++;
            }
        }

        index++;
    }
    printf("3D avg|G|  WL %.2e  DEN %.2e  TIM %.2e\n",
        gWL, gDEN, gT);
}

vector<VECTOR_3D> EPlacer_3D::getGradient()
{
    if (placementStage == mGP)
    {
        return totalGradient;
    }
    else if (placementStage == FILLERONLY)
    {
        return fillerGradient;
    }
    else if (placementStage == cGP)
    {
        return cGPGradient;
    }
    return totalGradient;
}

vector<VECTOR_3D> EPlacer_3D::getPosition()
{
    if (placementStage == mGP)
    {
        return getModulePositions(ePlaceNodesAndFillers);
    }
    else if (placementStage == FILLERONLY)
    {
        return getModulePositions(ePlaceFillers);
    }
    else if (placementStage == cGP)
    {
        return getModulePositions(ePlaceCellsAndFillers);
    }
    return getModulePositions(ePlaceNodesAndFillers);
}

void EPlacer_3D::setPosition(vector<VECTOR_3D> modulePositions)
{
    int moduleCount;
    if (placementStage == mGP)
    {
        moduleCount = ePlaceNodesAndFillers.size();
        for (int i = 0; i < moduleCount; i++)
        {
            db->setModuleCenter_3D(ePlaceNodesAndFillers[i], modulePositions[i]); // Need 3D version
        }
    }
    else if (placementStage == FILLERONLY)
    {
        moduleCount = ePlaceFillers.size();
        for (int i = 0; i < moduleCount; i++)
        {
            db->setModuleCenter_3D(ePlaceFillers[i], modulePositions[i]);
        }
    }
    else if (placementStage == cGP)
    {
        moduleCount = ePlaceCellsAndFillers.size();
        for (int i = 0; i < moduleCount; i++)
        {
            db->setModuleCenter_3D(ePlaceCellsAndFillers[i], modulePositions[i]);
        }
    }
}

void EPlacer_3D::penaltyFactorInitilization()
{
    lastHPWL = db->calcHPWL_3D(); // Use 3D HPWL calculation
    
    float denominator = 0;
    float numerator = 0;

    int nodeCount = wirelengthGradient.size();
    int nodeAndFillerCount = densityGradient.size();

    for (int i = 0; i < nodeCount; i++)
    {
        numerator += fabs(wirelengthGradient[i].x);
        numerator += fabs(wirelengthGradient[i].y);
        numerator += fabs(wirelengthGradient[i].z); // Add Z component
        // numerator += fabs(beta * p2pattractionGradient[i].x);
        // numerator += fabs(beta * p2pattractionGradient[i].y);
        // numerator += fabs(beta * p2pattractionGradient[i].z); // Add Z component
        // denominator += fabs(densityGradient[i].x);
        // denominator += fabs(densityGradient[i].y);
        // denominator += fabs(densityGradient[i].z); // Add Z component
    }
    for (int i = nodeCount; i < nodeAndFillerCount; i++)
    {
        denominator += fabs(densityGradient[i].x);
        denominator += fabs(densityGradient[i].y);
        denominator += fabs(densityGradient[i].z); // Add Z component
    }

    lambda = float_div(numerator, denominator);
    cout << "Initial 3D penalty factor: " << lambda << endl;
}

void EPlacer_3D::updatePenaltyFactor()
{
    float curHPWL = db->calcHPWL_3D(); // Use 3D HPWL calculation
    float multiplier;
    double deltaHPWL = curHPWL - lastHPWL;
    if ((deltaHPWL) < 0.0)
    {
        multiplier = PENALTY_MULTIPLIER_UPPERBOUND;
    }
    else
    {
        multiplier = pow(PENALTY_MULTIPLIER_BASE, (-(deltaHPWL) / DELTA_HPWL_REF + 1.0));
    }

    if (float_greater(multiplier, PENALTY_MULTIPLIER_UPPERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_UPPERBOUND;
    }
    if (float_less(multiplier, PENALTY_MULTIPLIER_LOWERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_LOWERBOUND;
    }
    lambda *= multiplier;
    lastHPWL = curHPWL;
}

void EPlacer_3D::updatePenaltyFactorbyTNS(int iter_power)
{
    float multiplier;
    double deltaTNS = abs(curTNS) - abs(lastTNS);
    
    if ((deltaTNS) < 0.0)
    {
        multiplier = PENALTY_MULTIPLIER_UPPERBOUND;
    }
    else
    {
        multiplier = pow(PENALTY_MULTIPLIER_BASE, (-(deltaTNS) / DELTA_TNS_REF + 1.0));
    }

    if (float_greater(multiplier, PENALTY_MULTIPLIER_UPPERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_UPPERBOUND;
    }
    if (float_less(multiplier, PENALTY_MULTIPLIER_LOWERBOUND))
    {
        multiplier = PENALTY_MULTIPLIER_LOWERBOUND;
    }
    lambda *= multiplier;
    lastTNS = curTNS;
}

void EPlacer_3D::switch2FillerOnly()
{
    for (Module *curFiller : ePlaceFillers)
    {
        db->setModuleLocation_3D_random(curFiller);
    }
    placementStage = FILLERONLY;
}

void EPlacer_3D::switch2cGP()
{
    lambda = lambda / pow(1.1, mGPIterationCount * 0.1);
    placementStage = cGP;
}

void EPlacer_3D::showInfo()
{
    cout << "3D Overflow: " << globalDensityOverflow << endl;
    cout << "3D penalty factor: " << fixed << lambda << endl;
    cout << "HPWL (3D): " << lastHPWL << endl;
    cout << "curTNS: " << curTNS << endl;
    cout << "lastTNS: " << lastTNS << endl << endl;
}

void EPlacer_3D::showInfoFinal()
{
    cout << "Final 3D placement results:" << endl;
    showInfo();
}

vector<VECTOR_3D> EPlacer_3D::getModulePositions(vector<Module *> modules)
{
    int moduleCount = modules.size();
    vector<VECTOR_3D> res;
    res.resize(moduleCount);

    for (int i = 0; i < modules.size(); i++)
    {
        res[i] = modules[i]->getCenter(); // Should return VECTOR_3D
    }
    return res;
} 

void EPlacer_3D::shrink2DTo3DTestData()
{
    cout << "=== Shrinking 2D test data to 3D ===" << endl;
    
    // Step 1: 設定3D core region (縮放因子 1/√2)
    float originalWidth = db->coreRegion.getWidth();
    float originalHeight = db->coreRegion.getHeight();
    float originalArea = originalWidth * originalHeight;
    float originalCenterX = db->coreRegion.ll.x + originalWidth * 0.5f;
    float originalCenterY = db->coreRegion.ll.y + originalHeight * 0.5f;
    
    float shrinkFactor = 1.0f / sqrt(2.0f);  // 每層面積 = 原面積/2
    float newWidth = originalWidth * shrinkFactor;
    float newHeight = originalHeight * shrinkFactor;
    // float layerThickness = 10.0f;  // 簡化的層厚度
    float layerThickness = min(originalWidth, originalHeight) / 2;  // 簡化的層厚度
    
    cout << "Shrink factor: " << shrinkFactor << ", Layer thickness: " << layerThickness << endl;
    
    // 更新3D core region (中心對齊)
    float newLLX = originalCenterX - newWidth * 0.5f;
    float newLLY = originalCenterY - newHeight * 0.5f;
    db->setCoreRegion3D(newLLX, newLLY, 0.0f, 
                       newLLX + newWidth, newLLY + newHeight, layerThickness * 2);
    db->defaultModuleDepth = layerThickness;
    
    // Step 2: 設定所有modules的depth屬性
    for (Module* node : db->dbNodes) {
        node->depth = layerThickness;
        node->calcVolume();
    }
    
    // Step 3: 調整terminals位置 (應用shrink factor)
    float origCenterX = originalCenterX;
    float origCenterY = originalCenterY;
    
    for (Module* terminal : db->dbTerminals) {
        terminal->depth = layerThickness;
        terminal->calcVolume();
        
        POS_2D originalPos = terminal->getLL_2D();
        float terminalCenterX = originalPos.x + terminal->getWidth() * 0.5f;
        float terminalCenterY = originalPos.y + terminal->getHeight() * 0.5f;
        
        // 計算相對位置並應用shrink factor
        float relativeX = (terminalCenterX - origCenterX) / originalWidth;
        float relativeY = (terminalCenterY - origCenterY) / originalHeight;
        
        float newCenterX = newLLX + newWidth * 0.5f + relativeX * newWidth;
        float newCenterY = newLLY + newHeight * 0.5f + relativeY * newHeight;
        float newX = newCenterX - terminal->getWidth() * 0.5f;
        float newY = newCenterY - terminal->getHeight() * 0.5f;
        
        // 根據Y位置分配層：上半部→頂層，下半部→底層
        // float terminalZ = (relativeY > 0) ? layerThickness : 0.0f;
        float terminalZ = layerThickness/2;
        db->setModuleLocation_3D(terminal, newX, newY, terminalZ);
    }
    
    // Step 4: 創建3D site rows (兩層)
    vector<SiteRow> originalRows = db->dbSiteRows;
    db->dbSiteRows.clear();
    
    for (const SiteRow& originalRow : originalRows) {
        float originalRowCenterX = (originalRow.start.x + originalRow.end.x) * 0.5f;
        float originalRowCenterY = originalRow.start.y + originalRow.height * 0.5f;
        
        // 計算相對位置並應用shrink factor
        float relativeX = (originalRowCenterX - origCenterX) / originalWidth;
        float relativeY = (originalRowCenterY - origCenterY) / originalHeight;
        
        float newRowCenterX = newLLX + newWidth * 0.5f + relativeX * newWidth;
        float newRowCenterY = newLLY + newHeight * 0.5f + relativeY * newHeight;
        float newRowWidth = (originalRow.end.x - originalRow.start.x) * shrinkFactor;
        
        // 為每層創建site row
        for (int layer = 0; layer < 2; layer++) {
            SiteRow newRow = originalRow;
            newRow.height *= shrinkFactor; // Height must be scaled along with coordinates
            newRow.start.x = newRowCenterX - newRowWidth * 0.5f;
            newRow.end.x = newRowCenterX + newRowWidth * 0.5f;
            newRow.start.y = newRowCenterY - newRow.height * 0.5f; // Use scaled height
            newRow.end.y = newRow.start.y;
            newRow.bottom = newRow.start.y;
            newRow.front = layer * layerThickness;
            newRow.depth = layerThickness;
            
            // 邊界檢查
            newRow.start.x = max(newLLX, newRow.start.x);
            newRow.end.x = min(newLLX + newWidth, newRow.end.x);
            newRow.start.y = max(newLLY, newRow.start.y);
            
            db->dbSiteRows.push_back(newRow);
        }
    }
    
    // 計算total row volume
    db->totalRowVolume = 0;
    for (const SiteRow& row : db->dbSiteRows) {
        db->totalRowVolume += (row.end.x - row.start.x) * row.height * row.depth;
    }

    // Verification step
    cout << "Verifying SiteRow heights..." << endl;
    for (const auto& row : db->dbSiteRows) {
        if (row.height <= 0.0f) {
            cout << "Error: Invalid SiteRow height found!" << endl;
            cout << "  start.x: " << row.start.x << ", end.x: " << row.end.x << endl;
            cout << "  start.y: " << row.start.y << ", end.y: " << row.end.y << endl;
            cout << "  height: " << row.height << endl;
            exit(1);
        }
    }
    cout << "SiteRow verification passed." << endl;

    cout << "Created " << db->dbSiteRows.size() << " site rows (2 layers), total volume: " 
         << db->totalRowVolume << endl;
}

void EPlacer_3D::placeModulesAtCenter()
{
    cout << "=== Placing modules at center (replacing QPlace3D) ===" << endl;
    
    // 計算core region的中心區域
    float centerX = (db->coreRegion.ll.x + db->coreRegion.ur.x) * 0.5f;
    float centerY = (db->coreRegion.ll.y + db->coreRegion.ur.y) * 0.5f;
    float centerZ = (db->coreRegion.ll_z + db->coreRegion.ur_z) * 0.5f;
    
    // 計算放置區域大小（中心區域的50%）
    float placeWidth = db->coreRegion.getWidth() * 0.5f;
    float placeHeight = db->coreRegion.getHeight() * 0.5f;
    float placeDepth = db->coreRegion.getDepth() * 0.5f;
    
    cout << "Center placement area: " << placeWidth << " x " << placeHeight << " x " << placeDepth << endl;
    
    // 使用2層分配策略確保平分
    int nodeCount = db->dbNodes.size();
    int gridX = (int)ceil(sqrt(nodeCount / 2.0));  // 每層的X網格數
    int gridY = gridX;                              // 每層的Y網格數
    int gridZ = 2;                                  // 固定2層
    
    float stepX = placeWidth / max(1, gridX);
    float stepY = placeHeight / max(1, gridY);
    float stepZ = placeDepth / max(1, gridZ-1);     // 兩層之間的距離
    
    cout << "Grid layout (2-layer): " << gridX << " x " << gridY << " x " << gridZ << endl;
    cout << "Grid steps: " << stepX << " x " << stepY << " x " << stepZ << endl;
    
    // 放置所有nodes，確保平分到兩層
    for (int i = 0; i < nodeCount; i++) {
        Module* node = db->dbNodes[i];
        
        // 交替分配到兩層
        int layerIndex = i % 2;  // 0 = 底層, 1 = 頂層
        
        // 在每層內的XY位置
        int moduleIndexInLayer = i / 2;
        int gx = moduleIndexInLayer % gridX;
        int gy = moduleIndexInLayer / gridX;
        
        // 計算實際位置（加上隨機偏移避免重疊）
        float x = centerX - placeWidth * 0.5f + gx * stepX + stepX * 0.5f;
        float y = centerY - placeHeight * 0.5f + gy * stepY + stepY * 0.5f;
        // 使用明確的層位置，確保正確的層分離
        float layerThickness = db->defaultModuleDepth;
        float z = (layerIndex == 0) ? 0.0f : layerThickness;
        
        // // 添加小幅隨機偏移避免完全對齊
        // x += (rand() % 100 - 50) * 0.01f * stepX;
        // y += (rand() % 100 - 50) * 0.01f * stepY;
        // // Z 軸只能向上偏移，避免負數
        // z += abs(rand() % 50) * 0.01f * stepZ;
        
        // // 確保在邊界內 (Z軸必須 >= 0 以滿足 binStartIdx.z >= 0 的限制)
        // x = max(db->coreRegion.ll.x, min(db->coreRegion.ur.x - node->getWidth(), x));
        // y = max(db->coreRegion.ll.y, min(db->coreRegion.ur.y - node->getHeight(), y));
        
        // // Z座標邊界檢查：LL.z應該在[0, Zmax/2]範圍內連續變化
        // float zMax = db->coreRegion.ur_z;  // = 2 * layerThickness  
        // float zMaxHalf = zMax * 0.5f;      // = layerThickness
        
        // // 確保LL.z在[0, Zmax/2]範圍內，這樣UR.z = LL.z + depth不會超過Zmax
        // z = max(db->coreRegion.ll_z, min(zMaxHalf, z));  // LL.z ∈ [0, Zmax/2]
        
        // // 設定位置
        // // db->setModuleLocation_3D(node, x, y, z);
        db->setModuleLocation_3D(node, centerX, centerY, layerThickness/2);

        // db->setModuleLocation_3D_random(node,layerThickness/2 );
        
        if (i < 10) { // 只打印前10個模組避免輸出太多
            cout << "Node " << i << " (" << node->name << "): [" << x << ", " << y << ", " << z << "]" << endl;
        }
    }
    
    cout << "=== Center placement completed ===" << endl;
}

void EPlacer_3D::testInitialization()
{
    cout << "\n=== 3D Test Initialization ===" << endl;
    
    // Step 1: Shrink 2D data to 3D
    shrink2DTo3DTestData();
    
    // Step 2: Place modules at center (replacing QPlace)
    placeModulesAtCenter();
    
    // Step 3: Normal 3D initialization
    double binInitTime;
    fillerInitialization();
    time_start(&binInitTime);
    binInitialization();
    time_end(&binInitTime);
    cout << "3D Bin init time: " << binInitTime << endl;
    gradientVectorInitialization();
    totalGradientUpdate(); // first update
    cout << "Initial 3D Overflow: " << globalDensityOverflow << endl;
    penaltyFactorInitilization();
    
    cout << "=== 3D Test Initialization Completed ===" << endl;
} 