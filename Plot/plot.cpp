#include "plot.h"

using namespace PLOTTING;

int PLOTTING::getX(float regionLLx, float x, float unitX)
{
    return (x - regionLLx) * unitX;
}

// the Y-axis must be mirrored
int PLOTTING::getY(float regionHeight, float regionLLy, float y, float unitY)
{
    return (regionHeight - (y - regionLLy)) * unitY; //?
    // return (chipRegionHeight - y) * unitY;//?
}

void PLOTTING::plotCurrentPlacement(string imageName, PlaceDB *db)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = db->chipRegion.ur.x - db->chipRegion.ll.x;
    float chipRegionHeight = db->chipRegion.ur.y - db->chipRegion.ll.y;

    int minImgaeLength = 1000;

    int imageHeight;
    int imageWidth;

    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    CImg<unsigned char> img(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    float unitX = imageWidth / chipRegionWidth,
          unitY = imageHeight / chipRegionHeight;

    for (Module *curTerminal : db->dbTerminals)
    {
        assert(curTerminal);
        // ignore pin's location
        if (curTerminal->isNI)
        {
            continue;
        }
        int x1 = getX(db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : db->dbNodes)
    {
        assert(curNode);
        int x1 = getX(db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        if (curNode->isMacro)
        {
            img.draw_rectangle(x1, y1, x2, y2, Orange, opacity);
        }
        else
        {
            img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
        }
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(plotPath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}

// 3D version of plotCurrentPlacement with separate top and bottom layer images
void PLOTTING::plotCurrentPlacement_3D(string imageName, PlaceDB *db)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = db->chipRegion.ur.x - db->chipRegion.ll.x;
    float chipRegionHeight = db->chipRegion.ur.y - db->chipRegion.ll.y;
    float zMax = db->coreRegion.ur_z;
    float zThreshold = zMax * 0.25f; // 1/4 Zmax for layer separation

    cout << "3D PlotCurrentPlacement Info: Zmax=" << zMax << ", Z threshold=" << zThreshold << endl;

    int minImgaeLength = 1000;
    int imageHeight, imageWidth;
    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    float unitX = imageWidth / chipRegionWidth;
    float unitY = imageHeight / chipRegionHeight;

    // Count modules in each layer
    int bottomLayerCount = 0, topLayerCount = 0;

    // Create two separate images: one for top layer, one for bottom layer
    CImg<unsigned char> topImg(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);
    CImg<unsigned char> bottomImg(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    // Process all modules and separate them by layer
    for (Module *curNode : db->dbNodes)
    {
        assert(curNode);
        
        int x1 = getX(db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        
        // Get module's Z coordinate (lower-left corner)
        float moduleZ = curNode->getLL_3D().z;
        
        // Determine layer and draw on appropriate image
        if (moduleZ > zThreshold) // Top layer (Z > Zmax/4)
        {
            topLayerCount++;
            const unsigned char* color = curNode->isMacro ? Orange : Red; // Orange for macro, Red for std cell
            topImg.draw_rectangle(x1, y1, x2, y2, color, opacity);
        }
        else // Bottom layer (Z <= Zmax/4)
        {
            bottomLayerCount++;
            const unsigned char* color = curNode->isMacro ? Orange : Red; // Orange for macro, Red for std cell
            bottomImg.draw_rectangle(x1, y1, x2, y2, color, opacity);
        }
    }

    // Draw terminals on appropriate layer based on their Z coordinate
    for (Module *curTerminal : db->dbTerminals)
    {
        assert(curTerminal);
        if (curTerminal->isNI) continue;
        
        int x1 = getX(db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        
        // Get terminal's Z coordinate to determine which layer it belongs to
        float terminalZ = curTerminal->getLL_3D().z;
        
        if (terminalZ > zThreshold) {
            topImg.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
        } else {
            bottomImg.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
        }
    }

    // Add titles and layer information to both images
    string topTitle = imageName + " - Top Layer";
    string bottomTitle = imageName + " - Bottom Layer";
    
    topImg.draw_text(50, 50, topTitle.c_str(), Black, NULL, 1, 30);
    bottomImg.draw_text(50, 50, bottomTitle.c_str(), Black, NULL, 1, 30);
    
    // Add layer statistics
    string topInfo = "Top Layer: " + to_string(topLayerCount) + " modules (Z > " + to_string(zThreshold) + ")";
    string bottomInfo = "Bottom Layer: " + to_string(bottomLayerCount) + " modules (Z <= " + to_string(zThreshold) + ")";
    
    topImg.draw_text(50, 100, topInfo.c_str(), Black, NULL, 1, 20);
    bottomImg.draw_text(50, 100, bottomInfo.c_str(), Black, NULL, 1, 20);

    // Save both images
    string topFilename = plotPath + imageName + "_top.bmp";
    string bottomFilename = plotPath + imageName + "_bottom.bmp";
    
    topImg.save_bmp(topFilename.c_str());
    bottomImg.save_bmp(bottomFilename.c_str());
    
    cout << "INFO: 3D TOP LAYER BMP SAVED: " << imageName + "_top.bmp" << endl;
    cout << "INFO: 3D BOTTOM LAYER BMP SAVED: " << imageName + "_bottom.bmp" << endl;
    cout << "3D Layer Distribution - Bottom: " << bottomLayerCount << " modules, Top: " << topLayerCount << " modules" << endl;
}

void PLOTTING::plotEPlace_2D(string imageName, EPlacer_2D *eplacer)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = eplacer->db->chipRegion.ur.x - eplacer->db->chipRegion.ll.x;
    float chipRegionHeight = eplacer->db->chipRegion.ur.y - eplacer->db->chipRegion.ll.y;

    int minImgaeLength = 1000;

    int imageHeight;
    int imageWidth;

    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    CImg<unsigned char> img(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    float unitX = imageWidth / chipRegionWidth,
          unitY = imageHeight / chipRegionHeight;

    for (Module *curTerminal : eplacer->db->dbTerminals)
    {
        assert(curTerminal);
        // ignore pin's location
        if (curTerminal->isNI)
        {
            continue;
        }
        int x1 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        img.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
    }

    for (Module *curNode : eplacer->db->dbNodes)
    {
        assert(curNode);
        int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        if (curNode->isMacro)
        {
            img.draw_rectangle(x1, y1, x2, y2, Orange, opacity);
        }
        else
        {
            img.draw_rectangle(x1, y1, x2, y2, Red, opacity);
        }
    }

    if ((gArg.CheckExist("debug") || eplacer->placementStage == FILLERONLY))
    {
        for (Module *curNode : eplacer->ePlaceFillers)
        {
            assert(curNode);
            int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
            int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
            int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
            int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
            img.draw_rectangle(x1, y1, x2, y2, Green, opacity);
        }
    }

    img.draw_text(50, 50, imageName.c_str(), Black, NULL, 1, 30);
    img.save_bmp(string(plotPath + imageName + string(".bmp")).c_str());
    cout << "INFO: BMP HAS BEEN SAVED: " << imageName + string(".bmp") << endl;
}

// 3D placement plotting with separate top and bottom layer images
void PLOTTING::plotEPlace_3D(string imageName, EPlacer_3D *eplacer)
{
    string plotPath;
    if (!gArg.GetString("plotPath", &plotPath))
    {
        plotPath = "./";
    }

    float chipRegionWidth = eplacer->db->chipRegion.ur.x - eplacer->db->chipRegion.ll.x;
    float chipRegionHeight = eplacer->db->chipRegion.ur.y - eplacer->db->chipRegion.ll.y;
    float zMax = eplacer->db->coreRegion.ur_z;
    float zThreshold = zMax * 0.25f; // 1/4 Zmax for layer separation

    cout << "3D Plot Info: Zmax=" << zMax << ", Z threshold=" << zThreshold << endl;

    int minImgaeLength = 1000;
    int imageHeight, imageWidth;
    float opacity = 0.7;
    int xMargin = 30, yMargin = 30;

    if (chipRegionWidth < chipRegionHeight)
    {
        imageHeight = 1.0 * chipRegionHeight / (chipRegionWidth / minImgaeLength);
        imageWidth = minImgaeLength;
    }
    else
    {
        imageWidth = 1.0 * chipRegionWidth / (chipRegionHeight / minImgaeLength);
        imageHeight = minImgaeLength;
    }

    float unitX = imageWidth / chipRegionWidth;
    float unitY = imageHeight / chipRegionHeight;

    // Count modules in each layer
    int bottomLayerCount = 0, topLayerCount = 0;

    // Create two separate images: one for top layer, one for bottom layer
    CImg<unsigned char> topImg(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);
    CImg<unsigned char> bottomImg(imageWidth + 2 * xMargin, imageHeight + 2 * yMargin, 1, 3, 255);

    // Process all modules and separate them by layer
    for (Module *curNode : eplacer->db->dbNodes)
    {
        assert(curNode);
        
        int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
        
        // Get module's Z coordinate (lower-left corner)
        float moduleZ = curNode->getLL_3D().z;
        
        // Determine layer and draw on appropriate image
        if (moduleZ > zThreshold) // Top layer (Z > Zmax/4)
        {
            topLayerCount++;
            const unsigned char* color = curNode->isMacro ? Orange : Red; // Orange for macro, Red for std cell
            topImg.draw_rectangle(x1, y1, x2, y2, color, opacity);
        }
        else // Bottom layer (Z <= Zmax/4)
        {
            bottomLayerCount++;
            const unsigned char* color = curNode->isMacro ? Orange : Red; // Orange for macro, Red for std cell
            bottomImg.draw_rectangle(x1, y1, x2, y2, color, opacity);
        }
    }

    // Draw terminals on both images (terminals are accessible from both layers)
    for (Module *curTerminal : eplacer->db->dbTerminals)
    {
        assert(curTerminal);
        if (curTerminal->isNI) continue;
        
        int x1 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getLL_2D().x, unitX) + xMargin;
        int x2 = getX(eplacer->db->chipRegion.ll.x, curTerminal->getUR_2D().x, unitX) + xMargin;
        int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getLL_2D().y, unitY) + yMargin;
        int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curTerminal->getUR_2D().y, unitY) + yMargin;
        
        // Get terminal's Z coordinate to determine which layer it belongs to
        float terminalZ = curTerminal->getLL_3D().z;
        
        if (terminalZ > zThreshold) {
            topImg.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
        } else {
            bottomImg.draw_rectangle(x1, y1, x2, y2, Blue, opacity);
        }
    }

    // Draw fillers if in debug mode or FILLERONLY stage
    if ((gArg.CheckExist("debug") || eplacer->placementStage == FILLERONLY))
    {
        for (Module *curNode : eplacer->ePlaceFillers)
        {
            assert(curNode);
            
            int x1 = getX(eplacer->db->chipRegion.ll.x, curNode->getLL_2D().x, unitX) + xMargin;
            int x2 = getX(eplacer->db->chipRegion.ll.x, curNode->getUR_2D().x, unitX) + xMargin;
            int y1 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getLL_2D().y, unitY) + yMargin;
            int y2 = getY(chipRegionHeight, eplacer->db->chipRegion.ll.y, curNode->getUR_2D().y, unitY) + yMargin;
            
            // Get filler's Z coordinate to determine layer
            float fillerZ = curNode->getLL_3D().z;
            
            if (fillerZ > zThreshold) {
                topImg.draw_rectangle(x1, y1, x2, y2, Green, opacity);
            } else {
                bottomImg.draw_rectangle(x1, y1, x2, y2, Green, opacity);
            }
        }
    }

    // Add titles and layer information to both images
    string topTitle = imageName + " - Top Layer";
    string bottomTitle = imageName + " - Bottom Layer";
    
    topImg.draw_text(50, 50, topTitle.c_str(), Black, NULL, 1, 30);
    bottomImg.draw_text(50, 50, bottomTitle.c_str(), Black, NULL, 1, 30);
    
    // Add layer statistics
    string topInfo = "Top Layer: " + to_string(topLayerCount) + " modules (Z > " + to_string(zThreshold) + ")";
    string bottomInfo = "Bottom Layer: " + to_string(bottomLayerCount) + " modules (Z <= " + to_string(zThreshold) + ")";
    
    topImg.draw_text(50, 100, topInfo.c_str(), Black, NULL, 1, 20);
    bottomImg.draw_text(50, 100, bottomInfo.c_str(), Black, NULL, 1, 20);

    // Save both images
    string topFilename = plotPath + imageName + "_top.bmp";
    string bottomFilename = plotPath + imageName + "_bottom.bmp";
    
    topImg.save_bmp(topFilename.c_str());
    bottomImg.save_bmp(bottomFilename.c_str());
    
    cout << "INFO: 3D TOP LAYER BMP SAVED: " << imageName + "_top.bmp" << endl;
    cout << "INFO: 3D BOTTOM LAYER BMP SAVED: " << imageName + "_bottom.bmp" << endl;
    cout << "3D Layer Distribution - Bottom: " << bottomLayerCount << " modules, Top: " << topLayerCount << " modules" << endl;
}
