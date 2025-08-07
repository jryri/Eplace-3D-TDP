#include "qplace.h"
#include "parser.h"
#include "arghandler.h"
#include "eplace.h"
#include "opt.hpp"
#include "nesterov.hpp"
#include "legalizer.h"
#include "detailed.h"
#include "plot.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    BookshelfParser parser;
    string inital_def_path ="";
    PlaceDB *placedb = new PlaceDB();
    gArg.Init(argc, argv);

    if (argc < 2)
    {
        return 0;
    }
    if (strcmp(argv[1] + 1, "aux") == 0) // -aux, argv[1]=='-'
    {
        // bookshelf
        printf("Use BOOKSHELF placement format\n");

        string filename = argv[2];
        string::size_type pos = filename.rfind("/");
        string benchmarkName;
        if (pos != string::npos)
        {
            printf("    Path = %s\n", filename.substr(0, pos + 1).c_str());
            gArg.Override("path", filename.substr(0, pos + 1));

            int length = filename.length();

            benchmarkName = filename.substr(pos + 1, length - pos);

            int len = benchmarkName.length();
            if (benchmarkName.substr(len - 4, 4) == ".aux")
            {
                benchmarkName = benchmarkName.erase(len - 4, 4);
            }
            gArg.Override("benchmarkName", benchmarkName);
            cout << "    Benchmark: " << benchmarkName << endl;

            string outputFilePath;
            if (!gArg.GetString("outputPath", &outputFilePath))
            {
                outputFilePath = "./";
            }
            outputFilePath = outputFilePath + "/" + benchmarkName + "/";
            gArg.Override("outputPath", outputFilePath);

            string plotPath = outputFilePath + "Graphs/";
            gArg.Override("plotPath", plotPath);

            string cmd = "rm -rf " + outputFilePath;
            system(cmd.c_str());

            cmd = "mkdir -p " + outputFilePath;
            system(cmd.c_str());

            cmd = "rm -rf " + plotPath;
            system(cmd.c_str());

            cmd = "mkdir -p " + plotPath;
            system(cmd.c_str());

            cout << "    Plot path: " << plotPath << endl;
        }
        string auxPath = string(argv[2]);
        parser.ReadFile(auxPath, *placedb);
        inital_def_path = auxPath.substr(0, auxPath.rfind(".")) + ".def";

    }
    placedb->showDBInfo();

    
    // string plPath;
    // if (gArg.GetString("loadpl", &plPath))
    // {
    //     //! modules will be moved to center in QP, so if QP is not skipped, loading module locations from an existing pl file is meaningless
    //     parser.ReadPLFile(plPath, *placedb, false);
    // }

    // QPPlacer *qpplacer = new QPPlacer(placedb);
    // if (!gArg.CheckExist("noQP"))
    // {
    //     qpplacer->quadraticPlacement();
    // }

    // if (gArg.CheckExist("addNoise"))
    // {
    //     placedb->addNoise(); // the noise range is [-avgbinStep,avgbinStep]
    // }

    double mGPTime;
    double mLGTime;
    double FILLERONLYtime;
    double cGPTime;
    bool isTiming = false;
    if (gArg.CheckExist("timing")|| gArg.CheckExist("TDP"))
    {
        isTiming = true;
        cout << "Timing optimization enabled!\n";
    }
    else
    {
        cout << "Timing optimization disabled!\n";
    }

    OpenroadInterface* openroadInterface = new OpenroadInterface(placedb);
    string initialDEFPath = inital_def_path ;//"./testcase/aes_cipher_top/aes_cipher_top.def";
    string openroadPath = "/usr/bin/openroad";
    string tclPath = "./eplace_sta.tcl";
    string staReportPath = "./eplace_sta_report.rpt";
    openroadInterface->intializePaths(initialDEFPath, openroadPath, tclPath, staReportPath);

    float targetDensity;
    if (!gArg.GetFloat("targetDensity", &targetDensity))
    {
        targetDensity = 1.0;
    }

    FirstOrderOptimizer<VECTOR_3D> *opt = nullptr;
    EPlacer_2D *eplacer = nullptr;
    EPlacer_3D *eplacer3d = nullptr;
    // EPlacer_2D_all_tier *eplacer2d_all_tier = nullptr;
    
    // Check if 3DIC mode is enabled
    if (gArg.CheckExist("3DIC"))
    {
        cout << "Using 3D placer for 2D-to-3D shrink test!" << endl;
        eplacer3d = new EPlacer_3D(placedb);
        eplacer3d->setTargetDensity(targetDensity);
        
        // Use special initialization for 2D-to-3D conversion
        eplacer3d->testInitialization();  // This includes shrink2DTo3D + placeModulesAtCenter + normal init
        
        opt = new EplaceNesterovOpt<VECTOR_3D>(eplacer3d, isTiming, openroadInterface);
    }
    else
    {
        cout << "Using 2D placer (normal mode)" << endl;
        eplacer = new EPlacer_2D(placedb);
        eplacer->setTargetDensity(targetDensity);
        eplacer->initialization();
        
        opt = new EplaceNesterovOpt<VECTOR_3D>(eplacer, isTiming, openroadInterface);
    }

    if (isTiming) {
        cout << "isTiming" << endl;
    }
    else {
        cout << "not isTiming" << endl;
    }

    if (!gArg.CheckExist("nomGP"))
    {
        cout << "mGP started!\n";

        time_start(&mGPTime);
        opt->opt();
        time_end(&mGPTime);

        cout << "mGP finished!\n";
        // Additional 3D processing if needed
        if (gArg.CheckExist("3DIC")) {
            cout << "3D mGP completed!\n";
        }
       
        if (gArg.CheckExist("3DIC")) {
            cout << "Final HPWL (3D): " << int(placedb->calcHPWL_3D()) << endl;
        } else {
            cout << "Final HPWL: " << int(placedb->calcHPWL()) << endl;
        }

        cout << "mGP time: " << mGPTime << endl;
        if (gArg.CheckExist("3DIC")) {
            PLOTTING::plotCurrentPlacement_3D("mGP result", placedb);
        } else {
            PLOTTING::plotCurrentPlacement("mGP result", placedb);
        }
    }

    ///////////////////////////////////////////////////
    // legalization and detailed placement
    ///////////////////////////////////////////////////

    // if (placedb->dbMacroCount > 0 && !gArg.CheckExist("nomLG"))
    // {
    //     SAMacroLegalizer *macroLegalizer = new SAMacroLegalizer(placedb);
    //     macroLegalizer->setTargetDensity(targetDensity);
    //     cout << "Start mLG, total macro count: " << placedb->dbMacroCount << endl;
    //     time_start(&mLGTime);
    //     macroLegalizer->legalization();
    //     time_end(&mLGTime);

    //     if (gArg.CheckExist("3DIC")) {
    //         PLOTTING::plotCurrentPlacement_3D("mLG result", placedb);
    //     } else {
    //         PLOTTING::plotCurrentPlacement("mLG result", placedb);
    //     }
    //     if (gArg.CheckExist("3DIC")) {
    //         cout << "mLG finished. HPWL after mLG (3D): " << int(placedb->calcHPWL_3D()) << endl;
    //     } else {
    //         cout << "mLG finished. HPWL after mLG: " << int(placedb->calcHPWL()) << endl;
    //     }
    //     cout << "mLG time: " << mLGTime << endl;
    //     // exit(0);

    //     if (!gArg.CheckExist("nocGP"))
    //     {
    //         // Call switch2FillerOnly on appropriate placer
    //         if (gArg.CheckExist("3DIC") && eplacer3d) {
    //             eplacer3d->switch2FillerOnly();
    //         } else if (eplacer) {
    //             eplacer->switch2FillerOnly();
    //         }
    //         cout << "filler placement started!\n";

    //         time_start(&FILLERONLYtime);
    //         opt->opt();
    //         time_end(&FILLERONLYtime);

    //         cout << "filler placement finished!\n";
    //         cout << "FILLERONLY time: " << mGPTime << endl;
    //         if (gArg.CheckExist("3DIC")) {
    //             PLOTTING::plotCurrentPlacement_3D("FILLERONLY result", placedb);
    //         } else {
    //             PLOTTING::plotCurrentPlacement("FILLERONLY result", placedb);
    //         }

    //         // Call switch2cGP on appropriate placer
    //         if (gArg.CheckExist("3DIC") && eplacer3d) {
    //             eplacer3d->switch2cGP();
    //         } else if (eplacer) {
    //             eplacer->switch2cGP();
    //         }
    //         cout << "cGP started!\n";

    //         time_start(&cGPTime);
    //         opt->opt();
    //         time_end(&cGPTime);

    //         cout << "cGP finished!\n";
    //         if (gArg.CheckExist("3DIC")) {
    //             cout << "cGP Final HPWL (3D): " << int(placedb->calcHPWL_3D()) << endl;
    //         } else {
    //             cout << "cGP Final HPWL: " << int(placedb->calcHPWL()) << endl;
    //         }
    //         cout << "cGP time: " << mGPTime << endl;
    //         if (gArg.CheckExist("3DIC")) {
    //             PLOTTING::plotCurrentPlacement_3D("cGP result", placedb);
    //         } else {
    //             PLOTTING::plotCurrentPlacement("cGP result", placedb);
    //         }
    //     }
    // }

    placedb->outputBookShelf("eGP",true); // output, files will be used for legalizers such as ntuplace3

    if (!gArg.CheckExist("noLegal"))
    {
        string legalizerPath;
        if (gArg.GetString("legalizerPath", &legalizerPath))
        {
            string outputAUXPath;
            string outputPLPath;
            string outputPath;
            string benchmarkName;

            gArg.GetString("outputAUX", &outputAUXPath);
            gArg.GetString("outputPL", &outputPLPath);
            gArg.GetString("outputPath", &outputPath);
            gArg.GetString("benchmarkName", &benchmarkName);

            string cmd = legalizerPath + "/ntuplace3" + " -aux " + outputAUXPath + " -loadpl " + outputPLPath + " -noglobal" + " -out " + outputPath + benchmarkName + "-ntu" + " > " + outputPath + "ntuplace3-log.txt";

            cout << RED << "Running legalizer and detailed placer: " << cmd << RESET << endl;
            system(cmd.c_str());
        }
        else if (gArg.GetString("internalLegal", &legalizerPath))
        {
            // for std cell design only, since we don't have macro legalizer for now
            cout << "Calling internal legalizer: " << endl;
            if (gArg.CheckExist("3DIC")) {
                cout << "Global HPWL (3D): " << int(placedb->calcHPWL_3D()) << endl;
            } else {
                cout << "Global HPWL: " << int(placedb->calcHPWL()) << endl;
            }

            AbacusLegalizer *legalizer = new AbacusLegalizer(placedb);
            legalizer->legalization();

            if (gArg.CheckExist("3DIC")) {
                cout << "Legal HPWL (3D): " << int(placedb->calcHPWL_3D()) << endl;
            } else {
                cout << "Legal HPWL: " << int(placedb->calcHPWL()) << endl;
            }
            if (gArg.CheckExist("3DIC")) {
                PLOTTING::plotCurrentPlacement_3D("Cell legalized result", placedb);
            } else {
                PLOTTING::plotCurrentPlacement("Cell legalized result", placedb);
            }

            placedb->outputBookShelf("eLG",true);
            placedb->outputDEF("eLP",inital_def_path);
        }
        else
        {
            cout<<"Legalization not done!!!\n";
            exit(0);
        }
    }

    if (gArg.CheckExist("internalDP"))// currently only works after internal legalization
    {
        cout << "Calling internal detailed placement: " << endl;
        if (gArg.CheckExist("3DIC")) {
            cout << "HPWL before detailed placement (3D): " << int(placedb->calcHPWL_3D()) << endl;
        } else {
            cout << "HPWL before detailed placement: " << int(placedb->calcHPWL()) << endl;
        }

        DetailedPlacer *detailedPlacer = new DetailedPlacer(placedb);
        detailedPlacer->detailedPlacement();

        if (gArg.CheckExist("3DIC")) {
            cout << "HPWL after detailed placement (3D): " << int(placedb->calcHPWL_3D()) << endl;
        } else {
            cout << "HPWL after detailed placement: " << int(placedb->calcHPWL()) << endl;
        }
        if (gArg.CheckExist("3DIC")) {
            PLOTTING::plotCurrentPlacement_3D("Detailed placement result", placedb);
        } else {
            PLOTTING::plotCurrentPlacement("Detailed placement result", placedb);
        }

        placedb->outputBookShelf("eDP",false);
        placedb->outputDEF("eDP",inital_def_path);
    }
}