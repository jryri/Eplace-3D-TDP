#include <fstream>
#include <cstdlib>
#include <iostream>
#include "interface.h"


bool OpenroadInterface::run3DSTA(std::string staDEFPath)
{
    // 檢查 OpenROAD 執行檔是否存在
    if (system((DEFAULTopenroadPath + " -version > /dev/null 2>&1").c_str()) != 0) {
        std::cerr << "[ERROR] OpenROAD binary not found or not executable.\n";
        return false;
    }

    // 生成 eval_sta.tcl
    std::ofstream tclFile(DEFAULTtclPath);
    if (!tclFile.is_open()) {
        std::cerr << "[ERROR] Cannot open TCL path for writing: " << DEFAULTtclPath << "\n";
        return false;
    }

    tclFile << R"(
foreach libFile [glob "./ASAP7/LIB/*nldm*.lib"] {
    puts "lib: $libFile"
    read_liberty $libFile
}
read_liberty ./ASAP7/LIB/asapHBT.lib
puts "reading lef.."
read_lef ./ASAP7/techlef/asap7_tech_1x_201209.lef
foreach lef [glob "./ASAP7/LEF/*.lef"] {
    read_lef $lef
}
puts "reading def.."
read_def )" << staDEFPath << R"(

read_sdc ./testcase/aes_cipher_top/aes_cipher_top.sdc
source ./ASAP7/setRC.tcl

estimate_parasitics -placement

report_tns
report_wns
report_checks -slack_max 0 -endpoint_count 10000 
exit )";

    tclFile.close();

    // 組合系統命令
    std::stringstream cmd;
    cmd << DEFAULTopenroadPath << " " << DEFAULTtclPath << " > " << DEFAULTstaReportPath;

    std::cout << "[INFO] Running OpenROAD STA...\n";
    int ret = system(cmd.str().c_str());

    if (ret != 0) {
        std::cerr << "[ERROR] OpenROAD execution failed with code: " << ret << "\n";
        return false;
    }

    std::cout << "[INFO] STA completed. Report generated: " << DEFAULTstaReportPath << "\n";
    return true;
}

void OpenroadInterface::analyze3DSTAReport()
{
    // If staReportPath is empty, use the default path
    std::string staReportPath = DEFAULTstaReportPath;

    std::ifstream fin(staReportPath);
    if (!fin.is_open()) {
        std::cerr << "Failed to open " << staReportPath << "\n";
        return;
    }

    //reset p2p weights
    db->resetP2Pweights();

    std::string line;
    std::regex startpoint_regex("^Startpoint: (\\S+)");
    std::regex endpoint_regex("^Endpoint: (\\S+)");
    std::regex pin_regex("^\\s*[-+]?\\d+\\.\\d+\\s+\\d+\\.\\d+\\s+[v\\^]\\s+(\\S+)/(\\S+)");
    std::regex slack_regex("^\\s*(-?\\d+\\.\\d+)\\s+slack");

    std::string start_node, end_node;
    float slack = 0.0f;
    float WNS = DEFAULTWNS;//
    float TNS =0.0f;
    std::vector<std::pair<std::string, std::string>> pathPins;

    int NegativeSlackCount = 0;


    while (std::getline(fin, line)) {
        if (line.find("wns")!= std::string::npos) {
            std::istringstream iss(line);
            std::string wns_str;
            iss >> wns_str >> WNS; // Extract WNS value
            continue;
        }
        if (line.find("tns")!= std::string::npos) {
            std::istringstream iss(line);
            std::string tns_str;
            iss >> tns_str >> TNS; // Extract TNS value
            db->setTNS(TNS);
            continue;
        }
        std::smatch match;
        if (std::regex_search(line, match, startpoint_regex)) {
            pathPins.clear();  // reset
            start_node = match[1].str();
        }
        else if (std::regex_search(line, match, endpoint_regex)) {
            end_node = match[1].str();
        }
        else if (std::regex_search(line, match, pin_regex)) {
            std::string node = match[1].str();
            std::string pin = match[2].str();
            pathPins.emplace_back(node, pin);
        }
        else if (std::regex_search(line, match, slack_regex)) {
            slack = std::stof(match[1].str());
            if (slack < 0) {
                for (size_t i = 1; i < pathPins.size(); ++i) {
                auto [node1, pin1] = pathPins[i - 1];
                auto [node2, pin2] = pathPins[i];
                auto module1 = db->getModuleFromName(node1);
                auto module2 = db->getModuleFromName(node2);
                if (module1 == nullptr ) {
                    cout << "Module not found in database: " << node1 << endl;
                    exit(1);
                }
                if (module2 == nullptr) {
                    cout << "Module not found in database: " << node2 << endl;
                    exit(1);
                }
                // cout << "node1: " << node1 << " pin1: " << pin1 << " node2: " << node2 << " pin2: " << pin2 << " slack: " << slack << " WNS: " << WNS << endl;
                
                db->updateP2Pweight(node1, pin1, node2, pin2, slack, WNS);
                NegativeSlackCount++;

                }
            }
        }
    }
    std::cout << "Total negative slack count: " << NegativeSlackCount << std::endl;
    std::cout << "WNS: " << WNS << std::endl;
    std::cout << "TNS: " << TNS << std::endl;
    std::cout << "P2P weights updated based on STA report." << std::endl;
    // Close the file
    fin.close();
}

