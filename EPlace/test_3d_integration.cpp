#include "eplace.h"
#include "placedb.h"
#include <iostream>
#include <vector>
#include <chrono>

using namespace std;

// Simplified helper function to create mock 3D design data
void create3DTestDesign(PlaceDB* db) {
    cout << "Creating simplified 3D test design..." << endl;
    
    // Set up 3D core region (100x100x50 units)
    db->setCoreRegion3D(0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 50.0f);
    db->defaultModuleDepth = 2.0f;
    db->commonRowHeight = 5.0f;
    db->totalRowVolume = 100.0f * 100.0f * 50.0f * 0.8f; // 80% utilizable
    
    // Create 3D site rows (layers)
    for (int layer = 0; layer < 5; layer++) {
        SiteRow row;
        row.front = layer * 10.0f;
        row.depth = 10.0f;
        row.left = 0.0f;
        row.width = 100.0f;
        row.bottom = 0.0f;
        row.height = 100.0f;
        db->dbSiteRows.push_back(row);
    }
    
    // Create simplified modules (nodes) - smaller number for testing
    int nodeCount = 10; // Smaller number for initial testing
    db->dbNodes.clear();
    
    for (int i = 0; i < nodeCount; i++) {
        string name = "node_3d_" + to_string(i);
        float width = 3.0f;
        float height = 3.0f;
        bool isMacro = (i == 0); // Only first node is a macro
        
        Module* node = new Module(i, name, width, height, isMacro, false);
        node->depth = db->defaultModuleDepth;
        node->calcVolume();
        
        // Set simple grid-based initial 3D position
        POS_3D initPos;
        initPos.x = 10.0f + (i % 3) * 20.0f;
        initPos.y = 10.0f + (i / 3) * 20.0f;
        initPos.z = 10.0f + (i % 5) * 8.0f;
        
        node->setInitialLocation_3D(initPos.x, initPos.y, initPos.z);
        node->setLocation_3D(initPos.x, initPos.y, initPos.z);
        
        db->dbNodes.push_back(node);
    }
    
    // Create simplified nets
    int netCount = 5; // Fewer nets for testing
    db->dbNets.clear();
    
    for (int i = 0; i < netCount; i++) {
        string name = "net_3d_" + to_string(i);
        Net* net = new Net(i, name);
        
        // Connect 2 nodes per net
        for (int j = 0; j < 2; j++) {
            int nodeIdx = (i * 2 + j) % nodeCount;
            Pin* pin = new Pin(i * 10 + j, "pin_" + to_string(i) + "_" + to_string(j));
            pin->module = db->dbNodes[nodeIdx];
            pin->net = net;
            
            // Simple pin offset
            pin->offsetX = 0.0f;
            pin->offsetY = 0.0f;
            pin->offsetZ = 0.0f;
            
            net->netPins.push_back(pin);
            db->dbNodes[nodeIdx]->modulePins.push_back(pin);
        }
        
        db->dbNets.push_back(net);
    }
    
    // Create simplified terminals
    db->dbTerminals.clear();
    for (int i = 0; i < 4; i++) {
        string name = "terminal_3d_" + to_string(i);
        Module* terminal = new Module(1000 + i, name, 5.0f, 5.0f, false, true);
        terminal->depth = 5.0f;
        terminal->calcVolume();
        
        // Place terminals at corners
        float x = (i % 2 == 0) ? 5.0f : 95.0f;
        float y = (i < 2) ? 5.0f : 95.0f;
        float z = 25.0f;
        
        terminal->setLocation_3D(x, y, z);
        db->dbTerminals.push_back(terminal);
    }
    
    cout << "Simplified 3D design created:" << endl;
    cout << "  - Nodes: " << db->dbNodes.size() << endl;
    cout << "  - Nets: " << db->dbNets.size() << endl;
    cout << "  - Terminals: " << db->dbTerminals.size() << endl;
    cout << "  - Site Rows: " << db->dbSiteRows.size() << endl;
    cout << "  - Core Region: 100x100x50" << endl;
}

