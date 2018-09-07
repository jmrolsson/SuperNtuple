// std
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <math.h>

// ROOT
#include "TChain.h"
#include "TVectorD.h"
#include "TRandom.h"
#include "TF1.h"

// SusyNtuple
#include "SusyNtuple/ChainHelper.h"
#include "SusyNtuple/string_utils.h"
#include "SusyNtuple/SusyNtSys.h"
#include "SusyNtuple/KinematicTools.h"

// Superflow
#include "Superflow/Superflow.h"
#include "Superflow/Superlink.h"
#include "Superflow/Cut.h"
#include "Superflow/StringTools.h"
#include "Superflow/input_options.h"

// lwtnn
#include "SuperNtuple/LightweightGraph.hh"
#include "SuperNtuple/parse_json.hh"

using namespace std;
using namespace sflow;

const string analysis_name = "ntupler_nn";
//const string nn_file = "/data/uclhc/uci/user/dantrim/lwtnn_test/my_model/nn_descriptor.json";
string network_dir = "/data/uclhc/uci/user/dantrim/n0234val/my_networks/";
//string network_dir = "./n0234val/my_networks/";
string nn_file_lowlvl = network_dir + "nn_descriptor_lowlvl.json";
string nn_file_hlvl = network_dir + "nn_descriptor_hlvl_completed.json";
string nn_file_hlvl_ttonly = network_dir + "nn_descriptor_hlvl_ttonly_completed.json";
string nn_file_hlvl_wt = network_dir + "nn_descriptor_hlvl_wt_completed.json";
//const string nn_file = "./n0234val/nn_descriptor.json";

