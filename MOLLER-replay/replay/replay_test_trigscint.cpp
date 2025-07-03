/* Test replay script for moller trigger scintillator data */

#include "TSystem.h"
#include "THaRun.h"
#include "THaAnalyzer.h"
#include "THaEvent.h"
#include "TDatime.h"
#include "TString.h"

#include "MOLLERTriggerScintillator.h"
#include "MOLLERSpectrometer.h"
#include "THaApparatus.h"

void replay_test_trigscint() {
    // Create an apparatus and add your detector to it
    // THaApparatus* testApp = new THaApparatus("TestApp");
    MOLLERSpectrometer *moller = new MOLLERSpectrometer ("moller", "Generic apparatus");
    MOLLERTriggerScintillator* trigscint = new MOLLERTriggerScintillator("trigscint", "Trigger scintillator");
    
    moller->AddDetector(trigscint);

    // Register the apparatus with the analyzer framework
    gHaApps->Add(moller);

    // Create a dummy run (no real evio file needed for DB load test)
    TString prefix = gSystem->Getenv("DATA_DIR");
    TString codafilename;
    codafilename.Form("%s/test_vtp_372.evio.0", prefix.Data()); // existing file or any evio file

    TDatime now;
    THaRun* run = new THaRun(codafilename);
    run->SetNumber(372);
    run->SetDate(now);
    run->SetFirstEvent(0);
    run->SetLastEvent(1);  // Just process 1 event

    // Create analyzer and set up event
    THaEvent* event = new THaEvent;
    THaAnalyzer* analyzer = new THaAnalyzer;
    analyzer->SetEvent(event);
    analyzer->SetVerbosity(2);
    analyzer->EnableBenchmarks();

    // Minimal output file setup
    TString outfilename = gSystem->Getenv("OUT_DIR");
    outfilename += "/test_trigscint_output.root";
    analyzer->SetOutFile(outfilename.Data());

    // ODEF is optional here, but needed if any variables are defined
    TString odef_filename = gSystem->Getenv("MOLLER_REPLAY");
    odef_filename += "/replay/replay_moller_fadc.odef";
    analyzer->SetOdefFile(odef_filename);

    // Start analysis, which triggers ReadDatabase
    analyzer->Process(run);
}