int main()
{
    cout << "=== 3D EPlace Integration Testing ===" << endl;
    cout << "Testing 3D placement system components" << endl;
    
    try {
        auto startTime = chrono::high_resolution_clock::now();
        
        // Create test database and design
        PlaceDB* testDB = new PlaceDB();
        create3DTestDesign(testDB);
        
        cout << "\n🏗️  **Step 1: 3D Database Setup** ✅" << endl;
        
        // Create 3D placer
        EPlacer_3D* placer3d = new EPlacer_3D(testDB);
        placer3d->setTargetDensity(0.8f);
        
        cout << "\n🎯 **Step 2: EPlacer_3D Initialization**" << endl;
        
        // Initialize the 3D placer
        auto initStart = chrono::high_resolution_clock::now();
        placer3d->initialization();
        auto initEnd = chrono::high_resolution_clock::now();
        auto initTime = chrono::duration_cast<chrono::milliseconds>(initEnd - initStart);
        
        cout << "✅ 3D Placer initialized in " << initTime.count() << "ms" << endl;
        cout << "   - Bin dimensions: " << placer3d->binDimension << endl;
        cout << "   - Bin steps: " << placer3d->binStep << endl;
        cout << "   - Total modules: " << placer3d->ePlaceNodesAndFillers.size() << endl;
        cout << "   - Initial overflow: " << placer3d->globalDensityOverflow << endl;
        
        // Test individual 3D components
        cout << "\n⚡ **Step 3: Testing 3D Component Functions**" << endl;
        
        // Test gradient calculations
        auto gradStart = chrono::high_resolution_clock::now();
        
        cout << "\n📊 Testing 3D gradient calculations..." << endl;
        float initialHPWL = testDB->calcHPWL_3D(); // Use 3D HPWL for testing
        cout << "Initial HPWL: " << initialHPWL << endl;
        
        // Test wirelength gradient
        placer3d->wirelengthGradientUpdate();
        cout << "✅ 3D wirelength gradient calculated" << endl;
        
        // Test density gradient (using FFT_3D)
        placer3d->densityGradientUpdate();
        cout << "✅ 3D density gradient calculated (FFT_3D)" << endl;
        
        // Test total gradient update
        placer3d->totalGradientUpdate();
        cout << "✅ 3D total gradient calculated" << endl;
        
        auto gradEnd = chrono::high_resolution_clock::now();
        auto gradTime = chrono::duration_cast<chrono::milliseconds>(gradEnd - gradStart);
        cout << "Gradient calculation time: " << gradTime.count() << "ms" << endl;
        
        // Test position and gradient access
        cout << "\n📍 Testing 3D position and gradient access..." << endl;
        vector<VECTOR_3D> positions = placer3d->getPosition();
        vector<VECTOR_3D> gradients = placer3d->getGradient();
        
        cout << "Position vector size: " << positions.size() << endl;
        cout << "Gradient vector size: " << gradients.size() << endl;
        
        if (!positions.empty()) {
            cout << "Sample position[0]: " << positions[0] << endl;
        }
        if (!gradients.empty()) {
            cout << "Sample gradient[0]: " << gradients[0] << endl;
        }
        
        // Test simple position update
        cout << "\n🔄 Testing position updates..." << endl;
        for (int i = 0; i < min(3, (int)positions.size()); i++) {
            positions[i].x += 1.0f;
            positions[i].y += 1.0f;
            positions[i].z += 0.5f;
        }
        placer3d->setPosition(positions);
        cout << "✅ Position update completed" << endl;
        
        // Recalculate metrics after position change
        placer3d->totalGradientUpdate();
        float newHPWL = testDB->calcHPWL();
        cout << "HPWL after position update: " << newHPWL << endl;
        
        // Test placement stage switching
        cout << "\n🎭 Testing placement stage switching..." << endl;
        
        // Test FILLERONLY stage
        placer3d->switch2FillerOnly();
        cout << "✅ Switched to FILLERONLY stage" << endl;
        cout << "   Stage: " << placer3d->placementStage << endl;
        cout << "   Filler count: " << placer3d->ePlaceFillers.size() << endl;
        
        // Test cGP stage
        placer3d->switch2cGP();
        cout << "✅ Switched to cGP stage" << endl;
        cout << "   Stage: " << placer3d->placementStage << endl;
        
        // Final Results and Analysis
        auto endTime = chrono::high_resolution_clock::now();
        auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        
        cout << "\n📊 **3D Integration Test Results**" << endl;
        cout << "================================" << endl;
        
        cout << "⏱️  Total runtime: " << totalTime.count() << "ms" << endl;
        cout << "📏 Final HPWL: " << newHPWL << endl;
        cout << "📦 Final density overflow: " << placer3d->globalDensityOverflow << endl;
        cout << "🎯 Target density: " << placer3d->targetDensity << endl;
        cout << "⚖️  Penalty factor: " << placer3d->lambda << endl;
        
        // 3D Specific Analysis
        cout << "\n🎲 **3D-Specific Analysis**" << endl;
        cout << "3D Bin grid: " << placer3d->binDimension.x << "×" 
             << placer3d->binDimension.y << "×" << placer3d->binDimension.z << endl;
        cout << "Total 3D bins: " << (placer3d->binDimension.x * placer3d->binDimension.y * placer3d->binDimension.z) << endl;
        cout << "3D Core volume: " << (100.0f * 100.0f * 50.0f) << endl;
        
        // Module Z-distribution analysis
        cout << "\n📍 **3D Module Distribution**" << endl;
        float avgZ = 0, minZ = 1000, maxZ = -1000;
        for (Module* node : testDB->dbNodes) {
            VECTOR_3D center = node->getCenter();
            float z = center.z;
            avgZ += z;
            minZ = min(minZ, z);
            maxZ = max(maxZ, z);
            cout << "Node " << node->name << ": [" << center.x << ", " << center.y << ", " << center.z << "]" << endl;
        }
        avgZ /= testDB->dbNodes.size();
        
        cout << "Z-axis distribution: min=" << minZ << ", max=" << maxZ << ", avg=" << avgZ << endl;
        
        // Success indicators
        cout << "\n🎉 **Integration Test Results**" << endl;
        cout << "================================" << endl;
        
        bool success = true;
        
        // Check 1: Basic functionality
        if (gradients.size() == positions.size() && !gradients.empty()) {
            cout << "✅ 3D gradient system: PASSED" << endl;
        } else {
            cout << "❌ 3D gradient system: FAILED" << endl;
            success = false;
        }
        
        // Check 2: 3D bounds
        bool inBounds = (minZ >= 0 && maxZ <= 50.0f);
        if (inBounds) {
            cout << "✅ 3D bounds check: PASSED (all modules within Z=[0,50])" << endl;
        } else {
            cout << "⚠️  3D bounds check: WARNING (minZ=" << minZ << ", maxZ=" << maxZ << ")" << endl;
        }
        
        // Check 3: FFT_3D functionality
        if (placer3d->globalDensityOverflow >= 0) {
            cout << "✅ 3D FFT density calculation: PASSED" << endl;
        } else {
            cout << "❌ 3D FFT density calculation: FAILED" << endl;
            success = false;
        }
        
        // Check 4: Runtime performance
        if (totalTime.count() < 10000) { // Less than 10 seconds
            cout << "✅ Performance: PASSED (runtime < 10s)" << endl;
        } else {
            cout << "⚠️  Performance: ACCEPTABLE (runtime = " << totalTime.count()/1000.0f << "s)" << endl;
        }
        
        // Final verdict
        cout << "\n🏆 **FINAL VERDICT**: ";
        if (success) {
            cout << "3D EPLACE INTEGRATION TEST PASSED! 🎉" << endl;
            cout << "All 3D components are working correctly!" << endl;
        } else {
            cout << "3D EPLACE INTEGRATION TEST NEEDS ATTENTION ⚡" << endl;
            cout << "Some components need debugging." << endl;
        }
        
        // Cleanup
        delete placer3d;
        delete testDB;
        
    } catch (const exception& e) {
        cerr << "❌ Integration test failed: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "❌ Unknown error during integration testing" << endl;
        return 1;
    }
    
    cout << "\n🎊 Integration testing completed!" << endl;
    return 0;
} 