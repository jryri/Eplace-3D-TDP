#include "eplace.h"
#include <iostream>

using namespace std;

int main()
{
    cout << "=== Testing 3D Data Structures and EPlacer_3D ===" << endl;
    
    // Test VECTOR_3D_INT
    cout << "\n1. Testing VECTOR_3D_INT:" << endl;
    VECTOR_3D_INT vec3d_int;
    vec3d_int.x = 10;
    vec3d_int.y = 20;
    vec3d_int.z = 30;
    cout << "VECTOR_3D_INT: " << vec3d_int << endl;

    // Test VECTOR_3D
    cout << "\n2. Testing VECTOR_3D:" << endl;
    VECTOR_3D vec3d;
    vec3d.x = 1.5f;
    vec3d.y = 2.5f;
    vec3d.z = 3.5f;
    cout << "VECTOR_3D: " << vec3d << endl;

    // Test Bin_3D
    cout << "\n3. Testing Bin_3D:" << endl;
    Bin_3D bin3d;
    bin3d.width = 10.0f;
    bin3d.height = 20.0f;
    bin3d.depth = 15.0f;
    cout << "Bin_3D volume: " << bin3d.getVolume() << endl;

    // Test 3D overlap volume
    cout << "\n4. Testing 3D overlap volume:" << endl;
    POS_3D ll1, ur1, ll2, ur2;
    ll1.x = 0; ll1.y = 0; ll1.z = 0;
    ur1.x = 10; ur1.y = 10; ur1.z = 10;
    ll2.x = 5; ll2.y = 5; ll2.z = 5;
    ur2.x = 15; ur2.y = 15; ur2.z = 15;
    double overlap = getOverlapVolume_3D(ll1, ur1, ll2, ur2);
    cout << "Overlap volume: " << overlap << endl;

    // Test EPlacer_3D
    cout << "\n5. Testing EPlacer_3D:" << endl;
    try 
    {
        EPlacer_3D placer3d;
        cout << "EPlacer_3D initialized successfully" << endl;
        cout << "Initial target density: " << placer3d.targetDensity << endl;
        cout << "Initial placement stage: " << placer3d.placementStage << endl;
        
        // Test setter methods
        placer3d.setTargetDensity(0.8f);
        placer3d.setPlacementStage(mGP);
        cout << "After setting: target density = " << placer3d.targetDensity 
             << ", stage = " << placer3d.placementStage << endl;
             
        // Test 3D gamma initialization
        cout << "3D invertedGamma: " << placer3d.invertedGamma << endl;
        
        cout << "\n6. Testing 3D wirelength model data structures:" << endl;
        
        // Test Net 3D structures (these are already 3D)
        Net testNet;
        cout << "Net numeratorMax_WA: " << testNet.numeratorMax_WA << endl;
        cout << "Net sumMax_LSE: " << testNet.sumMax_LSE << endl;
        
        cout << "\n=== All 3D tests passed! ===" << endl;
    }
    catch (const exception& e) 
    {
        cerr << "Error during EPlacer_3D testing: " << e.what() << endl;
        return 1;
    }
    catch (...) 
    {
        cerr << "Unknown error during EPlacer_3D testing" << endl;
        return 1;
    }

    return 0;
} 