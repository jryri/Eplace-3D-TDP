#include "eplace.h"
#include "placedb.h"
#include "parser.h"
#include "arghandler.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    cout << "=== 2D to 3D Shrink Test ===" << endl;
    
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " -aux <aux_file>" << endl;
        return 1;
    }
    
    // Initialize argument handler
    gArg.Init(argc, argv);
    
    // Create PlaceDB and parser
    PlaceDB *placedb = new PlaceDB();
    BookshelfParser parser;
    
    // Parse the 2D benchmark file
    if (strcmp(argv[1] + 1, "aux") == 0) {
        string filename = argv[2];
        cout << "Loading 2D benchmark: " << filename << endl;
        parser.ReadFile(filename, *placedb);
    } else {
        cout << "Please provide -aux <filename>" << endl;
        return 1;
    }
    
    // Show original 2D database info
    cout << "\n=== Original 2D Database Info ===" << endl;
    placedb->showDBInfo();
    
    // Create 3D placer
    EPlacer_3D *eplacer3d = new EPlacer_3D(placedb);
    eplacer3d->setTargetDensity(0.8f);
    
    // Perform 2D to 3D conversion and placement
    cout << "\n=== Starting 2D to 3D Conversion ===" << endl;
    eplacer3d->testInitialization();
    
    // Show results
    cout << "\n=== 3D Conversion Results ===" << endl;
    cout << "3D Core region: [" << placedb->coreRegion.ll.x << "," << placedb->coreRegion.ll.y 
         << "," << placedb->coreRegion.ll_z << "] to [" 
         << placedb->coreRegion.ur.x << "," << placedb->coreRegion.ur.y 
         << "," << placedb->coreRegion.ur_z << "]" << endl;
    cout << "Total nodes: " << placedb->dbNodes.size() << endl;
    cout << "Total terminals: " << placedb->dbTerminals.size() << endl;
    cout << "3D Fillers created: " << eplacer3d->ePlaceFillers.size() << endl;
    cout << "3D Bin dimensions: " << eplacer3d->binDimension << endl;
    cout << "3D Bin steps: " << eplacer3d->binStep << endl;
    cout << "3D Density overflow: " << eplacer3d->globalDensityOverflow << endl;
    
    // Show some node positions as examples
    cout << "\n=== Sample 3D Node Positions ===" << endl;
    int maxShow = min(5, (int)placedb->dbNodes.size());
    for (int i = 0; i < maxShow; i++) {
        Module* node = placedb->dbNodes[i];
        VECTOR_3D center = node->getCenter();
        cout << "Node " << i << " (" << node->name << "): [" 
             << center.x << ", " << center.y << ", " << center.z << "]" 
             << " Volume: " << node->getVolume() << endl;
    }
    
    cout << "\n=== 2D to 3D Shrink Test Completed Successfully! ===" << endl;
    
    // Clean up
    delete eplacer3d;
    delete placedb;
    
    return 0;
} 