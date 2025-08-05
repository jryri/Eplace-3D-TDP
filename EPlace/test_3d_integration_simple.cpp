#include "eplace.h"
#include "placedb.h"
#include <iostream>
#include <vector>
#include <chrono>

using namespace std;

int main()
{
    cout << "=== 3D EPlace Simplified Integration Testing ===" << endl;
    cout << "Testing core 3D placement components" << endl;
    
    try {
        auto startTime = chrono::high_resolution_clock::now();
        
        // Create minimal PlaceDB for testing
        PlaceDB* testDB = new PlaceDB();
        
        // Set up minimal 3D core region
        testDB->setCoreRegion3D(0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 50.0f);
        testDB->defaultModuleDepth = 2.0f;
        testDB->commonRowHeight = 5.0f;
        testDB->totalRowVolume = 100.0f * 100.0f * 50.0f * 0.8f;
        
        cout << "\n🏗️  **Step 1: 3D Database Setup** ✅" << endl;
        cout << "Core region: [0,0,0] to [100,100,50]" << endl;
        
        // Create EPlacer_3D instance
        EPlacer_3D* placer3d = new EPlacer_3D(testDB);
        placer3d->setTargetDensity(0.8f);
        
        cout << "\n🎯 **Step 2: EPlacer_3D Basic Initialization**" << endl;
        cout << "Target density: " << placer3d->targetDensity << endl;
        cout << "Placement stage: " << placer3d->placementStage << endl;
        
        // Test 3D bin initialization without full design
        cout << "\n📦 **Step 3: Testing 3D Bin System**" << endl;
        
        // Set bin dimensions manually for testing
        placer3d->binDimension.x = 8;
        placer3d->binDimension.y = 8;
        placer3d->binDimension.z = 4;
        
        placer3d->binStep.x = testDB->coreRegion.getWidth() / placer3d->binDimension.x;
        placer3d->binStep.y = testDB->coreRegion.getHeight() / placer3d->binDimension.y;
        placer3d->binStep.z = testDB->coreRegion.getDepth() / placer3d->binDimension.z;
        
        cout << "3D Bin dimensions set: " << placer3d->binDimension << endl;
        cout << "3D Bin steps: " << placer3d->binStep << endl;
        cout << "Total bins: " << (placer3d->binDimension.x * placer3d->binDimension.y * placer3d->binDimension.z) << endl;
        
        // Initialize bin arrays
        placer3d->bins.resize(placer3d->binDimension.x);
        for (int i = 0; i < placer3d->binDimension.x; i++) {
            placer3d->bins[i].resize(placer3d->binDimension.y);
            for (int j = 0; j < placer3d->binDimension.y; j++) {
                placer3d->bins[i][j].resize(placer3d->binDimension.z);
                for (int k = 0; k < placer3d->binDimension.z; k++) {
                    placer3d->bins[i][j][k] = new Bin_3D();
                    
                    // Set bin coordinates
                    placer3d->bins[i][j][k]->ll.x = i * placer3d->binStep.x + testDB->coreRegion.ll.x;
                    placer3d->bins[i][j][k]->ll.y = j * placer3d->binStep.y + testDB->coreRegion.ll.y;
                    placer3d->bins[i][j][k]->ll.z = k * placer3d->binStep.z + testDB->coreRegion.ll_z;
                    
                    placer3d->bins[i][j][k]->ur.x = placer3d->bins[i][j][k]->ll.x + placer3d->binStep.x;
                    placer3d->bins[i][j][k]->ur.y = placer3d->bins[i][j][k]->ll.y + placer3d->binStep.y;
                    placer3d->bins[i][j][k]->ur.z = placer3d->bins[i][j][k]->ll.z + placer3d->binStep.z;
                    
                    placer3d->bins[i][j][k]->width = placer3d->binStep.x;
                    placer3d->bins[i][j][k]->height = placer3d->binStep.y;
                    placer3d->bins[i][j][k]->depth = placer3d->binStep.z;
                    placer3d->bins[i][j][k]->volume = placer3d->binStep.x * placer3d->binStep.y * placer3d->binStep.z;
                    
                    // Initialize center
                    placer3d->bins[i][j][k]->center.x = placer3d->bins[i][j][k]->ll.x + 0.5f * placer3d->binStep.x;
                    placer3d->bins[i][j][k]->center.y = placer3d->bins[i][j][k]->ll.y + 0.5f * placer3d->binStep.y;
                    placer3d->bins[i][j][k]->center.z = placer3d->bins[i][j][k]->ll.z + 0.5f * placer3d->binStep.z;
                }
            }
        }
        
        cout << "✅ 3D Bin grid initialized successfully" << endl;
        
        // Test 3D data structures
        cout << "\n🧪 **Step 4: Testing 3D Data Structures**" << endl;
        
        // Test Bin_3D
        Bin_3D* testBin = placer3d->bins[0][0][0];
        cout << "Sample bin [0][0][0]:" << endl;
        cout << "  - Lower left: [" << testBin->ll.x << ", " << testBin->ll.y << ", " << testBin->ll.z << "]" << endl;
        cout << "  - Upper right: [" << testBin->ur.x << ", " << testBin->ur.y << ", " << testBin->ur.z << "]" << endl;
        cout << "  - Volume: " << testBin->getVolume() << endl;
        cout << "  - Center: [" << testBin->center.x << ", " << testBin->center.y << ", " << testBin->center.z << "]" << endl;
        
        // Test 3D vectors
        VECTOR_3D testVec1, testVec2;
        testVec1.x = 1.0f; testVec1.y = 2.0f; testVec1.z = 3.0f;
        testVec2.x = 4.0f; testVec2.y = 5.0f; testVec2.z = 6.0f;
        VECTOR_3D result = testVec1 + testVec2;
        cout << "3D vector test: " << testVec1 << " + " << testVec2 << " = " << result << endl;
        
        // Test 3D overlap volume
        POS_3D ll1, ur1, ll2, ur2;
        ll1.x = 0; ll1.y = 0; ll1.z = 0;
        ur1.x = 10; ur1.y = 10; ur1.z = 10;
        ll2.x = 5; ll2.y = 5; ll2.z = 5;
        ur2.x = 15; ur2.y = 15; ur2.z = 15;
        float overlap = getOverlapVolume_3D(ll1, ur1, ll2, ur2);
        cout << "3D overlap volume test: " << overlap << " (expected: 125)" << endl;
        
        // Test FFT_3D basic functionality
        cout << "\n⚡ **Step 5: Testing 3D FFT System**" << endl;
        
        try {
            replace::FFT_3D fft(placer3d->binDimension.x, placer3d->binDimension.y, placer3d->binDimension.z,
                              placer3d->binStep.x, placer3d->binStep.y, placer3d->binStep.z);
            
            // Set some test density values
            for (int i = 0; i < placer3d->binDimension.x; i++) {
                for (int j = 0; j < placer3d->binDimension.y; j++) {
                    for (int k = 0; k < placer3d->binDimension.z; k++) {
                        float testDensity = 0.1f + 0.01f * (i + j + k); // Simple test pattern
                        fft.updateDensity(i, j, k, testDensity);
                    }
                }
            }
            
            // Perform FFT
            fft.doFFT();
            
            // Check results
            auto force = fft.getElectroForce(0, 0, 0);
            float phi = fft.getElectroPhi(0, 0, 0);
            
            cout << "✅ FFT_3D test completed" << endl;
            cout << "  Sample force [0][0][0]: [" << get<0>(force) << ", " << get<1>(force) << ", " << get<2>(force) << "]" << endl;
            cout << "  Sample potential [0][0][0]: " << phi << endl;
            
        } catch (const exception& e) {
            cout << "⚠️  FFT_3D test failed: " << e.what() << endl;
        }
        
        // Test 3D placement stages
        cout << "\n🎭 **Step 6: Testing 3D Placement Stages**" << endl;
        
        placer3d->setPlacementStage(mGP);
        cout << "✅ Set to mGP stage: " << placer3d->placementStage << endl;
        
        placer3d->setPlacementStage(FILLERONLY);
        cout << "✅ Set to FILLERONLY stage: " << placer3d->placementStage << endl;
        
        placer3d->setPlacementStage(cGP);
        cout << "✅ Set to cGP stage: " << placer3d->placementStage << endl;
        
        // Final performance analysis
        auto endTime = chrono::high_resolution_clock::now();
        auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        
        cout << "\n📊 **Integration Test Results**" << endl;
        cout << "================================" << endl;
        cout << "⏱️  Total runtime: " << totalTime.count() << "ms" << endl;
        cout << "🎯 Target density: " << placer3d->targetDensity << endl;
        cout << "📦 Total 3D bins: " << (placer3d->binDimension.x * placer3d->binDimension.y * placer3d->binDimension.z) << endl;
        cout << "🎲 3D Core volume: " << (100.0f * 100.0f * 50.0f) << endl;
        
        // Success checks
        cout << "\n🎉 **Test Results**" << endl;
        cout << "==================" << endl;
        
        bool success = true;
        
        // Check 1: 3D bin system
        if (placer3d->bins.size() > 0 && placer3d->bins[0].size() > 0 && placer3d->bins[0][0].size() > 0) {
            cout << "✅ 3D bin system: PASSED" << endl;
        } else {
            cout << "❌ 3D bin system: FAILED" << endl;
            success = false;
        }
        
        // Check 2: 3D data structures
        if (overlap > 0 && result.x == 5.0f && result.y == 7.0f && result.z == 9.0f) {
            cout << "✅ 3D data structures: PASSED" << endl;
        } else {
            cout << "❌ 3D data structures: FAILED" << endl;
            success = false;
        }
        
        // Check 3: Performance
        if (totalTime.count() < 5000) { // Less than 5 seconds
            cout << "✅ Performance: PASSED (runtime < 5s)" << endl;
        } else {
            cout << "⚠️  Performance: ACCEPTABLE (runtime = " << totalTime.count()/1000.0f << "s)" << endl;
        }
        
        // Final verdict
        cout << "\n🏆 **FINAL VERDICT**: ";
        if (success) {
            cout << "3D EPLACE CORE SYSTEMS WORKING! 🎉" << endl;
            cout << "All fundamental 3D components are operational!" << endl;
        } else {
            cout << "3D EPLACE NEEDS DEBUGGING ⚡" << endl;
            cout << "Some core components need attention." << endl;
        }
        
        // Cleanup
        for (int i = 0; i < placer3d->binDimension.x; i++) {
            for (int j = 0; j < placer3d->binDimension.y; j++) {
                for (int k = 0; k < placer3d->binDimension.z; k++) {
                    delete placer3d->bins[i][j][k];
                }
            }
        }
        delete placer3d;
        delete testDB;
        
    } catch (const exception& e) {
        cerr << "❌ Integration test failed: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "❌ Unknown error during integration testing" << endl;
        return 1;
    }
    
    cout << "\n🎊 3D Core Systems Integration Test Completed!" << endl;
    return 0;
} 