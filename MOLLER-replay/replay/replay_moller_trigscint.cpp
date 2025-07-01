/* Replay script for analyzing moller trigger scintillator data */
#include "MOLLERTriggerScintillator.h"

#include "TSystem.h"
#include "TList.h"
#include "THaRun.h"
#include "THaEvent.h"
#include "THaAnalyzer.h"
#include "THaApparatus.h"
#include "TString.h"

using namespace std;

void replay_moller_trigscint(int runnum=372, int firstsegment=0, int maxsegments=1, long firstevent=0, long nevents=1000) {
    MOLLERTriggerScintillator *trigscint = new MOLLERTriggerScintillator("trigscint", "moller trigger scintillator");

    THaAnalyzer *analyzer = new THaAnalyzer;

    gHaApps->Add(trigscint);

    THaEvent* event = new THaEvent;

    TString prefix = gSystem->Getenv("DATA_DIR");

    bool segmentexists = true;
    int segment = firstsegment;

    int lastsegment = firstsegment;

    TClonesArray *filelist = new TClonesArray("THaRun", 10);

    TDatime now = TDatime();

    int segcounter = 0;

    // This loop adds all file segments found to the list of THaRuns to process:
    while (segcounter < maxsegments && segment - firstsegment < maxsegments) {
        
        /*fill the loop*/
        segment++;
    }
    
    cout << "n segments to analyze = " << segcounter << endl;

    prefix = gSystem->Getenv("OUT_DIR");
    firstsegment = 0;
    lastsegment = 1;
    TString outfilename;
    outfilename.Form("%s/moller_fadc_reolayed_%d_seg%d_%d.root", prefix.Data(), runnum, firstsegment, lastsegment);

    analyzer->SetVerbosity(2);
    analyzer->SetMarkInterval(100);
    analyzer->EnableBenchmarks();

    // Define the analysis parameters
    analyzer->SetEvent(event);
    analyzer->SetOutFile(outfilename.Data());
    // File to record cuts accounting info
    analyzer->SetSummaryFile("summary_example.log"); // Optional

    prefix = gSystem->Getenv("MOLLER_REPLAY");
    prefix += "/replay/";

    TString odef_filename = "replay_moller_fadc.odef"; // Check what this is
    odef_filename.Prepend(prefix);


    analyzer->SetOdefFile(odef_filename);
    //analyzer->SetCompressionLevel(0); // turn off compression

    filelist->Compress();

    for (int iseg = 0; iseg < filelist->GetEntries(); iseg++) {
        /*complete the loop*/
    }
    
}