void OpenroadInterface::output3DSTADEF(std::string outputDEFPath)
{
    std::ifstream in(DEFAULTinitialDEFPath);
    if (!in.is_open()) {
        std::cerr << "Failed to open initial DEF file: " << DEFAULTinitialDEFPath << "\n";
        return;
    }

    std::ofstream out(outputDEFPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open output DEF file: " << outputDEFPath << "\n";
        return;
    }
    string line;
    bool inComponent = false;
    bool inNets = false;

    while (std::getline(in, line)) {
        
        if (line.find("COMPONENTS") != string::npos && !inComponent) {
            inComponent = true;
            string comp_line;
            stringstream ss(line);
            ss >> comp_line; // COMPONENTS
            int original_comp_count;
            ss >> original_comp_count;
            out << "COMPONENTS " << original_comp_count + db->dbHBTs.size() << " ;" << endl;
            continue;
        }

        if (line.find("END COMPONENTS") != string::npos && inComponent == true) {
            // Write virtual HBTs before ending the components section
            for(const auto& hbt : db->dbHBTs){
                out <<"- "<< hbt->name << " " << "HBTvia" << " + PLACED ( "
                        << hbt->getLocation().x << " " << hbt->getLocation().y << " ) N ;" << endl;
            }
            inComponent = false;
            out << line << endl;
            continue;
        }
        else if (inComponent) {
             if (line.find("PLACED") != string::npos) {
                stringstream ss(line);
                std::string tmp, nodeName , nodetype;
                ss >> tmp >> nodeName >> nodetype; // Get the node name

                string originalNodeName = nodeName;
                for (int i = nodeName.size() - 1; i >= 0; --i) {
                    if (nodeName[i] == '\\') {
                        nodeName.erase(i, 1);
                    }
                }
                auto module = db->getModuleFromName(nodeName);
                if (module) {
                     out <<"- "<< originalNodeName << " " << nodetype << " + PLACED ( "
                        << module->getLocation().x << " " << module->getLocation().y << " ) N ;" << endl;
                } else {
                    out << line << endl; // Keep original line if module not found
                }
            } else {
                 out << line << endl; // Copy non-PLACED lines within COMPONENTS
            }
            continue;
        }

        // Handle NETS section, replacing with data from DB
        if (line.find("NETS") != std::string::npos && !inComponent) {
            inNets = true; // Start of nets section in input file
            getline(in, line); // Skip original net count line

            // Traverse dbNets; for partitioned nets, expand to 2 virtual nets
            size_t net_lines = 0;
            for (const auto& net : db->dbNets) {
                if (net->isPartitioned && db->netToVNetIdx.count(net)) net_lines += 2;
                else net_lines += 1;
            }
            out << "NETS " << net_lines << " ;\n";

            for (const auto& net : db->dbNets) {
                if (net->isPartitioned && db->netToVNetIdx.count(net)) {
                    auto [botIdx, topIdx] = db->netToVNetIdx[net];
                    for (int idx : {botIdx, topIdx}) {
                        const auto& vnet = db->virtualNets[idx];
                        out << "- " << vnet.name;
                        for (const auto& pin : vnet.pins) {
                            std::string inst_name = pin->module->name;
                            for (int i = inst_name.size() - 1; i >= 0; --i) {
                                if (inst_name[i] == '[' || inst_name[i] == ']') inst_name.insert(i, 1, '\\');
                            }
                            out << " ( " << (pin->module->isFixed ? "PIN " : "")<<inst_name << " " << pin->name << " )";
                        }
                        out << " ;\n";
                    }
                } else {
                    out << "- " << net->name;
                    for (const auto& pin : net->netPins) {
                        std::string inst_name = pin->module->name;
                        for (int i = inst_name.size() - 1; i >= 0; --i) {
                            if (inst_name[i] == '[' || inst_name[i] == ']') inst_name.insert(i, 1, '\\');
                        }
                        out << " ( " << (pin->module->isFixed ? "PIN " : "")<<inst_name << " " << pin->name << " )";
                    }
                    out << " ;\n";
                }
            }
            out << "END NETS\n";
            while(getline(in,line) && line.find("END NETS") == string::npos); // Skip to the end of nets in input
            continue;
        }
        
        out << line << endl; // Write other lines as they are
    }
    in.close();
    out.close();
}