int main(int argc, char* argv[])
{
    /////////////////////////////////////////////////////////////////////
    // Read in the command-line options (input file, num events, etc...)
    /////////////////////////////////////////////////////////////////////
    SFOptions options(argc, argv);
    options.ana_name = analysis_name;
    if(!read_options(options)) {
        exit(1);
    }

    TChain* chain = new TChain("susyNt");
    chain->SetDirectory(0);

    // load the NN
    //std::ifstream input_nn_file(nn_file);
    //std::string output_layer_name = "OutputLayer";
    //auto config = lwt::parse_json_graph(input_nn_file);
    //lwt::LightweightGraph nn_graph(config, output_layer_name);

    // load the NN files
    std::ifstream input_nn_file_lowlvl(nn_file_lowlvl);
    std::string output_layer_name = "OutputLayer";
    auto config_lowlvl = lwt::parse_json_graph(input_nn_file_lowlvl);
    lwt::LightweightGraph nn_graph_lowlvl(config_lowlvl, output_layer_name);

    std::ifstream input_nn_file_hlvl(nn_file_hlvl);
    auto config_hlvl = lwt::parse_json_graph(input_nn_file_hlvl);
    lwt::LightweightGraph nn_graph_hlvl(config_hlvl, output_layer_name);

    std::ifstream input_nn_file_hlvl_ttonly(nn_file_hlvl_ttonly);
    auto config_hlvl_ttonly = lwt::parse_json_graph(input_nn_file_hlvl_ttonly);
    lwt::LightweightGraph nn_graph_hlvl_ttonly(config_hlvl_ttonly, output_layer_name);

    std::ifstream input_nn_file_hlvl_wt(nn_file_hlvl_wt);
    auto config_hlvl_wt = lwt::parse_json_graph(input_nn_file_hlvl_wt);
    lwt::LightweightGraph nn_graph_hlvl_wt(config_hlvl_wt, output_layer_name);

    std::map<std::string, float> var_means;
    var_means["j0_pt"] = 134.898;
    var_means["j1_pt"] = 69.5677;
    var_means["bj0_pt"] = 122.180;
    var_means["bj1_pt"] = 54.1126;
    var_means["j0_eta"] = 0.0035159;
    var_means["j1_eta"] = -0.0014209;
    var_means["bj0_eta"] = -0.005168;
    var_means["bj1_eta"] = 0.00638;
    var_means["j0_phi"] = 0.0146455;
    var_means["j1_phi"] = 0.0051678;
    var_means["bj0_phi"] = 0.013698;
    var_means["bj1_phi"] = 0.011199;
    var_means["dphi_j0_ll"] = 0.0058626;
    var_means["dphi_j0_l0"] = -0.0030659;
    var_means["dphi_bj0_ll"] = 0.0086884;
    var_means["dphi_bj0_l0"] = -0.0016912;
    var_means["mbb"] = 144.59769;
    var_means["dRbb"] = 2.130620;
    var_means["dR_ll_bb"] = 2.815526;
    var_means["dphi_ll_bb"] = 0.00045855;
    var_means["dphi_WW_bb"] = -0.0093672;
    var_means["HT2"] = 279.0936;
    var_means["HT2Ratio"] = 0.63980;
    var_means["MT_1"] = 478.057;
    var_means["MT_1_scaled"] = 470.3389;
    var_means["mt2_llbb"] = 172.9586;
    var_means["mt2_bb"] = 66.25853;
    var_means["dphi_bb"] = -0.003595;
    var_means["mT_bb"] = 144.5976;

    bool verbose = true;
    ChainHelper::addInput(chain, options.input, verbose);
    Long64_t tot_num_events = chain->GetEntries();
    options.n_events_to_process = (options.n_events_to_process < 0 ? tot_num_events : options.n_events_to_process);

    ////////////////////////////////////////////////////
    // Construct and configure the Superflow object
    ////////////////////////////////////////////////////
    Superflow* cutflow = new Superflow();
    cutflow->setAnaName(options.ana_name);
    cutflow->setAnaType(AnalysisType::Ana_WWBB);
    cutflow->setLumi(1000); // 1/fb
    cutflow->setSampleName(options.input);
    cutflow->setRunMode(options.run_mode);
    cutflow->setCountWeights(true);
    cutflow->setChain(chain);
    cutflow->setDebug(options.dbg);
    if(options.suffix_name != "") {
        cutflow->setFileSuffix(options.suffix_name);
    }
    if(options.sumw_file_name != "") {
        cout << options.ana_name << "    Reading sumw for sample from file: " << options.sumw_file_name << endl;
        cutflow->setUseSumwFile(options.sumw_file_name);
    }
    cutflow->nttools().initTriggerTool(ChainHelper::firstFile(options.input, options.dbg));

    // print some useful
    cout << analysis_name << "    Total Entries    : " << chain->GetEntries() << endl;
    if(options.n_events_to_process > 0) {
    cout << analysis_name << "    Process Entries  : " << options.n_events_to_process << endl;
    } else {
    cout << analysis_name << "    Process Entries  : " << chain->GetEntries() << endl;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //
    // Superflow methods [BEGIN]
    //
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////

    *cutflow << CutName("read in ") << [](Superlink* /* sl */) -> bool { return true; };

    ////////////////////////////////////////////////////
    // Cleaning cuts
    ////////////////////////////////////////////////////
    int cutflags = 0;
    *cutflow << CutName("Pass GRL") << [&](Superlink* sl) -> bool {
        cutflags = sl->nt->evt()->cutFlags[NtSys::NOM];
        return (sl->tools->passGRL(cutflags));
    };
    *cutflow << CutName("LAr error") << [&](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(cutflags));
    };
    *cutflow << CutName("Tile Error") << [&](Superlink* sl) -> bool {
        return (sl->tools->passTileErr(cutflags));
    };
    *cutflow << CutName("SCT error") << [&](Superlink* sl) -> bool {
        return (sl->tools->passSCTErr(cutflags));
    };
    *cutflow << CutName("TTC veto") << [&](Superlink* sl) -> bool {
        return (sl->tools->passTTC(cutflags));
    };
    *cutflow << CutName("pass Good Vertex") << [&](Superlink * sl) -> bool {
        return (sl->tools->passGoodVtx(cutflags));
    };
    *cutflow << CutName("pass bad muon veto") << [&](Superlink* sl) -> bool {
        return (sl->tools->passBadMuon(sl->preMuons));
    };
    *cutflow << CutName("pass cosmic muon veto") << [&](Superlink* sl) -> bool {
        return (sl->tools->passCosmicMuon(sl->baseMuons));
    };
    *cutflow << CutName("pass jet cleaning") << [&](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };

//    *cutflow << CutName("pass met cleaning") << [&](Superlink* sl) -> bool {
//        return (sl->tools->passMetCleaning(sl->met));
//    };

    ///////////////////////////////////////////////////
    // Analysis Cuts
    ///////////////////////////////////////////////////
    *cutflow << CutName("exactly 2 signal leptons") << [](Superlink* sl) -> bool {
        return sl->leptons->size() == 2;
    };

//    #warning REMOVING LEPTON PT CUT
  //  *cutflow << CutName("lepton pTs > (25,20) GeV") << [](Superlink* sl) -> bool {
  //      return ( (sl->leptons->at(0)->Pt()>25) && (sl->leptons->at(1)->Pt()>20) );
  //  };

    *cutflow << CutName("opposite sign") << [](Superlink* sl) -> bool {
        return ((sl->leptons->at(0)->q * sl->leptons->at(1)->q) < 0);
    };

    *cutflow << CutName("mll > 20 GeV") << [](Superlink* sl) -> bool {
        return ( (*sl->leptons->at(0) + *sl->leptons->at(1)).M() > 20. );
    };

    *cutflow << CutName("veto SF Z-window (within 20 GeV)") << [](Superlink* sl) -> bool {
        bool pass = true;
        bool isSF = false;
        if((sl->leptons->size()==2 && (sl->electrons->size()==2 || sl->muons->size()==2))) isSF = true;
        if(isSF) {
            double mll = (*sl->leptons->at(0) + *sl->leptons->at(1)).M();
            if( fabs(mll-91.2) < 20. ) pass = false;
        }
        return pass;
    };

 //   #warning REMOVED CUT ON B-JETS
    *cutflow << CutName(">=1 b-tagged jets") << [](Superlink* sl) -> bool {
        int n_bjets = 0;
        for(auto j : (*sl->jets)) {
            if(sl->tools->jetSelector().isB(j)) { n_bjets++; }
        }
        if(n_bjets>=1) return true;
        else return false;
    };

    ///*
//    *cutflow << CutName("pass trigger requirement") << [&](Superlink* sl) -> bool {
//        int year = sl->nt->evt()->treatAsYear;
//        bool pass = false;
//
//        bool passes_mu18_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu18_mu8noL1");
//        bool passes_e17_lhloose_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_mu14");
//        bool passes_2e12_lhloose_L12EM10VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e12_lhloose_L12EM10VH");
//        bool passes_2e17_lhvloose_nod0 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e17_lhvloose_nod0");
//        bool passes_mu22_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu22_mu8noL1");
//        bool passes_e17_lhloose_nod0_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_nod0_mu14");
//
//        bool trig_pass_2015 = (passes_mu18_mu8noL1 || passes_e17_lhloose_mu14 || passes_2e12_lhloose_L12EM10VH);
//        bool trig_pass_2016 = (passes_2e17_lhvloose_nod0 || passes_mu22_mu8noL1 || passes_e17_lhloose_nod0_mu14);
//        if( (year==2015 && trig_pass_2015==1) || (year==2016 && trig_pass_2016==1) ) {
//            pass = true;
//        }
//        return pass;
//    };
    //*/
    
    //*cutflow << CutName("isSF") << [&](Superlink* sl) -> bool {
    //    bool isSF = false;
    //    bool zveto = false;
    //    if((sl->leptons->size()==2 && (sl->electrons->size()==2 || sl->muons->size()==2))) isSF = true;
    //    if(isSF) {
    //        double mll = (*sl->leptons->at(0) + *sl->leptons->at(1)).M();
    //        if(fabs(mll-91.2)>10.) zveto = true;
    //    }
    //    if(isSF && zveto) return true;
    //    else { return false; }
    //};
    //*cutflow << CutName("isDF") << [&](Superlink* sl) -> bool {
    //    bool isDF = false;
    //    if((sl->leptons->size()==2 && sl->electrons->size()==1 && sl->muons->size()==1)) isDF = true;
    //    if(isDF) return true;
    //    else { return false; }
    //};

    ///////////////////////////////////////////////////
    // Ntuple Setup
    ///////////////////////////////////////////////////


    // TRIGGERS
    bool pass_mu18_mu8noL1;
    bool pass_mu20_mu8noL1;
    bool pass_e17_lhloose_mu14;
    bool pass_2e12_lhloose_L12EM10VH;
    bool pass_2e15_lhvloose_L12EM13VH;
    // updated
    bool pass_2e17_lhvloose_nod0;
    bool pass_mu22_mu8noL1;
    bool pass_e17_lhloose_nod0_mu14;

    // for eff
    bool p_mu20;
    bool p_mu20_iloose_L1MU15;
//    bool p_mu22;
//    bool p_mu24;
    bool p_mu24_iloose;
    bool p_mu24_imedium;
    bool p_mu24_iloose_L1MU15;
    bool p_mu26_imedium;
    bool p_mu26_ivarmedium;
    bool p_mu40;
    bool p_mu50;
    bool p_mu60_0eta105_msonly;
    bool p_mu18_mu8noL1;
    bool p_mu20_mu8noL1;
    bool p_mu22_mu8noL1;
    bool p_mu24_mu8noL1;
//    bool p_e12_lhloose;
//    bool p_e12_lhloose_L1EM10VH;
//    bool p_e15_lhloose_L1EM13VH;
//    bool p_e17_lhloose;
//    bool p_e17_lhmedium;
    bool p_e24_lhmedium_iloose_L1EM18VH;
    bool p_e24_lhmedium_iloose_L1EM20VH;
    bool p_e24_lhmedium_L1EM20VHI;
    bool p_e24_lhtight_iloose;
    bool p_e24_lhtight_nod0_iloose;
    bool p_e24_lhmedium_nod0_iloose_L1EM20VH;
    bool p_e24_lhtight_ivarloose;
    bool p_e24_lhtight_nod0_ivarloose;
    bool p_e26_lhmedium_L1EM22VHI;
    bool p_e26_lhtight_iloose;
    bool p_e26_lhtight_nod0_iloose;
    bool p_e26_lhtight_ivarloose;
    bool p_e26_lhtight_nod0_ivarloose;
    bool p_e60_lhmedium;
    bool p_e60_lhmedium_nod0;
    bool p_e120_lhloose;
    bool p_e140_lhloose_nod0;
    bool p_2e12_lhloose_L12EM10VH;
    bool p_2e15_lhloose_L12EM13VH;
    bool p_2e15_lhvloose_L12EM13VH;
    bool p_2e15_lhvloose_nod0_L12EM13VH;
    bool p_2e17_lhloose;
    bool p_2e17_lhvloose;
    bool p_2e17_lhvloose_nod0;
    bool p_e17_lhloose_mu14;
    bool p_e17_lhloose_nod0_mu14;
    bool p_e7_lhmedium_mu24;
    bool p_e7_lhmedium_nod0_mu24;
    bool p_e26_lhmedium_L1EM22VHI_mu8noL1;
    bool p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1;
    bool p_e24_lhmedium_L1EM20VHI_mu8noL1;


    *cutflow << [&](Superlink* sl, var_void*) {

        // S2L

        pass_mu18_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu18_mu8noL1");
        pass_mu20_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu20_mu8noL1");
        pass_e17_lhloose_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_mu14");
        pass_2e12_lhloose_L12EM10VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e12_lhloose_L12EM10VH");
        pass_2e15_lhvloose_L12EM13VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e15_lhvloose_L12EM13VH");
        pass_2e17_lhvloose_nod0 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e17_lhvloose_nod0");
        pass_mu22_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu22_mu8noL1");
        pass_e17_lhloose_nod0_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_nod0_mu14"); 

        // eff
        p_mu20 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu20");
        p_mu20_iloose_L1MU15 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu20_iloose_L1MU15");
        //p_mu22 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu22");
        //p_mu24 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu24");
        p_mu24_iloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu24_iloose");
        p_mu24_imedium = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu24_imedium");
        p_mu24_iloose_L1MU15 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu24_iloose_L1MU15");
        p_mu26_imedium = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu26_imedium"); 
        p_mu26_ivarmedium = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu26_ivarmedium");
        p_mu40 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu40");
        p_mu50 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu50");
        p_mu60_0eta105_msonly = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu60_0eta105_msonly");
        p_mu18_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu18_mu8noL1");
        p_mu20_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu20_mu8noL1");
        p_mu22_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu22_mu8noL1");
        p_mu24_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_mu24_mu8noL1");
        //p_e12_lhloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e12_lhloose");
        //p_e12_lhloose_L1EM10VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e12_lhloose_L1EM10VH");
        //p_e15_lhloose_L1EM13VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e15_lhloose_L1EM13VH");
        //p_e17_lhloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose");
        //p_e17_lhmedium = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhmedium");
        p_e24_lhmedium_iloose_L1EM18VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhmedium_iloose_L1EM18VH");
        p_e24_lhmedium_iloose_L1EM20VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhmedium_iloose_L1EM20VH");
        p_e24_lhmedium_L1EM20VHI = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhmedium_L1EM20VHI");
        p_e24_lhtight_iloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhtight_iloose");
        p_e24_lhtight_nod0_iloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhtight_nod0_iloose");
        p_e24_lhmedium_nod0_iloose_L1EM20VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhmedium_nod0_iloose_L1EM20VH");
        p_e24_lhtight_ivarloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhtight_ivarloose");
        p_e24_lhtight_nod0_ivarloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhtight_nod0_ivarloose");

        p_e26_lhmedium_L1EM22VHI = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhmedium_L1EM22VHI");
        p_e26_lhtight_iloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhtight_iloose");
        p_e26_lhtight_nod0_iloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhtight_nod0_iloose");
        p_e26_lhtight_ivarloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhtight_ivarloose");
        p_e26_lhtight_nod0_ivarloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhtight_nod0_ivarloose");
    
        p_e60_lhmedium = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e60_lhmedium");
        p_e60_lhmedium_nod0 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e60_lhmedium_nod0");
        p_e120_lhloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e120_lhloose");
        p_e140_lhloose_nod0 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e140_lhloose_nod0");
        p_2e12_lhloose_L12EM10VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e12_lhloose_L12EM10VH");
        p_2e15_lhloose_L12EM13VH = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e15_lhloose_L12EM13VH");
        p_2e17_lhloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e17_lhloose");
        p_2e17_lhvloose = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e17_lhvloose");
        p_2e17_lhvloose_nod0 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_2e17_lhvloose_nod0");
        p_e17_lhloose_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_mu14");
        p_e17_lhloose_nod0_mu14 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_nod0_mu14");
        p_e7_lhmedium_mu24 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e7_lhmedium_mu24");
        p_e7_lhmedium_nod0_mu24 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e7_lhmedium_nod0_mu24");
        p_e26_lhmedium_L1EM22VHI_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhmedium_L1EM22VHI_mu8noL1");
        p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1");
        p_e24_lhmedium_L1EM20VHI_mu8noL1 = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e24_lhmedium_L1EM20VHI_mu8noL1");
        
    };
    *cutflow << NewVar("pass mu18_mu8noL1"); {
        *cutflow << HFTname("trig_mu18_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_mu18_mu8noL1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass mu20_mu8noL1"); {
        *cutflow << HFTname("trig_mu20_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_mu20_mu8noL1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass e17_lhloose_mu14"); {
        *cutflow << HFTname("trig_e17_lhloose_mu14");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_e17_lhloose_mu14;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass 2e12_lhloose_L12EM10VH"); {
        *cutflow << HFTname("trig_2e12_lhloose_L12EM10VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_2e12_lhloose_L12EM10VH;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass 2e15_lhvloose_L12EM13VH"); {
        *cutflow << HFTname("trig_2e15_lhvloose_L12EM13VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_2e15_lhvloose_L12EM13VH;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass OR 2015 trigger set"); {
        *cutflow << HFTname("trig_pass2015");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return (pass_mu18_mu8noL1 || pass_e17_lhloose_mu14 || pass_2e12_lhloose_L12EM10VH);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass OR 2016 trigger set"); {
        *cutflow << HFTname("trig_pass2016");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return (pass_mu20_mu8noL1 || pass_e17_lhloose_mu14 || pass_2e15_lhvloose_L12EM13VH);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("pass HLT_2e17_lhvloose_nod0"); {
        *cutflow << HFTname("trig_2e17_lhvloose_nod0");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_2e17_lhvloose_nod0;
        };
        *cutflow << SaveVar();
    } 
    *cutflow << NewVar("pass mu22_mu8noL1"); {
        *cutflow << HFTname("trig_mu22_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_mu22_mu8noL1;
        };
        *cutflow << SaveVar();
    } 
    *cutflow << NewVar("pass HLT_e17_lhloose_nod0_mu14"); {
        *cutflow << HFTname("trig_e17_lhloose_nod0_mu14");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return pass_e17_lhloose_nod0_mu14;
        };
        *cutflow << SaveVar();
    } 
    *cutflow << NewVar("pass OR UPDATED trigger set"); {
        *cutflow << HFTname("trig_pass2016update");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return (pass_2e17_lhvloose_nod0 || pass_mu22_mu8noL1 || pass_e17_lhloose_nod0_mu14); 
        };
        *cutflow << SaveVar();
    }


    // eff trig cuts
    *cutflow << NewVar("pass_mu20"); {
        *cutflow << HFTname("trig_mu20");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu20;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu20_iloose_L1MU15"); {
        *cutflow << HFTname("trig_mu20_iloose_L1MU15");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu20_iloose_L1MU15;
        };
        *cutflow << SaveVar();
    }


//    *cutflow << NewVar("pass_mu22"); {
//        *cutflow << HFTname("trig_mu22");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_mu22;
//        };
//        *cutflow << SaveVar();
//    }


//    *cutflow << NewVar("pass_mu24"); {
//        *cutflow << HFTname("trig_mu24");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_mu24;
//        };
//        *cutflow << SaveVar();
//    }


    *cutflow << NewVar("pass_mu24_iloose"); {
        *cutflow << HFTname("trig_mu24_iloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu24_iloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu24_imedium"); {
        *cutflow << HFTname("trig_mu24_imedium");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu24_imedium;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu24_iloose_L1MU15"); {
        *cutflow << HFTname("trig_mu24_iloose_L1MU15");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu24_iloose_L1MU15;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu26_imedium"); {
        *cutflow << HFTname("trig_mu26_imedium");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu26_imedium;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_HLT_mu26_ivarmedium"); {
        *cutflow << HFTname("trig_mu26_ivarmedium");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu26_ivarmedium;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu40"); {
        *cutflow << HFTname("trig_mu40");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu40;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu50"); {
        *cutflow << HFTname("trig_mu50");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu50;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu60_0eta105_msonly"); {
        *cutflow << HFTname("trig_mu60_0eta105_msonly");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu60_0eta105_msonly;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu18_mu8noL1"); {
        *cutflow << HFTname("trig_mu18_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu18_mu8noL1;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu20_mu8noL1"); {
        *cutflow << HFTname("trig_mu20_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu20_mu8noL1;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu22_mu8noL1"); {
        *cutflow << HFTname("trig_mu22_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu22_mu8noL1;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_mu24_mu8noL1"); {
        *cutflow << HFTname("trig_mu24_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_mu24_mu8noL1;
        };
        *cutflow << SaveVar();
    }


//    *cutflow << NewVar("pass_e12_lhloose"); {
//        *cutflow << HFTname("trig_e12_lhloose");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_e12_lhloose;
//        };
//        *cutflow << SaveVar();
//    }


//    *cutflow << NewVar("pass_e12_lhloose_L1EM10VH"); {
//        *cutflow << HFTname("trig_e12_lhloose_L1EM10VH");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_e12_lhloose_L1EM10VH;
//        };
//        *cutflow << SaveVar();
//    }


//    *cutflow << NewVar("pass_e15_lhloose_L1EM13VH"); {
//        *cutflow << HFTname("trig_e15_lhloose_L1EM13VH");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_e15_lhloose_L1EM13VH;
//        };
//        *cutflow << SaveVar();
//    }


//    *cutflow << NewVar("pass_e17_lhloose"); {
//        *cutflow << HFTname("trig_e17_lhloose");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_e17_lhloose;
//        };
//        *cutflow << SaveVar();
//    }


//    *cutflow << NewVar("pass_e17_lhmedium"); {
//        *cutflow << HFTname("trig_e17_lhmedium");
//        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
//            return p_e17_lhmedium;
//        };
//        *cutflow << SaveVar();
//    }


    *cutflow << NewVar("pass_e24_lhmedium_iloose_L1EM18VH"); {
        *cutflow << HFTname("trig_e24_lhmedium_iloose_L1EM18VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhmedium_iloose_L1EM18VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhmedium_iloose_L1EM20VH"); {
        *cutflow << HFTname("trig_e24_lhmedium_iloose_L1EM20VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhmedium_iloose_L1EM20VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhmedium_L1EM20VHI"); {
        *cutflow << HFTname("trig_e24_lhmedium_L1EM20VHI");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhmedium_L1EM20VHI;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhtight_iloose"); {
        *cutflow << HFTname("trig_e24_lhtight_iloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhtight_iloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhtight_nod0_iloose"); {
        *cutflow << HFTname("trig_e24_lhtight_nod0_iloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhtight_nod0_iloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhmedium_nod0_iloose_L1EM20VH"); {
        *cutflow << HFTname("trig_e24_lhmedium_nod0_iloose_L1EM20VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhmedium_nod0_iloose_L1EM20VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhtight_ivarloose"); {
        *cutflow << HFTname("trig_e24_lhtight_ivarloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhtight_ivarloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhtight_nod0_ivarloose"); {
        *cutflow << HFTname("trig_e24_lhtight_nod0_ivarloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhtight_nod0_ivarloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhmedium_L1EM22VHI"); {
        *cutflow << HFTname("trig_e26_lhmedium_L1EM22VHI");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhmedium_L1EM22VHI;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhtight_iloose"); {
        *cutflow << HFTname("trig_e26_lhtight_iloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhtight_iloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhtight_nod0_iloose"); {
        *cutflow << HFTname("trig_e26_lhtight_iloose_nod0");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhtight_nod0_iloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhtight_ivarloose"); {
        *cutflow << HFTname("trig_e26_lhtight_ivarloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhtight_ivarloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhtight_nod0_ivarloose"); {
        *cutflow << HFTname("trig_e26_lhtight_nod0_ivarloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhtight_nod0_ivarloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e60_lhmedium"); {
        *cutflow << HFTname("trig_e60_lhmedium");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e60_lhmedium;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e60_lhmedium_nod0"); {
        *cutflow << HFTname("trig_e60_lhmedium_nod0");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e60_lhmedium_nod0;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_e120_lhloose"); {
        *cutflow << HFTname("trig_e120_lhloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e120_lhloose;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_e140_lhloose_nod0"); {
        *cutflow << HFTname("trig_e140_lhloose_nod0");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e140_lhloose_nod0;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e12_lhloose_L12EM10VH"); {
        *cutflow << HFTname("trig_2e12_lhloose_L12EM10VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e12_lhloose_L12EM10VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e15_lhloose_L12EM13VH"); {
        *cutflow << HFTname("trig_2e15_lhloose_L12EM13VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e15_lhloose_L12EM13VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e15_lhvloose_L12EM13VH"); {
        *cutflow << HFTname("trig_2e15_lhvloose_L12EM13VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e15_lhvloose_L12EM13VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e15_lhvloose_nod0_L12EM13VH"); {
        *cutflow << HFTname("trig_2e15_lhvloose_nod0_L12EM13VH");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e15_lhvloose_nod0_L12EM13VH;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e17_lhloose"); {
        *cutflow << HFTname("trig_2e17_lhloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e17_lhloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e17_lhvloose"); {
        *cutflow << HFTname("trig_2e17_lhvloose");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e17_lhvloose;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_2e17_lhvloose_nod0"); {
        *cutflow << HFTname("trig_2e17_lhvloose_nod0");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_2e17_lhvloose_nod0;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e17_lhloose_mu14"); {
        *cutflow << HFTname("trig_e17_lhloose_mu14");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e17_lhloose_mu14;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e17_lhloose_nod0_mu14"); {
        *cutflow << HFTname("trig_e17_lhloose_nod0_mu14");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e17_lhloose_nod0_mu14;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e7_lhmedium_mu24"); {
        *cutflow << HFTname("trig_e7_lhmedium_mu24");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e7_lhmedium_mu24;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e7_lhmedium_nod0_mu24"); {
        *cutflow << HFTname("trig_e7_lhmedium_nod0_mu24");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e7_lhmedium_nod0_mu24;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e26_lhmedium_L1EM22VHI_mu8noL1"); {
        *cutflow << HFTname("trig_e26_lhmedium_L1EM22VHI_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhmedium_L1EM22VHI_mu8noL1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_e26_lhmedium_nod0_L1EM22VHI_mu8noL1"); {
        *cutflow << HFTname("trig_e26_lhmedium_nod0_L1EM22VHI_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_e26_lhmedium_nod0_L1EM22VHI_mu8noL1"); {
        *cutflow << HFTname("trig_e26_lhmedium_nod0_L1EM22VHI_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("pass_e24_lhmedium_L1EM20VHI_mu8noL1"); {
        *cutflow << HFTname("trig_e24_lhmedium_L1EM20VHI_mu8noL1");
        *cutflow << [&](Superlink* /*sl*/, var_bool*) -> bool {
            return p_e24_lhmedium_L1EM20VHI_mu8noL1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_trig_tight_2015"); {

        *cutflow << HFTname("trig_tight_2015");
        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
            if(sl->leptons->size()<2) return false;
            bool isEE = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isEle());
            bool isMM = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isMu());
            bool isEM = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isMu());
            bool isME = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isEle());

            float lead_pt = sl->leptons->at(0)->Pt();
            float sub_pt =  sl->leptons->at(1)->Pt();

            if(isEE) {
                if(lead_pt>=26 && p_e24_lhmedium_L1EM20VHI) {
                    return true;
                }
                else if( (lead_pt>=14 && sub_pt>=14) && p_2e12_lhloose_L12EM10VH) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isMM) {
                if(lead_pt>=22 && p_mu20_iloose_L1MU15) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=10) && p_mu18_mu8noL1) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isEM) {
                if(lead_pt>=26 && p_e24_lhmedium_L1EM20VHI) {
                    return true;
                }
                else if( (lead_pt>=26 && sub_pt>=10) && p_e24_lhmedium_L1EM20VHI_mu8noL1) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isME) {
                if(lead_pt>=22 && p_mu20_iloose_L1MU15) {
                    return true;
                }
                else if( (lead_pt>=26 && sub_pt>=10) && p_e7_lhmedium_mu24) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                cout << "ERROR in pass tight 2015!" << endl;
                exit(1);
            }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_trig_tight_2015dil"); {

        *cutflow << HFTname("trig_tight_2015dil");
        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
            if(sl->leptons->size()<2) return false;
            bool isEE = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isEle());
            bool isMM = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isMu());
            bool isEM = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isMu());
            bool isME = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isEle());

            float lead_pt = sl->leptons->at(0)->Pt();
            float sub_pt =  sl->leptons->at(1)->Pt();

            if(isEE) {
                if( (lead_pt>=14 && sub_pt>=14) && p_2e12_lhloose_L12EM10VH) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isMM) {
                if( (lead_pt>=20 && sub_pt>=10) && p_mu18_mu8noL1) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isEM) {
                if( (lead_pt>=26 && sub_pt>=10) && p_e24_lhmedium_L1EM20VHI_mu8noL1) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isME) {
                if( (lead_pt>=26 && sub_pt>=10) && p_e7_lhmedium_mu24) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                cout << "ERROR in pass tight 2015!" << endl;
                exit(1);
            }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_trig_tight_2016"); {

        *cutflow << HFTname("trig_tight_2016");
        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
            if(sl->leptons->size()<2) return false;
            bool isEE = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isEle());
            bool isMM = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isMu());
            bool isEM = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isMu());
            bool isME = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isEle());

            float lead_pt = sl->leptons->at(0)->Pt();
            float sub_pt =  sl->leptons->at(1)->Pt();

            if(isEE) {
                if(lead_pt>=28 && p_e26_lhtight_nod0_ivarloose) {
                    return true;
                }
                else if( (lead_pt>=19 && sub_pt>=19) && p_2e17_lhvloose_nod0) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isMM) {
                if(lead_pt>=28 && p_mu26_ivarmedium) {
                    return true;
                }
                else if( (lead_pt>=24 && sub_pt>=10) && p_mu22_mu8noL1) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isEM) {
                if(lead_pt>=28 && p_e26_lhtight_nod0_ivarloose) {
                    return true;
                }
                else if( (lead_pt>=28 && sub_pt>=10) && p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isME) {
                if(lead_pt>=28 && p_mu26_ivarmedium) {
                    return true;
                }
                else if( (lead_pt>=26 && sub_pt>=10) && p_e7_lhmedium_nod0_mu24) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                cout << "trig pass tight 2016 error!" << endl;
                exit(1);
            }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("pass_trig_tight_2016dil"); {

        *cutflow << HFTname("trig_tight_2016dil");
        *cutflow << [&](Superlink* sl, var_bool*) -> bool {
            if(sl->leptons->size()<2) return false;
            bool isEE = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isEle());
            bool isMM = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isMu());
            bool isEM = (sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isMu());
            bool isME = (sl->leptons->at(0)->isMu() &&  sl->leptons->at(1)->isEle());

            float lead_pt = sl->leptons->at(0)->Pt();
            float sub_pt =  sl->leptons->at(1)->Pt();

            if(isEE) {
                if( (lead_pt>=19 && sub_pt>=19) && p_2e17_lhvloose_nod0) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isMM) {
                if( (lead_pt>=24 && sub_pt>=10) && p_mu22_mu8noL1) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isEM) {
                if( (lead_pt>=28 && sub_pt>=10) && p_e26_lhmedium_nod0_L1EM22VHI_mu8noL1) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else if(isME) {
                if( (lead_pt>=26 && sub_pt>=10) && p_e7_lhmedium_nod0_mu24) {
                    return true;
                }
                else if( (lead_pt>=20 && sub_pt>=17) && p_e17_lhloose_mu14) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                cout << "trig pass tight 2016 error!" << endl;
                exit(1);
            }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("run"); {
        *cutflow << HFTname("runNumber");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            return sl->nt->evt()->run;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lumi block"); {
        *cutflow << HFTname("lumi_block");
        *cutflow << [](Superlink* sl, var_int*) -> int {
            return sl->nt->evt()->lb;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mcid"); {
        *cutflow << HFTname("mcid");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            return sl->nt->evt()->mcChannel;
        };
        *cutflow << SaveVar();
    }

    //*cutflow << NewVar("mc campaign (Susy::MCType)"); {
    //    *cutflow << HFTname("mcType");
    //    *cutflow << [](Superlink* sl, var_int*) -> int {
    //        return sl->nt->evt()->mcType;
    //    };
    //    *cutflow << SaveVar();
    //}

    *cutflow << NewVar("year"); {
        *cutflow << HFTname("year");
        *cutflow << [&](Superlink* sl, var_int*) -> int { 
            return sl->nt->evt()->treatAsYear;
        };
        *cutflow << SaveVar();
    }

    // standard variables
    *cutflow << NewVar("event weight"); {
        *cutflow << HFTname("eventweight");
        *cutflow << [&](Superlink* sl, var_double*) -> double {
            return sl->weights->product() * sl->nt->evt()->wPileup;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("event weight x btag SF"); {
        *cutflow << HFTname("eventweightbtag");
        *cutflow << [&](Superlink* sl, var_double*) -> double {
            return sl->weights->product() * sl->weights->btagSf;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pile-up weight"); {
        *cutflow << HFTname("pupw");
        *cutflow << [](Superlink* sl, var_double*) -> double {
            return sl->nt->evt()->wPileup;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pile-up weight (up variation)"); {
        *cutflow << HFTname("pupw_up");
        *cutflow << [](Superlink* sl, var_double*) -> double {
            return sl->nt->evt()->wPileup_up;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pile-up weight (down variation"); {
        *cutflow << HFTname("pupw_down");
        *cutflow << [](Superlink* sl, var_double*) -> double {
            return sl->nt->evt()->wPileup_dn;
        };
        *cutflow << SaveVar();
    }

    //*cutflow << NewVar("Pile-up weight period weight"); {
    //    *cutflow << HFTname("pupw_period");
    //    *cutflow << [](Superlink* sl, var_double*) -> double {
    //        return sl->nt->evt()->wPileup_period;
    //    };
    //    *cutflow << SaveVar();
    //}

    *cutflow << NewVar("is MC"); {
        *cutflow << HFTname("isMC");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of primary vertices"); {
        *cutflow << HFTname("nVtx");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->nVtx; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("average interactions per b.c."); {
        *cutflow << HFTname("avgMu");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->avgMu; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("average interactions per b.c. with data scale factor applied"); {
        *cutflow << HFTname("avgMuDataSF");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->avgMuDataSF; };
        *cutflow << SaveVar();
    } 

    //*cutflow << NewVar("actual interactions per b.c."); {
    //    *cutflow << HFTname("actualMu");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->actualMu; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("actual interactions per b.c. with data scale factor applied"); {
    //    *cutflow << HFTname("actualMuDataSF");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->actualMuDataSF; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("primary vertex X position"); {
    //    *cutflow << HFTname("pvX");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->pvX; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("primary vertex Y position"); {
    //    *cutflow << HFTname("pvY");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->pvY; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("primary vertex Z position"); {
    //    *cutflow << HFTname("pvZ");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->pvZ; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot X position"); {
    //    *cutflow << HFTname("beamSpotX");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosX; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot Y position"); {
    //    *cutflow << HFTname("beamSpotY");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosY; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot Z position"); {
    //    *cutflow << HFTname("beamSpotZ");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosZ; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot X position error"); {
    //    *cutflow << HFTname("beamPosSigmaX");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosSigmaX; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot Y position error"); {
    //    *cutflow << HFTname("beamPosSigmaY");
    //    *cutflow << [](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosSigmaY; };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("beam spot Z position error"); {
    //    *cutflow << HFTname("beamPosSigmaZ");
    //    *cutflow << [&](Superlink* sl, var_float*) -> double { return sl->nt->evt()->beamPosSigmaZ; };
    //    *cutflow << SaveVar();
    //}

//  //  TF1* pu_profile = new TF1("pu_profile", "gausn", -250, 250);
    //TF1 pu_profile("pu_profile", "gausn", -250, 250);

    //*cutflow << NewVar("pileup density"); {
    //    *cutflow << HFTname("pileup_density");
    //    *cutflow << [&](Superlink* sl, var_float*) -> double {
    //        float actual_mu = sl->nt->evt()->actualMu;
    //        float sigmaZ = sl->nt->evt()->beamPosSigmaZ;
    //        float beamPosZ = sl->nt->evt()->beamPosZ;
    //        float pvZ = sl->nt->evt()->pvZ;

    //        pu_profile.SetParameter(0, actual_mu);
    //        pu_profile.SetParameter(1, beamPosZ);
    //        pu_profile.SetParameter(2, sigmaZ);

    //        return pu_profile.Eval(pvZ);
    //    };
    //    *cutflow << SaveVar();
    //}

    //*cutflow << NewVar("number of tracks associated with primvary vertex"); {
    //    *cutflow << HFTname("nTracksAtPV");
    //    *cutflow << [](Superlink* sl, var_int*) -> int {
    //        return sl->nt->evt()->nTracksAtPV;
    //    };
    //    *cutflow << SaveVar();
    //}

    // lepton variables
    // lepton variables
    // lepton variables

    LeptonVector leptons;
    ElectronVector electrons;
    MuonVector muons;
    *cutflow << [&](Superlink* sl, var_void*) { leptons = *sl->leptons; };
    *cutflow << [&](Superlink* sl, var_void*) { electrons = *sl->electrons; };
    *cutflow << [&](Superlink* sl, var_void*) { muons = *sl->muons; };

   *cutflow << NewVar("number of leptons"); {
       *cutflow << HFTname("nLeptons");
       *cutflow << [&](Superlink* sl, var_int*) -> int { return leptons.size(); };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("number of electrons"); {
       *cutflow << HFTname("nElectrons");
       *cutflow << [&](Superlink* sl, var_int*) -> int { return electrons.size(); };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("number of muons"); {
       *cutflow << HFTname("nMuons");
       *cutflow << [&](Superlink* sl, var_int*) -> int { return muons.size(); };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("is an EE event"); {
       *cutflow << HFTname("isEE");
       *cutflow << [&](Superlink* sl, var_int*) -> int  {
           if(leptons.size()<2) return 0;
           if(leptons.at(0)->isEle() && leptons.at(1)->isEle()) { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }

   *cutflow << NewVar("is an MM event"); {
       *cutflow << HFTname("isMM");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return 0;
           if(leptons.at(0)->isMu() && leptons.at(1)->isMu()) { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("is an EM event"); {
       *cutflow << HFTname("isEM");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return 0;
           if(leptons.at(0)->isEle() && leptons.at(1)->isMu()) { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("is an ME event"); {
       *cutflow << HFTname("isME");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return 0;
           if(leptons.at(0)->isMu() && leptons.at(1)->isEle()) { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("is a SF event"); {
       *cutflow << HFTname("isSF");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return 0;
           if( (leptons.at(0)->isEle() && leptons.at(1)->isEle()) ||
               (leptons.at(0)->isMu() && leptons.at(1)->isMu()) ) { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("is a DF event"); {
       *cutflow << HFTname("isDF");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return 0;
           if( (leptons.at(0)->isEle() && leptons.at(1)->isMu()) ||
               (leptons.at(0)->isMu() && leptons.at(1)->isEle()) )  { return 1; }
           else { return 0; }
       };
       *cutflow << SaveVar();
   }
   *cutflow << NewVar("lepton flavor [EE=0,MM=1,EM=2,ME=3]"); {
       *cutflow << HFTname("l_flav");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return -1;
           bool e0 = leptons.at(0)->isEle();
           bool e1 = leptons.at(1)->isEle();

           if( e0 && e1 ) { return 0; }
           else if( !e0 && !e1 ) { return 1; }
           else if( e0 && !e1 ) { return 2; }
           else if( !e0 && e1 ) { return 3; }
           else { return -1; }
       };
       *cutflow << SaveVar();
   }

   *cutflow << NewVar("lead lepton flavor [E=0, M=1]"); {
       *cutflow << HFTname("l0_flav");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           bool e = leptons.at(0)->isEle();
           bool m = leptons.at(0)->isMu();

           if(e && !m) return 0;
           if(!e && m) return 1;
           else { return -1; }
       };
       *cutflow << SaveVar();
   }

   *cutflow << NewVar("sub lead lepton flavor [E=0, M=1]"); {
       *cutflow << HFTname("l1_flav");
       *cutflow << [&](Superlink* sl, var_int*) -> int {
           if(leptons.size()<2) return -1;
           bool e = leptons.at(1)->isEle();
           bool m = leptons.at(1)->isMu();
           if(e && !m) return 0;
           if(!e && m) return 1;
           else { return -1; }
       };
       *cutflow << SaveVar();
   }

    *cutflow << NewVar("lead lepton q"); {
        *cutflow << HFTname("l0_q");
        *cutflow << [&](Superlink* sl, var_int*) -> int { return leptons.at(0)->q; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lepton q"); {
        *cutflow << HFTname("l1_q");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            if(leptons.size()<2) return 0;
            return leptons.at(1)->q;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lepton d0"); {
        *cutflow << HFTname("l0_d0");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return leptons.at(0)->d0;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lepton d0"); {
        *cutflow << HFTname("l1_d0");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -10;
            return leptons.at(1)->d0;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lepton d0sig"); {
        *cutflow << HFTname("l0_d0sig");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->d0sigBSCorr; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lepton d0sig"); {
        *cutflow << HFTname("l1_d0sig");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -10;
            return leptons.at(1)->d0sigBSCorr;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lepton z0sinTheta"); {
        *cutflow << HFTname("l0_z0sinTheta");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->z0SinTheta(); };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lepton z0sinTheta"); {
        *cutflow << HFTname("l1_z0sinTheta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -10;
            return leptons.at(1)->z0SinTheta();
        };
        *cutflow << SaveVar();
    }

    // electron stuff
    *cutflow << NewVar("lead electron clusE"); {
        *cutflow << HFTname("e0_clusE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(0))->clusE;
            }
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead electron clusE"); {
        *cutflow << HFTname("e1_clusE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -1;
            if(leptons.at(1)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(1))->clusE;
            }
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead electron clusEtaBE"); {
        *cutflow << HFTname("e0_clusEtaBE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(0))->clusEtaBE;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub-lead electron clusEtaBE"); {
        *cutflow << HFTname("e1_clusEtaBE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(1))->clusEtaBE;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead electron clusPhiBE"); {
        *cutflow << HFTname("e0_clusPhiBE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(0))->clusPhiBE;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("sub-lead electron clusPhiBE"); {
        *cutflow << HFTname("e1_clusPhiBE");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(1))->clusPhiBE;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead electron track Pt"); {
        *cutflow << HFTname("e0_trackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(0))->trackPt;
            }
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub-lead electron track Pt"); {
        *cutflow << HFTname("e1_trackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -1;
            if(leptons.at(1)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(1))->trackPt;
            }
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead electron track Eta"); {
        *cutflow << HFTname("e0_trackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(0))->trackEta;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub-lead electron track Eta"); {
        *cutflow << HFTname("e1_trackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isEle()) {
                return static_cast<Susy::Electron*>(leptons.at(1))->trackEta;
            }
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    // muon stuff
    *cutflow << NewVar("lead muon ID track Pt"); {
        *cutflow << HFTname("mu0_idTrackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->idTrackPt;
            }
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead muon ID track Pt"); {
        *cutflow << HFTname("mu1_idTrackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -1;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->idTrackPt;
            }
            else return -1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon ID track Eta"); {
        *cutflow << HFTname("mu0_idTrackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->idTrackEta;
            }
            return -5;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead muon ID track Eta"); {
        *cutflow << HFTname("mu1_idTrackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->idTrackEta;
            }
            return -5;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon ID track Phi"); {
        *cutflow << HFTname("mu0_idTrackPhi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->idTrackPhi;
            } 
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("sublead muon ID track Phi"); {
        *cutflow << HFTname("mu1_idTrackPhi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->idTrackPhi;
            } 
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon ID q/p"); {
        *cutflow << HFTname("mu0_idTrackQoverP");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->idTrackQoverP;
            }
            else return -5;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead muon ID q/p"); {
        *cutflow << HFTname("mu1_idTrackQoverP");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->idTrackQoverP;
            }
            else return -5;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon MS track Pt"); {
        *cutflow << HFTname("mu0_msTrackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->msTrackPt;
            }
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead muon MS track Pt"); {
        *cutflow << HFTname("mu1_msTrackPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -1;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->msTrackPt;
            }
            else return -1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon MS track Eta"); {
        *cutflow << HFTname("mu0_msTrackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->msTrackEta;
            }
            return -5;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead muon MS track Eta"); {
        *cutflow << HFTname("mu1_msTrackEta");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->msTrackEta;
            }
            return -5;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon MS track Phi"); {
        *cutflow << HFTname("mu0_msTrackPhi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->msTrackPhi;
            } 
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("sublead muon MS track Phi"); {
        *cutflow << HFTname("mu1_msTrackPhi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->msTrackPhi;
            } 
            else { return -5; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead muon MS q/p"); {
        *cutflow << HFTname("mu0_msTrackQoverP");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.at(0)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(0))->msTrackQoverP;
            }
            else return -5;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead muon MS q/p"); {
        *cutflow << HFTname("mu1_msTrackQoverP");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(leptons.size()<2) return -5;
            if(leptons.at(1)->isMu()) {
                return static_cast<Susy::Muon*>(leptons.at(1))->msTrackQoverP;
            }
            else return -5;
        };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("lead lepton pt"); {
        *cutflow << HFTname("l0_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("sublead lepton pt"); {
        *cutflow << HFTname("l1_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->Pt();
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep etconetopo20"); {
        *cutflow << HFTname("l0_etconetopo20");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->etconetopo20; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep etconetopo20"); {
        *cutflow << HFTname("l1_etconetopo20");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->etconetopo20;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep etconetopo30"); {
        *cutflow << HFTname("l0_etconetopo30");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->etconetopo30; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep etconetopo30"); {
        *cutflow << HFTname("l1_etconetopo30");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->etconetopo30;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep ptcone20"); {
        *cutflow << HFTname("l0_ptcone20");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->ptcone20; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep ptcone20"); {
        *cutflow << HFTname("l1_ptcone20");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->ptcone20;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep ptcone30"); {
        *cutflow << HFTname("l0_ptcone30");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->ptcone30; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep ptcone30"); {
        *cutflow << HFTname("l1_ptcone30");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->ptcone30;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep ptvarcone20"); {
        *cutflow << HFTname("l0_ptvarcone20");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->ptvarcone20; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep ptvarcone20"); {
        *cutflow << HFTname("l1_ptvarcone20");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->ptvarcone20;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep ptvarcone30"); {
        *cutflow << HFTname("l0_ptvarcone30");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->ptvarcone30; };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep ptvarcone30"); {
        *cutflow << HFTname("l1_ptvarcone30");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return leptons.at(1)->ptvarcone30;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep eta"); {
        *cutflow << HFTname("l0_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->Eta(); };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep eta"); {
        *cutflow << HFTname("l1_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -5;
            return leptons.at(1)->Eta();
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead lep phi"); {
        *cutflow << HFTname("l0_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return leptons.at(0)->Phi(); };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sublead lep phi"); {
        *cutflow << HFTname("l1_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -5;
            return leptons.at(1)->Phi();
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mll leptons"); {
        *cutflow << HFTname("mll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double mll = -10.0;
            if(leptons.size() == 2) {
                Lepton* l0 = leptons.at(0);
                Lepton* l1 = leptons.at(1);
                mll = (*l0 + *l1).M();
            }
            return mll;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("dilepton pT"); {
        *cutflow << HFTname("pTll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double pTll = -10.0;
            if(leptons.size() == 2) {
                Lepton* l0 = leptons.at(0);
                Lepton* l1 = leptons.at(1);
                pTll = (*l0 + *l1).Pt();
            }
            return pTll;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("delta phi between to leptons"); {
        *cutflow << HFTname("dphi_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double dphi = -10.0;
            if(leptons.size() == 2) {
                Lepton l0 = *leptons.at(0);
                Lepton l1 = *leptons.at(1);
                dphi = l0.DeltaPhi(l1);
            }
            return dphi;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("delta eta between two leptons"); {
        *cutflow << HFTname("deta_ll");
        *cutflow << [&](Superlink* /* sl */, var_float*) -> double {
            double deta = -10.0;
            if(leptons.size() == 2) {
                Lepton l0 = *leptons.at(0);
                Lepton l1 = *leptons.at(1);
                deta = l0.Eta() - l1.Eta();
            }
            return deta;
        };
        *cutflow << SaveVar();
    }

    // jet variables
    // jet variables
    // jet variables

    JetVector jets;
    JetVector bjets;
    JetVector sjets;

    *cutflow << [&](Superlink* sl, var_void*) { jets = *sl->jets; };
    *cutflow << [&](Superlink* sl, var_void*) {
        for(int i = 0; i < jets.size(); i++) {
            Jet* j = jets[i];
            if(sl->tools->jetSelector().isBMod(j, 77))  bjets.push_back(j);
            else { sjets.push_back(j); }
        }// i
    };

    *cutflow << NewVar("lead jet jvt"); {
        *cutflow << HFTname("j0_jvt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->jvt;
            else { return -10; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead sjet jvt"); {
        *cutflow << HFTname("sj0_jvt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->jvt;
            else { return -10; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead bjet jvt"); {
        *cutflow << HFTname("bj0_jvt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->jvt;
            else { return -10; }
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet nTracks"); {
        *cutflow << HFTname("j0_nTracks");
        *cutflow << [&](Superlink* /*sl*/, var_int*) -> int {
            if(jets.size()>0) return jets.at(0)->nTracks;
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sjet nTracks"); {
        *cutflow << HFTname("sj0_nTracks");
        *cutflow << [&](Superlink* /*sl*/, var_int*) -> int {
            if(sjets.size()>0) return sjets.at(0)->nTracks;
            else { return -1; }
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("bjet nTracks"); {
        *cutflow << HFTname("bj0_nTracks");
        *cutflow << [&](Superlink* /*sl*/, var_int*) -> int {
            if(bjets.size()>0) return bjets.at(0)->nTracks;
            else { return -1; }
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("jet sumTrkPt"); {
        *cutflow << HFTname("j0_sumTrkPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->sumTrkPt;
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sjet sumTrkPt"); {
        *cutflow << HFTname("sj0_sumTrkPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->sumTrkPt;
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("bjet sumTrkPt"); {
        *cutflow << HFTname("bj0_sumTrkPt");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->sumTrkPt;
            else return -1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet mv2c10"); {
        *cutflow << HFTname("j0_mv2c10");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->mv2c10;
            else return -10;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sjet mv2c10"); {
        *cutflow << HFTname("sj0_mv2c10");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->mv2c10;
            else return -10;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("bjet mv2c10"); {
        *cutflow << HFTname("bj0_mv2c10");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->mv2c10;
            else return -10;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet emfrac"); {
        *cutflow << HFTname("j0_emfrac");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->emfrac;
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sjet emfrac"); {
        *cutflow << HFTname("sj0_emfrac");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->emfrac;
            else return -1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("bjet emfrac"); {
        *cutflow << HFTname("bj0_emfrac");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->emfrac;
            else return -1;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of jets"); {
        *cutflow << HFTname("nJets");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            return jets.size();
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("number of sjets"); {
        *cutflow << HFTname("nSJets");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            return sjets.size();
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of bjets"); {
        *cutflow << HFTname("nBJets");
        *cutflow << [&](Superlink* sl, var_int*) -> int {
            return bjets.size();
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("lead jet pt"); {
        *cutflow << HFTname("j0_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->Pt();
            else return var_means.at("j0_pt");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead jet pt"); {
        *cutflow << HFTname("j1_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>1) return jets.at(1)->Pt();
            else return var_means.at("j1_pt");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead jet pt"); {
        *cutflow << HFTname("j2_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>2) return jets.at(2)->Pt();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead sjet pt"); {
        *cutflow << HFTname("sj0_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->Pt();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead sjet pt"); {
        *cutflow << HFTname("sj1_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>1) return sjets.at(1)->Pt();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead sjet pt"); {
        *cutflow << HFTname("sj2_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>2) return sjets.at(2)->Pt();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead bjet pt"); {
        *cutflow << HFTname("bj0_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->Pt();
            else return var_means.at("bj0_pt");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead bjet pt"); {
        *cutflow << HFTname("bj1_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>1) return bjets.at(1)->Pt();
            else return var_means.at("bj1_pt");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead bjet pt"); {
        *cutflow << HFTname("bj2_pt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>2) return bjets.at(2)->Pt();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead jet eta"); {
        *cutflow << HFTname("j0_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->Eta();
            else return var_means.at("j0_eta");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead jet eta"); {
        *cutflow << HFTname("j1_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>1)  return jets.at(1)->Eta();
            else return var_means.at("j1_eta");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead jet eta"); {
        *cutflow << HFTname("j2_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>2)  return jets.at(2)->Eta();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("lead sjet eta"); {
        *cutflow << HFTname("sj0_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->Eta();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead sjet eta"); {
        *cutflow << HFTname("sj1_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>1) return sjets.at(1)->Eta();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead sjet eta"); {
        *cutflow << HFTname("sj2_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>2) return sjets.at(2)->Eta();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead bjet eta"); {
        *cutflow << HFTname("bj0_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->Eta();
            else return var_means.at("bj0_eta");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead bjet eta"); {
        *cutflow << HFTname("bj1_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>1) return bjets.at(1)->Eta();
            else return var_means.at("bj1_eta");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead bjet eta"); {
        *cutflow << HFTname("bj2_eta");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>2) return bjets.at(2)->Eta();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead jet phi"); {
        *cutflow << HFTname("j0_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>0) return jets.at(0)->Phi();
            else return var_means.at("j0_phi");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead jet phi"); {
        *cutflow << HFTname("j1_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>1) return jets.at(1)->Phi();
            else return var_means.at("j1_phi");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead jet phi"); {
        *cutflow << HFTname("j2_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(jets.size()>2) return jets.at(2)->Phi();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead sjet phi"); {
        *cutflow << HFTname("sj0_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>0) return sjets.at(0)->Phi();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead sjet phi"); {
        *cutflow << HFTname("sj1_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>1)  return sjets.at(1)->Phi();
            else return -10.;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead sjet phi"); {
        *cutflow << HFTname("sj2_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(sjets.size()>2) return sjets.at(2)->Phi();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lead bjet phi"); {
        *cutflow << HFTname("bj0_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>0) return bjets.at(0)->Phi();
            else return var_means.at("bj0_phi");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("sub lead bjet phi"); {
        *cutflow << HFTname("bj1_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>1) return bjets.at(1)->Phi();
            else return var_means.at("bj1_phi");
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("third lead bjet phi"); {
        *cutflow << HFTname("bj2_phi");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>2) return bjets.at(2)->Phi();
            else return -10.;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta phi between dilepton system and leading jet"); {
        *cutflow << HFTname("dphi_j0_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = var_means.at("dphi_j0_ll");
            if(jets.size()>0 && leptons.size()>=2) {
                TLorentzVector l0, l1, ll;
                l0.SetPtEtaPhiM(leptons.at(0)->Pt(), leptons.at(0)->Eta(), leptons.at(0)->Phi(), leptons.at(0)->M());
                l1.SetPtEtaPhiM(leptons.at(1)->Pt(), leptons.at(1)->Eta(), leptons.at(1)->Phi(), leptons.at(1)->M());
                ll = l0 + l1;
                out = jets.at(0)->DeltaPhi(ll);
            }
            return out;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("delta phi between leading lepton and leading sjet"); {
        *cutflow << HFTname("dphi_j0_l0");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = var_means.at("dphi_j0_l0");
            if(jets.size()>0) {
                out = jets.at(0)->DeltaPhi(*leptons.at(0));
            }
            return out;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta phi between dilepton system and leading sjet"); {
        *cutflow << HFTname("dphi_sj0_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = -10;
            if(sjets.size()>0 && leptons.size()>=2) {
                TLorentzVector l0, l1, ll;
                l0.SetPtEtaPhiM(leptons.at(0)->Pt(), leptons.at(0)->Eta(), leptons.at(0)->Phi(), leptons.at(0)->M());
                l1.SetPtEtaPhiM(leptons.at(1)->Pt(), leptons.at(1)->Eta(), leptons.at(1)->Phi(), leptons.at(1)->M());
                ll = l0 + l1;
                out = sjets.at(0)->DeltaPhi(ll);
            }
            return out;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("delta phi between leading lepton and leading sjet"); {
        *cutflow << HFTname("dphi_sj0_l0");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = -10;
            if(sjets.size()>0) {
                out = sjets.at(0)->DeltaPhi(*leptons.at(0));
            }
            return out;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta phi between dilepton system and leading bjet"); {
        *cutflow << HFTname("dphi_bj0_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = var_means.at("dphi_bj0_ll");
            if(bjets.size()>0 && leptons.size()>=2) {
                TLorentzVector l0, l1, ll;
                l0.SetPtEtaPhiM(leptons.at(0)->Pt(), leptons.at(0)->Eta(), leptons.at(0)->Phi(), leptons.at(0)->M());
                l1.SetPtEtaPhiM(leptons.at(1)->Pt(), leptons.at(1)->Eta(), leptons.at(1)->Phi(), leptons.at(1)->M());
                ll = l0 + l1;
                out = bjets.at(0)->DeltaPhi(ll);
            }
            return out;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("delta phi between leading lepton and leading bjet"); {
        *cutflow << HFTname("dphi_bj0_l0");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double out = var_means.at("dphi_bj0_l0");
            if(bjets.size()>0) {
                out = bjets.at(0)->DeltaPhi(*leptons.at(0));
            }
            return out;
        };
        *cutflow << SaveVar();
    }
    // met variables
    // met variables
    // met variables
    Met met;
    *cutflow << [&](Superlink* sl, var_void*) { met = *sl->met; };
    *cutflow << NewVar("transverse missing energy (Etmiss)"); {
        *cutflow << HFTname("met");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return met.lv().Pt(); };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("phi coord. of Etmiss"); {
        *cutflow << HFTname("metPhi");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return met.lv().Phi(); };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met TST"); {
        *cutflow << HFTname("metTST");
        *cutflow << [&](Superlink* sl, var_float*) -> double { return met.softTerm_et; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta phi between dilepton system and met"); {
        *cutflow << HFTname("dphi_met_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -5;
            TLorentzVector l0, l1, ll;
            l0.SetPtEtaPhiM(leptons.at(0)->Pt(), leptons.at(0)->Eta(), leptons.at(0)->Phi(), leptons.at(0)->M());
            l1.SetPtEtaPhiM(leptons.at(1)->Pt(), leptons.at(1)->Eta(), leptons.at(1)->Phi(), leptons.at(1)->M());
            ll = l0 + l1;
            return met.lv().DeltaPhi(ll);
        };
        *cutflow << SaveVar();
    }

    // MET terms
    *cutflow << NewVar("met_ele_et"); {
        *cutflow << HFTname("met_ele_et");   
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refEle_et;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_ele_phi"); {
        *cutflow << HFTname("met_ele_phi");   
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refEle_phi;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_ele_sumet"); {
        *cutflow << HFTname("met_ele_sumet");   
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refEle_sumet;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("met_jet_et"); {
        *cutflow << HFTname("met_jet_et");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refJet_et;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_jet_phi"); {
        *cutflow << HFTname("met_jet_phi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refJet_phi;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_jet_sumet"); {
        *cutflow << HFTname("met_jet_sumet");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refJet_sumet;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_muo_et"); {
        *cutflow << HFTname("met_muo_et");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refMuo_et;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_muo_phi"); {
        *cutflow << HFTname("met_muo_phi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refMuo_phi;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_muo_sumet"); {
        *cutflow << HFTname("met_muo_sumet");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.refMuo_sumet;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_soft_et"); {
        *cutflow << HFTname("met_soft_et");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.softTerm_et;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_soft_phi"); {
        *cutflow << HFTname("met_soft_phi");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.softTerm_phi;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("met_soft_sumet"); {
        *cutflow << HFTname("met_soft_sumet");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            return met.softTerm_sumet;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("mt2"); {
        *cutflow << HFTname("mt2");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double mt2 = -10.0;
            if(leptons.size() == 2) {
                mt2 = kin::getMT2(*sl->leptons, *sl->met);
            }
            return mt2;
        };
        *cutflow << SaveVar();
    }

    double meff;
    *cutflow << NewVar("meff : scalar sum pt of all jets, leptons, and met"); {
        *cutflow << HFTname("meff");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            meff = 0.0;
            // met
            meff += met.lv().Pt();
            // jets
            for(unsigned int ij = 0; ij < jets.size(); ij++){
                meff += jets.at(ij)->Pt();
            }
            // leptons
            for(unsigned int il=0; il < leptons.size(); il++){
                meff += leptons.at(il)->Pt();
            }
            return meff;
        };
        *cutflow << SaveVar();
    }
    double meff_S2L;
    *cutflow << NewVar("meff S2L : scalar sum pt of leptons, met, and up to two jets"); {
        *cutflow << HFTname("meff_S2L");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            meff_S2L = 0.0;
            // met
            meff_S2L += met.lv().Pt();
            // leptons
            for(int il=0; il < (int)leptons.size(); il++){
                meff_S2L += leptons.at(il)->Pt();
            }
            // jets
            int n_j = 0;
            for(int ij = 0; ij < (int)jets.size(); ij++){
                if(n_j < 2) {
                    meff_S2L += jets.at(ij)->Pt();
                    n_j++;
                }
            }
            return meff_S2L;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("R1 : met / meff"); {
        *cutflow << HFTname("R1");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double R1 = -10.0;
            if(meff>0.0) {
                R1 = met.lv().Pt() / meff * 1.0;
            }
            return R1;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("R1 S2L : met / meff_S2L"); {
        *cutflow << HFTname("R1_S2L");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double R1_S2L = -10.0;
            if(meff_S2L>0.0) {
                R1_S2L = met.lv().Pt() / (meff_S2L * 1.0);
            }
            return R1_S2L;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("R2 : met / (met + l0pt + l1pt)"); {
        *cutflow << HFTname("R2");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double R2 = -10.0;
            if(leptons.size() == 2) {
                double denom = met.lv().Pt() + leptons.at(0)->Pt() + leptons.at(1)->Pt();
                R2 = met.lv().Pt() / denom * 1.0;
            }
            return R2;
        };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("cosThetaB (WW-like)"); {
        *cutflow << HFTname("cosThetaB");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            double cosThetaB = -10;
            if(leptons.size()==2) {
                TLorentzVector lp, lm;
                for(int il = 0; il < (int)leptons.size(); il++) {
                    if(leptons.at(il)->q < 0) lm = *leptons.at(il);
                    else if(leptons.at(il)->q > 0) lp = *leptons.at(il);
                } // il
                TLorentzVector ll = lp+lm;
                TVector3 boost = ll.BoostVector();
                lp.Boost(-boost);
                lm.Boost(-boost);
                cosThetaB = tanh((lp.Eta()-lm.Eta())/2.);
            }
            return cosThetaB;
        };
        *cutflow << SaveVar();
    }


    ///////////////////////////////////////////////
    // WWBB variables
    ///////////////////////////////////////////////

    // angles
    // dRll
    *cutflow << NewVar("delta R between two leptons"); {
        *cutflow << HFTname("dRll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return (leptons.at(0)->DeltaR(*leptons.at(1)));
        };
        *cutflow << SaveVar();
    }

    // M_bb
    *cutflow << NewVar("invariant mass of di-bjet system"); {
        *cutflow << HFTname("mbb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2) {
                return (*bjets.at(0) + *bjets.at(1)).M();
            }
            return var_means.at("mbb");
        };
        *cutflow << SaveVar();
    }

    // dRbb
    *cutflow << NewVar("delta R between two leading b-jets"); {
        *cutflow << HFTname("dRbb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2) {
                return (bjets.at(0)->DeltaR(*bjets.at(1)));
            }
            return var_means.at("dRbb");
        };
        *cutflow << SaveVar();
    }


    // dR_ll_bb
    *cutflow << NewVar("delta R between dilepton system and di-bjet system"); {
        *cutflow << HFTname("dR_ll_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                TLorentzVector l0 = (*leptons.at(0));
                TLorentzVector l1 = (*leptons.at(1));
                TLorentzVector b0 = (*bjets.at(0));
                TLorentzVector b1 = (*bjets.at(1));

                return ( (l0 + l1).DeltaR( (b0 + b1) ) );
            }
            return var_means.at("dR_ll_bb");
        };
        *cutflow << SaveVar();
    }

    // dphi bb  ll
    *cutflow << NewVar("delta phi between bb and ll systems"); {
        *cutflow << HFTname("dphi_ll_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                return ( (*bjets.at(0) + *bjets.at(1)).DeltaPhi( (*leptons.at(0) + *leptons.at(1)) ) );
            }
            return var_means.at("dphi_ll_bb");
        };
        *cutflow << SaveVar();
    }

    // dphi WW bb
    *cutflow << NewVar("delta phi between WW and bb systems"); {
        *cutflow << HFTname("dphi_WW_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                return ( (met.lv() + *leptons.at(0) + *leptons.at(1)).DeltaPhi( (*bjets.at(0) + *bjets.at(1)) ) ); 
            }
            return var_means.at("dphi_WW_bb");
        };
        *cutflow << SaveVar();
    }

    // dphi_met_ll
    *cutflow << NewVar("delta phi between MET and dilepton system"); {
        *cutflow << HFTname("dphi_met_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -5;
            return ( (met.lv().DeltaPhi( (*leptons.at(0) + *leptons.at(1)) )) );
        };
        *cutflow << SaveVar();
    }

    // mass_met_ll
    *cutflow << NewVar("mass of met and dilepton system"); {
        *cutflow << HFTname("mass_met_ll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return ( (met.lv() + *leptons.at(0) + *leptons.at(1)).M() );
        };
        *cutflow << SaveVar();
    }

    // mass_met_ll_T
    *cutflow << NewVar("mass of met and dilepton system transv"); {
        *cutflow << HFTname("mass_met_ll_T");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            TLorentzVector l0 = (*leptons.at(0));
            TLorentzVector l1 = (*leptons.at(1));
            l0.SetPz(0.0);
            l1.SetPz(0.0);
            return ( ( met.lv() + l0 + l1).M() );
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("mass of met and dilepton system transv"); {
        *cutflow << HFTname("mass_met_ll_T_2");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -5;
            TLorentzVector l0 = (*leptons.at(0));
            TLorentzVector l1 = (*leptons.at(1));
            return ( ( met.lv() + l0 + l1).Mt() );
        };
        *cutflow << SaveVar();
    }

    // met_pTll
    *cutflow << NewVar("pT of met + dilepton ssytem"); {
        *cutflow << HFTname("met_pTll");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            return ( (met.lv() + *leptons.at(0) + *leptons.at(1)).Pt() );
        };
        *cutflow << SaveVar();
    }

    // HT2
    *cutflow << NewVar("HT2"); {
        *cutflow << HFTname("HT2");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                double HT2 = ( (*bjets.at(0) + *bjets.at(1)).Pt() + 
                    (*leptons.at(0) + *leptons.at(1) + met.lv()).Pt() );
                return HT2;
            }
            return var_means.at("HT2");
        };
        *cutflow << SaveVar();
    }

    // HT2Ratio
    *cutflow << NewVar("HT2Ratio"); {
        *cutflow << HFTname("HT2Ratio");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {

                double num = ( (*bjets.at(0) + *bjets.at(1)).Pt() + 
                    (*leptons.at(0) + *leptons.at(1) + met.lv()).Pt() );

                double den = ((*bjets.at(0)).Pt());
                den += (*bjets.at(1)).Pt();
                den += (*leptons.at(0)).Pt();
                den += (*leptons.at(1)).Pt();
                den += met.lv().Pt();

                return (num / den);
            }
            return var_means.at("HT2Ratio");
        };
        *cutflow << SaveVar();
    }


    // MT_HWW
    *cutflow << NewVar("MT_HWW"); {
        *cutflow << HFTname("MT_HWW");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            double ptll_met = (*leptons.at(0) + *leptons.at(1) + met.lv()).Pt(); 
            double ptll2 = (*leptons.at(0) + *leptons.at(1)).Pt();
            ptll2 = ptll2 * ptll2;
            double mll2 = (*leptons.at(0) + *leptons.at(1)).M();
            mll2 = mll2 * mll2;
            double ET_ll = sqrt(ptll2 + mll2);
            return sqrt( (ET_ll + met.lv().Pt())*(ET_ll + met.lv().Pt()) - (ptll_met * ptll_met) );
        };
        *cutflow << SaveVar();
    }

    // MT_1
    *cutflow << NewVar("MT_1"); {
        *cutflow << HFTname("MT_1");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                TLorentzVector vis = (*leptons.at(0) + *leptons.at(1)
                            + *bjets.at(0) + *bjets.at(1));
                double pt_vis = vis.Pt();
                double m_vis = vis.M();
                double et_vis = sqrt(pt_vis * pt_vis + m_vis * m_vis);
                return sqrt( (et_vis + met.lv().Pt()) * (et_vis + met.lv().Pt()) -
                            ( (vis + met.lv()).Pt() * (vis + met.lv()).Pt() ) );
            }
            return var_means.at("MT_1");
        };
        *cutflow << SaveVar();
    }

    // MT_1_scaled
    *cutflow << NewVar("MT_1_scaled"); {
        *cutflow << HFTname("MT_1_scaled");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                TLorentzVector bjet_system = (*bjets.at(0) + *bjets.at(1));
                double m_bb = bjet_system.M();
                double scaling = 125.09/m_bb;
                bjet_system.SetPtEtaPhiE(bjet_system.Pt() * scaling, bjet_system.Eta(), bjet_system.Phi(), bjet_system.E() * scaling);
                TLorentzVector vis = (*leptons.at(0) + *leptons.at(1) + bjet_system);

                double pt_vis = vis.Pt();
                double m_vis = vis.M();
                double et_vis = sqrt(pt_vis * pt_vis + m_vis * m_vis);

                return ( sqrt( (et_vis + met.lv().Pt()) * (et_vis + met.lv().Pt()) -
                            ((vis + met.lv()).Pt() * (vis + met.lv()).Pt()) ) );
            }
            return var_means.at("MT_1_scaled");

        };
        *cutflow << SaveVar();
    }

    // mt1 (l0, l1), (b0, b1)
    *cutflow << NewVar("mt2_llbb"); {
        *cutflow << HFTname("mt2_llbb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2 && leptons.size()>=2) {
                const TLorentzVector v0 = ( *leptons.at(0) + *leptons.at(1) );
                const TLorentzVector v1 = ( *bjets.at(0) + *bjets.at(1) );
                return kin::getMT2( v0, v1, met );
            }
            return var_means.at("mt2_llbb");
        };
        *cutflow << SaveVar();
    }


    // mt2_bb
    *cutflow << NewVar("mt2_bb"); {
        *cutflow << HFTname("mt2_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2) {
                const TLorentzVector b0 = (*bjets.at(0));
                const TLorentzVector b1 = (*bjets.at(1));
                return kin::getMT2(b0,b1,met);
            }
            return var_means.at("mt2_bb");

        };
        *cutflow << SaveVar();
    }

    // dphi_bb
    *cutflow << NewVar("dphi_bb"); {
        *cutflow << HFTname("dphi_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size()>=2) {
                return (bjets.at(0)->DeltaPhi(*bjets.at(1)));
            }
            return var_means.at("dphi_bb");
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mT_W"); {
        *cutflow << HFTname("mT_W");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            TLorentzVector l0 = (*leptons.at(0));
            l0.SetPz(0.0);
            return ( l0 + met.lv() ).Mt();
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mT_full"); {
        *cutflow << HFTname("mT_full");
        *cutflow << [&](Superlink* /*sl*/, var_float*) -> double {
            TLorentzVector l0 = (*leptons.at(0));
            float delta_phi = l0.DeltaPhi(met.lv());
            float met_et = met.lv().Pt();
            float pt = l0.Pt();
            return sqrt( 2 * met_et * pt * (1 - cos(delta_phi)) );
        };
        *cutflow << SaveVar();
    }

    // mT_llmet
    *cutflow << NewVar("mT_llmet"); {
        *cutflow << HFTname("mT_llmet");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(leptons.size()<2) return -1;
            TLorentzVector l0t = (*leptons.at(0));
            TLorentzVector l1t = (*leptons.at(1));
            l0t.SetPz(0.0);
            l1t.SetPz(0.0);
            return ( (l0t + l1t).M() );
        };
        *cutflow << SaveVar();
    }

    // mT_bb
    *cutflow << NewVar("mT_bb"); {
        *cutflow << HFTname("mT_bb");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            if(bjets.size() >= 2) {
                TLorentzVector b0t = (*bjets.at(0));
                TLorentzVector b1t = (*bjets.at(1));
                return (b0t + b1t).M();
            }
            return var_means.at("mT_bb");
        };
        *cutflow << SaveVar();
    }

    double SHATR;
    double MDR;
    double DPB;
    double RPT;
    double GAM;
    

    *cutflow << [&](Superlink* sl, var_void*) {
        TVector3 dummyvec;
        double dummy;

        kin::superRazor(leptons, met, dummyvec, dummyvec,
                dummyvec, dummyvec, SHATR, DPB, dummy,
                GAM, dummy, MDR, RPT); 

    };

    *cutflow << NewVar("MDR"); {
        *cutflow << HFTname("MDR");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return MDR;
        }; 
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("SHATR"); {
        *cutflow << HFTname("SHATR");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return SHATR;
        };
        *cutflow <<SaveVar();
    }

    *cutflow << NewVar("DPB"); {
        *cutflow << HFTname("DPB");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return DPB;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("GAM"); {
        *cutflow << HFTname("GAM");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return GAM;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("RPT"); {
        *cutflow << HFTname("RPT");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return RPT;
        };
        *cutflow << SaveVar();
    }

    // NN VAR
    // low level
    double nn_output_hh = -999;
    double nn_output_tt = -999;
    double nn_output_wt = -999;
    double nn_output_zjets = -999;
    // high level
    double nn_output_hh_hlvl = -999;
    double nn_output_tt_hlvl = -999;
    double nn_output_wt_hlvl = -999;
    double nn_output_zjets_hlvl = -999;
    // high level ttonly
    double nn_output_hh_hlvl_tt = -999;
    double nn_output_tt_hlvl_tt = -999;
    // high level wtonly
    double nn_output_hh_hlvl_wt = -999;
    double nn_output_wt_hlvl_wt = -999;
    std::map< std::string, std::map< std::string, double >> inputs;
    std::map< std::string, double > nn_input;

    *cutflow << [&](Superlink* sl, var_void*) {
        double nn_mbb = (bjets.size()>=2) ? (*bjets.at(0) + *bjets.at(1)).M() : -10.;
        bool isEE = (leptons.at(0)->isEle() && leptons.at(1)->isEle());
        bool isMM = (leptons.at(0)->isMu() && leptons.at(1)->isMu());
        bool isSF = (isEE || isMM);
        bool isDF = ((leptons.at(0)->isEle() && leptons.at(1)->isMu()) || (leptons.at(0)->isMu() && leptons.at(1)->isEle()));
        double mt2_bb = -10.;
        if(bjets.size()>=2) {
            const TLorentzVector b0 = (*bjets.at(0));
            const TLorentzVector b1 = (*bjets.at(1));
            mt2_bb = kin::getMT2(b0,b1,met);
        }
        double mt2 = -10.;
        if(leptons.size() == 2) {
            mt2 = kin::getMT2(leptons, met);
        }
        double mt2_llbb = -10.;
        if(bjets.size()>=2 && leptons.size()>=2) {
            const TLorentzVector v0 = ( *leptons.at(0) + *leptons.at(1) );
            const TLorentzVector v1 = ( *bjets.at(0) + *bjets.at(1) );
            mt2_llbb = kin::getMT2( v0, v1, met );
        }
        double ht2 = -10.;
        double ht2ratio = -10.;
        if(bjets.size()>=2 && leptons.size()>=2) {

            double num = ( (*bjets.at(0) + *bjets.at(1)).Pt() + 
                (*leptons.at(0) + *leptons.at(1) + met.lv()).Pt() );

            double den = ((*bjets.at(0)).Pt());
            den += (*bjets.at(1)).Pt();
            den += (*leptons.at(0)).Pt();
            den += (*leptons.at(1)).Pt();
            den += met.lv().Pt();
            ht2 = num;
            ht2ratio = (num / den);
        }

    
        nn_input["met"] = met.lv().Pt();
        nn_input["metPhi"] = met.lv().Phi();
        nn_input["mll"] = (*leptons.at(0) + *leptons.at(1)).M();
        nn_input["dRll"] = (leptons.at(0)->DeltaR(*leptons.at(1)));
        nn_input["pTll"] =  (*leptons.at(0) + *leptons.at(1)).Pt();
        nn_input["mbb"] = nn_mbb;
        nn_input["dphi_ll"] = leptons.at(0)->DeltaPhi(*leptons.at(1));
        nn_input["dphi_bb"] = (bjets.size() > 1) ? (*bjets.at(0) + *bjets.at(1)).M() : -10.;
        nn_input["dphi_met_ll"] = met.lv().DeltaPhi( *leptons.at(0) + *leptons.at(1) );
        nn_input["met_pTll"] = (met.lv() + *leptons.at(0) + *leptons.at(1)).Pt();
        nn_input["nJets"] = jets.size();
        nn_input["nSJets"] = sjets.size();
        nn_input["nBJets"] = bjets.size();
        nn_input["isEE"] = isEE ? 1 : 0;
        nn_input["isMM"] = isMM ? 1 : 0;
        nn_input["isSF"] = isSF ? 1 : 0;
        nn_input["isDF"] = isDF ? 1 : 0;
        nn_input["l0_pt"] = leptons.at(0)->Pt();
        nn_input["l1_pt"] = leptons.at(1)->Pt();
        nn_input["l0_eta"] = leptons.at(0)->Eta();
        nn_input["l1_eta"] = leptons.at(1)->Eta();
        nn_input["l0_phi"] = leptons.at(0)->Phi();
        nn_input["l1_phi"] = leptons.at(1)->Phi();
        nn_input["j0_pt"] = (jets.size() > 0) ? jets.at(0)->Pt() : -10.;
        nn_input["j1_pt"] = (jets.size() > 1) ? jets.at(1)->Pt() : -10.;
        nn_input["j0_eta"] = (jets.size() > 0) ? jets.at(0)->Eta() : -10.;
        nn_input["j1_eta"] = (jets.size() > 1) ? jets.at(1)->Eta() : -10.;
        nn_input["j0_phi"] = (jets.size() > 0) ? jets.at(0)->Phi() : -10.;
        nn_input["j1_phi"] = (jets.size() > 1) ? jets.at(1)->Phi() : -10.;
        nn_input["bj0_pt"] = (bjets.size() > 0) ? bjets.at(0)->Pt() : -10.;
        nn_input["bj1_pt"] = (bjets.size() > 1) ? bjets.at(1)->Pt() : -10.;
        nn_input["bj0_eta"] = (bjets.size() > 0) ? bjets.at(0)->Eta() : -10.;
        nn_input["bj1_eta"] = (bjets.size() > 1) ? bjets.at(1)->Eta() : -10.;
        nn_input["bj0_phi"] = (bjets.size() > 0) ? bjets.at(0)->Phi() : -10.;
        nn_input["bj1_phi"] = (bjets.size() > 1) ? bjets.at(1)->Phi() : -10.;
        nn_input["dphi_bj0_ll"] = (bjets.size() > 0) ? (bjets.at(0)->DeltaPhi( *leptons.at(0) + *leptons.at(1) )) : -10.;
        nn_input["dphi_bj0_l0"] = (bjets.size() > 0) ? (bjets.at(0)->DeltaPhi( *leptons.at(0) )) : -10.;
        nn_input["mt2"] = mt2;
        nn_input["mt2_bb"] = mt2_bb;
        nn_input["mt2_llbb"] = mt2_llbb;
        nn_input["HT2"] = ht2;
        nn_input["HT2Ratio"] = ht2ratio;
    
        inputs["InputLayer"] = nn_input;
        auto output_scores_lowlvl = nn_graph_lowlvl.compute(inputs);
        auto output_scores_hlvl = nn_graph_hlvl.compute(inputs);
        auto output_scores_hlvl_ttonly = nn_graph_hlvl_ttonly.compute(inputs);
        auto output_scores_hlvl_wt = nn_graph_hlvl_wt.compute(inputs);

        nn_output_hh = output_scores_lowlvl.at("out_0_hh");
        nn_output_tt = output_scores_lowlvl.at("out_1_tt");
        nn_output_wt = output_scores_lowlvl.at("out_2_wt");
        nn_output_zjets = output_scores_lowlvl.at("out_3_zjets");

        nn_output_hh_hlvl = output_scores_hlvl.at("out_0_hh");
        nn_output_tt_hlvl = output_scores_hlvl.at("out_1_tt");
        nn_output_wt_hlvl = output_scores_hlvl.at("out_2_wt");
        nn_output_zjets_hlvl = output_scores_hlvl.at("out_3_zjets");

        nn_output_hh_hlvl_tt = output_scores_hlvl_ttonly.at("out_0_hh");
        nn_output_tt_hlvl_tt = output_scores_hlvl_ttonly.at("out_1_tt");

        nn_output_hh_hlvl_wt = output_scores_hlvl_wt.at("out_0_hh");
        nn_output_wt_hlvl_wt = output_scores_hlvl_wt.at("out_1_wt");
    };

    *cutflow << NewVar("nn_p_hh"); {
        *cutflow << HFTname("nn_p_hh");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_hh;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_tt"); {
        *cutflow << HFTname("nn_p_tt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_tt;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_wt"); {
        *cutflow << HFTname("nn_p_wt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_wt;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_zjets"); {
        *cutflow << HFTname("nn_p_zjets");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_zjets;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh / nn_p_tt"); {
        *cutflow << HFTname("nn_p_hh_tt");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh / nn_output_tt);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh / nn_p_wt"); {
        *cutflow << HFTname("nn_p_hh_wt");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh / nn_output_wt);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh / nn_p_z"); {
        *cutflow << HFTname("nn_p_hh_z");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh / nn_output_zjets);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh / (nn_p_tt+nn_p_wt)"); {
        *cutflow << HFTname("nn_p_hh_ttwt");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh / (nn_output_tt+nn_output_wt));
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_hh"); {
        *cutflow << HFTname("nn_d_hh");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log( nn_output_hh / (nn_output_tt + nn_output_wt + nn_output_zjets) );
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_tt"); {
        *cutflow << HFTname("nn_d_tt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log( nn_output_tt / (nn_output_hh + nn_output_wt + nn_output_zjets) );
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_wt"); {
        *cutflow << HFTname("nn_d_wt");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log( nn_output_wt / (nn_output_hh + nn_output_tt + nn_output_zjets) );
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_zjets"); {
        *cutflow << HFTname("nn_d_zjets");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log( nn_output_zjets / (nn_output_hh + nn_output_tt + nn_output_wt) );
        };
        *cutflow << SaveVar();
    }
    // highlvl
    *cutflow << NewVar("nn_p_hh_hlvl"); {
        *cutflow << HFTname("nn_p_hh_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_hh_hlvl;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_tt_hlvl"); {
        *cutflow << HFTname("nn_p_tt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_tt_hlvl;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_wt_hlvl"); {
        *cutflow << HFTname("nn_p_wt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_wt_hlvl;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh_tt_hlvl"); {
        *cutflow << HFTname("nn_p_hh_tt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh_hlvl / nn_output_tt_hlvl);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh_wt_hlvl)"); {
        *cutflow << HFTname("nn_p_hh_wt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh_hlvl / nn_output_wt_hlvl);
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh_ttwt_hlvl"); {
        *cutflow << HFTname("nn_p_hh_ttwt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh_hlvl / (nn_output_tt_hlvl + nn_output_wt_hlvl));
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_zjets_hlvl"); {
        *cutflow << HFTname("nn_p_zjets_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_zjets_hlvl;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_hh_hlvl"); {
        *cutflow << HFTname("nn_d_hh_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log(nn_output_hh_hlvl / (nn_output_tt_hlvl + nn_output_wt_hlvl + nn_output_zjets_hlvl));
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_tt_hlvl"); {
        *cutflow << HFTname("nn_d_tt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log(nn_output_tt_hlvl / (nn_output_hh_hlvl + nn_output_wt_hlvl + nn_output_zjets_hlvl));
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_wt_hlvl"); {
        *cutflow << HFTname("nn_d_wt_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log(nn_output_wt_hlvl / (nn_output_hh_hlvl + nn_output_tt_hlvl + nn_output_zjets_hlvl));
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_d_zjets_hlvl"); {
        *cutflow << HFTname("nn_d_zjets_hlvl");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return log(nn_output_zjets_hlvl / (nn_output_hh_hlvl + nn_output_tt_hlvl + nn_output_wt_hlvl));
        };
        *cutflow << SaveVar();
    }
    // high level tt only
    *cutflow << NewVar("nn_p_hh_hlvl_ttonly"); {
        *cutflow << HFTname("nn_p_hh_hlvl_ttonly");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_hh_hlvl_tt;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_tt_hlvl_ttonly"); {
        *cutflow << HFTname("nn_p_tt_hlvl_ttonly");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_tt_hlvl_tt;
        };
        *cutflow  << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh_tt_hlvl_ttonly"); {
        *cutflow << HFTname("nn_p_hh_tt_hlvl_ttonly");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh_hlvl_tt / nn_output_tt_hlvl_tt);
        };
        *cutflow << SaveVar();
    }
    // high level wt only
    *cutflow << NewVar("nn_p_hh_hlvl_wtonly"); {
        *cutflow << HFTname("nn_p_hh_hlvl_wtonly");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return nn_output_hh_hlvl_wt;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_wt_hlvl_wtonly"); {
        *cutflow << HFTname("nn_p_wt_hlvl_wtonly");
        *cutflow <<[&](Superlink* sl, var_float*) -> double {
            return nn_output_wt_hlvl_wt;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("nn_p_hh_wt_hlvl_wtonly"); {
        *cutflow << HFTname("nn_p_hh_wt_hlvl_wtonly");
        *cutflow << [&](Superlink* sl, var_float*) -> double {
            return (nn_output_hh_hlvl_wt / nn_output_wt_hlvl_wt);
        };
        *cutflow << SaveVar();
    }


    // clear the wectors
    *cutflow << [&](Superlink* /* sl */, var_void*) { leptons.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { electrons.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { muons.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { jets.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { bjets.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { sjets.clear(); };
    *cutflow << [&](Superlink* /* sl */, var_void*) { met.clear(); };


    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    //
    // Sysystematics [BEGIN]
    //
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////
    // weight systematics
    ////////////////////////////////////

    // electron eff
    *cutflow << NewSystematic("shift in electron ID efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_ID_UP, SupersysWeight::EL_EFF_ID_DN);
        *cutflow << TreeName("EL_EFF_ID");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron ISO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_ISO_UP, SupersysWeight::EL_EFF_ISO_DN);
        *cutflow << TreeName("EL_EFF_Iso");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron RECO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_RECO_UP, SupersysWeight::EL_EFF_RECO_DN);
        *cutflow << TreeName("EL_EFF_Reco");
        *cutflow << SaveSystematic();
    }

    // muon eff
    *cutflow << NewSystematic("muon eff stat uncertainty"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_STAT_UP, SupersysWeight::MUON_EFF_STAT_DN);
        *cutflow << TreeName("MUON_EFF_STAT");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon eff stat uncertainty (low pt)"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_STAT_LOWPT_UP, SupersysWeight::MUON_EFF_STAT_LOWPT_DN);
        *cutflow << TreeName("MUON_EFF_STAT_LOWPT");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon eff syst uncertainty"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_SYS_UP, SupersysWeight::MUON_EFF_SYS_DN);
        *cutflow << TreeName("MUON_EFF_SYS");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon eff syst uncertainty (low pt"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_SYS_LOWPT_UP, SupersysWeight::MUON_EFF_SYS_LOWPT_DN);
        *cutflow << TreeName("MUON_EFF_SYS_LOWPT");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon eff iso stat uncertainty"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_ISO_STAT_UP, SupersysWeight::MUON_EFF_ISO_STAT_DN);
        *cutflow << TreeName("MUON_ISO_STAT");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon eff iso syst uncertainty"); {
        *cutflow << WeightSystematic(SupersysWeight::MUON_EFF_ISO_SYS_UP, SupersysWeight::MUON_EFF_ISO_SYS_DN);
        *cutflow << TreeName("MUON_ISO_SYS");
        *cutflow << SaveSystematic();
    }

    // flavor tagging eff
    *cutflow << NewSystematic("shift in b-tag efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::FT_EFF_B_UP, SupersysWeight::FT_EFF_B_DN);
        *cutflow << TreeName("FT_EFF_B");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in c-tag efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::FT_EFF_C_UP, SupersysWeight::FT_EFF_C_DN);
        *cutflow << TreeName("FT_EFF_C");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in light tag (i.e. mis-tag) efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::FT_EFF_LT_UP, SupersysWeight::FT_EFF_LT_DN);
        *cutflow << TreeName("FT_EFF_Light");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift flavor tagging extrapolation ?"); {
        *cutflow << WeightSystematic(SupersysWeight::FT_EFF_EXTRAP_UP, SupersysWeight::FT_EFF_EXTRAP_DN);
        *cutflow << TreeName("FT_EFF_extrapolation");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift flavor tagging extrapolation - charm ?"); {
        *cutflow << WeightSystematic(SupersysWeight::FT_EFF_EXTRAPC_UP, SupersysWeight::FT_EFF_EXTRAPC_DN);
        *cutflow << TreeName("FT_EFF_extrapolation_charm");
        *cutflow << SaveSystematic();
    }

    // jvt eff
    *cutflow << NewSystematic("shift in JVT efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::JVT_EFF_UP, SupersysWeight::JVT_EFF_DN);
        *cutflow << TreeName("JET_JVTEff");
        *cutflow << SaveSystematic();
    }

    // pileup
    *cutflow << NewSystematic("shift in data mu (pile-up)"); {
        *cutflow << WeightSystematic(SupersysWeight::PILEUP_UP, SupersysWeight::PILEUP_DN);
        *cutflow << TreeName("PILEUP");
        *cutflow << SaveSystematic();
    }


    ////////////////////////////////////
    // shape systematics
    ////////////////////////////////////

    // egamma
    *cutflow << NewSystematic("shift in e-gamma resolution (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_UP);
        *cutflow << TreeName("EG_RESOLUTION_ALL_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma resolution (DOWN)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_DN);
        *cutflow << TreeName("EG_RESOLUTION_ALL_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma scale (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_SCALE_ALL_UP);
        *cutflow << TreeName("EG_SCALE_ALL_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma scale (DOWN)"); {
        *cutflow << EventSystematic(NtSys::EG_SCALE_ALL_DN);
        *cutflow << TreeName("EG_SCALE_ALL_DN");
        *cutflow << SaveSystematic();
    }
    // muon
    *cutflow << NewSystematic("muon ID (UP)"); {
        *cutflow << EventSystematic(NtSys::MUON_ID_UP);
        *cutflow << TreeName("MUON_ID_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon ID (DOWN)"); {
        *cutflow << EventSystematic(NtSys::MUON_ID_DN);
        *cutflow << TreeName("MUON_ID_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon MS (UP)"); {
        *cutflow << EventSystematic(NtSys::MUON_MS_UP);
        *cutflow << TreeName("MUON_MS_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon MS (DOWN)"); {
        *cutflow << EventSystematic(NtSys::MUON_MS_DN);
        *cutflow << TreeName("MUON_MS_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon scale shift (UP)"); {
        *cutflow << EventSystematic(NtSys::MUON_SCALE_UP);
        *cutflow << TreeName("MUON_SCALE_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("muon scale shift (DN)"); {
        *cutflow << EventSystematic(NtSys::MUON_SCALE_DN);
        *cutflow << TreeName("MUON_SCALE_DN");
        *cutflow << SaveSystematic();
    }

    // jet
    *cutflow << NewSystematic("JER"); {
        *cutflow << EventSystematic(NtSys::JER);
        *cutflow << TreeName("JER");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 1 (up)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_1_UP);
        *cutflow << TreeName("JET_GroupedNP_1_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 1 (down)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_1_DN);
        *cutflow << TreeName("JET_GroupedNP_1_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 2 (up)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_2_UP);
        *cutflow << TreeName("JET_GroupedNP_2_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 2 (down)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_2_DN);
        *cutflow << TreeName("JET_GroupedNP_2_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 3 (up)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_3_UP);
        *cutflow << TreeName("JET_GroupedNP_3_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("JES NP set 3 (down)"); {
        *cutflow << EventSystematic(NtSys::JET_GroupedNP_3_DN);
        *cutflow << TreeName("JET_GroupedNP_3_DN");
        *cutflow << SaveSystematic();
    }
    //#warning only setting MET systematics
    // met
    //*cutflow << NewSystematic("MET TST Soft-Term resolution (parallel)"); {
    //    *cutflow << EventSystematic(NtSys::MET_SoftTrk_ResoPara);
    //    *cutflow << TreeName("MET_SoftTrk_ResoPara");
    //    *cutflow << SaveSystematic();
    //}
    //*cutflow << NewSystematic("MET TST Soft-Term resolution (perpendicular)"); {
    //    *cutflow << EventSystematic(NtSys::MET_SoftTrk_ResoPerp);
    //    *cutflow << TreeName("MET_SoftTrk_ResoPerp");
    //    *cutflow << SaveSystematic();
    //}
    //*cutflow << NewSystematic("MET TST Soft-Term shift in scale (UP)"); {
    //    *cutflow << EventSystematic(NtSys::MET_SoftTrk_ScaleUp);
    //    *cutflow << TreeName("MET_SoftTrk_ScaleUp");
    //    *cutflow << SaveSystematic();
    //}
    //*cutflow << NewSystematic("MET TST Soft-Term shift in scale (DOWN)"); {
    //    *cutflow << EventSystematic(NtSys::MET_SoftTrk_ScaleDown);
    //    *cutflow << TreeName("MET_SoftTrk_ScaleDown");
    //    *cutflow << SaveSystematic();
    //}


    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    //
    // Superflow methods [END]
    //
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////

//    delete pu_profile;

    // initialize the cutflow and start the event loop
    chain->Process(cutflow, options.input.c_str(), options.n_events_to_process);
    delete cutflow;
    delete chain;
    cout << "La Fin." << endl;
    exit(0);

}
