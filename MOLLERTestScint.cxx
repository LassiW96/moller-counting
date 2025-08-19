//////////////////////////////////////////////////////////////////////////////////////
// Trying a test scintillator class according to JLab Hall A SDK
// Following UserDetector
//
//////////////////////////////////////////////////////////////////////////////////////

#include "MOLLERTestScint.h"
#include "FADCData.h"
/*#include "VarDef.h"
#include "THaDetMap.h"
#include "TMath.h"
#include "Helper.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "Fadc250Module.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
*/

#include <chrono>

using namespace std;
using namespace Podd;

// Hard coded maximum number of channels
static const int MAXCHAN = 100;

// Initial basic constructor
//////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::MOLLERTestScint(const char* name, const char* description,
                                THaApparatus* apparatus) :
                                THaNonTrackingDetector(name, description, apparatus), 
                                fPMT(nullptr)
{
    // Constructor
}

// Destructor
///////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::~MOLLERTestScint()
{
    // Remove vars from the global list
    RemoveVariables();
}

// Read the DB for this detector
///////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::ReadDatabase(const TDatime& date)
{
    std::cout << "[DEBUG] in RDB = " << std::endl;

    const char* const here = "ReadDatabase";

    Int_t err = THaNonTrackingDetector::ReadDatabase(date);
    if (err)
      return err;

    FILE* file = OpenFile(date);
    if (!file) return kFileError;

    // Read fOrigin and fsize (required!)
    err = ReadGeometry(file, date, true);
    if (err) {
        fclose(file);
        return err;
    }

    //enum {kModeUnset = -255, kCommonStop = 0, kCommonStart = 1};

    vector<Int_t> detmap;
    vector<Int_t> chanmap;
    Int_t model_in_detmap = 0;
    Int_t ncols = 0;
    Int_t nrows = 0;

    DBRequest config_request[] = {
        { "detmap",             &detmap,            kIntV },
        { "model_in_detmap",    &model_in_detmap,   kInt,   0,  true}, // Module number if mentioned in the db file.
        { "chanmap",            &chanmap,           kIntV },
        { "ncols",              &ncols,             kInt },
        { "nrows",              &nrows,             kInt },
        { nullptr }
    };

    err = LoadDB(file, date, config_request, fPrefix);

    UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
    std::cout<<"Flags ="<<flags<<std::endl;
    if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
      cout<<"here"<<endl;
      err = kInitError;  // Error already printed by FillDetMap
    }

    // Print detmap contents
    std::cout << "Detmap: ";
    for (size_t i = 0; i < detmap.size(); ++i) {
        std::cout << detmap[i] << " ";
    }
    std::cout << std::endl;

    auto ret = HallA::MakeFADCData(date, this);
    if (ret.second)
        return ret.second; // Database error

    fPMT = ret.first.get();

    fDetectorData.emplace_back(move(ret.first));

    //fPadData.resize(nval);
    //fHits.reserve(nval);

    // Calibration
    fclose(file);
    // Debug

    fIsInit = true;
    return kOK;
}

// Define/delete global vars
//////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::DefineVariables(EMode mode)
{
    std::cout << "[DEBUG] in def var = " << std::endl;

    // Define more variables as required
    // Only including following for now
    /*RVarDef vars[] = {
        {"nhits",       "Number of hits",       "GetNhits()"},
        {"chan",        "Channel number",       "fEventData.fChannel"},
        {"adc",         "Raw ADC value",        "fEventData.fRawADC"},
        {"adc_c",       "Calibrated ADC value", "fEventData.fCalADC"},
        {nullptr}
    };
    return DefineVarsFromList(vars, mode);*/

    return fPMT->DefineVariables(mode);
}

// Clear per-event data - this is called before Decode() function
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void MOLLERTestScint::Clear(Option_t* opt)
{
    THaNonTrackingDetector::Clear(opt);
    fEventData.clear();
}

// Adding a decode method following THaDetectorBase
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::Decode( const THaEvData& evdata )
{

  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.

  const char* const here = "Decode";

  bool has_warning = false;
  Int_t nhits = 0;
  
  // Iterator over all channels assigned to this detector
  auto hitIter = fDetMap->MakeIterator(evdata);
  while (hitIter) {
    const auto& hitinfo = *hitIter;

    // Example: Warn about multiple hits unless you expect them
    if (hitinfo.nhit > 1 &&
        hitinfo.modtype != Decoder::ChannelType::kMultiFunctionADC &&
        hitinfo.modtype != Decoder::ChannelType::kMultiFunctionTDC) {
      MultipleHitWarning(hitinfo, here);
      has_warning = true;
    }

    // Get amplitude (or raw ADC value)
    auto data = LoadData(evdata, hitinfo);
    if (!data) {
      DataLoadWarning(hitinfo, here);
      has_warning = true;
      ++hitIter;
      continue;
    }

    auto *fFadc = dynamic_cast <Fadc250Module*> (evdata.GetModule(hitinfo.crate, hitinfo.slot));

    // Need to figure out how to get these values from hitinfo or DB. Hard coding for now
    static const size_t NADCCHAN = fDetMap->GetTotNumChan();
    static const size_t NUMSLOTS = 22;        // Number of slots
    static const size_t NPEAK = 4;            // ??

    vector<uint32_t> raw_samples_vector[NUMSLOTS][NADCCHAN], raw_samples_npeak_vector[NUMSLOTS][NADCCHAN][NPEAK];

    if (!fFadc) {
        cout << "ERROR: Module at crate " << hitinfo.crate
            << ", slot " << hitinfo.slot << " is not an Fadc250Module" << endl;
        continue;
    }

    UInt_t npulses = fFadc->GetNumFadcEvents(hitinfo.chan);
    if (hitinfo.hit >= npulses) {
        cout << "ERROR: Requested hit index " << hitinfo.hit
            << " out of range (only " << npulses << " hits available) "
            << "for slot " << hitinfo.slot << ", channel " << hitinfo.chan << endl;
        continue;
    }

    for (size_t chan = 0; chan < NADCCHAN; chan++) {

        // Number of FADC events
        UInt_t fadcNevents = fFadc->GetNumFadcEvents(chan);
        // Number of FADC samples
        UInt_t fadcNsamples = fFadc->GetNumFadcSamples(chan, hitinfo.hit);
        //Double_t time = fFadc->GetPulseTimeData(chan, hitinfo.hit);

        for (UInt_t jevent = 0; jevent < fadcNevents; jevent++) {
            Double_t integral = fFadc->GetEmulatedPulseIntegralData(chan);
            fIntegral.push_back(integral);

            // Acquire raw sample vector
            if (fadcNsamples > 0) {
                raw_samples_vector[hitinfo.slot][chan] = fFadc->GetPulseSamplesVector(chan);
                for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
                    if (uint32_t (fadcNevents) == ipeak+1) {
                        raw_samples_npeak_vector[hitinfo.slot][chan][ipeak] = fFadc->GetPulseSamplesVector(chan);
                    }
                }
                // Raw sample n peak index needs to be filled
            }
        }
    }
    
    StoreHit(hitinfo, data.value());

    // Clear hit-done flag for next iteration
    for (auto& detData : fDetectorData)
      detData->ClearHitDone();

    ++hitIter;
    ++nhits;
  }

  if (has_warning)
    ++fNEventsWithWarnings;

#ifdef WITH_DEBUG
  if (fDebug > 3)
    PrintDecodedData(evdata);
#endif

  return nhits;
}

// Adding LoadData function according to FADCScintillator
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/*OptUInt_t MOLLERTestScint::LoadData( const THaEvData& evdata,
    const DigitizerHitInfo_t& hitinfo )
{
    //std::cout << "[DEBUG] in Load data= " << std::endl;

// Callback from Decoder for loading the data for the 'hitinfo' channel.
// This routine supports FADC modules and returns the pulse amplitude integral.
// Additional info is retrieved from the FADC modules in StoreHit later.

// Following new script in hana_decode/apps/tstfadc script
for (size_t mod = 0; mod + 4 < fDetMap->GetSize(); mod++) {
    THaDetMap::Module *d = fDetMap->GetModule(mod);
    fFadc = dynamic_cast<Fadc250Module*>(
            evdata.GetModule(d->crate, d->slot)
    );
}

if (hitinfo.modtype == Decoder::ChannelType::kMultiFunctionADC) {
    //std::cout << " [DEBUG] before chrono start " << std::endl;
    auto start = std::chrono::steady_clock::now();
    auto result = fPMT->LoadFADCData(hitinfo);
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::milliseconds(100)) {
      std::cerr << "[ERROR] LoadFADCData hung for "
                << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                << " ms, skipping" << std::endl;
      return {}; // empty
    }
    return result;
}

// Fallback to legacy modules
return THaNonTrackingDetector::LoadData(evdata, hitinfo);
//if (ret) return ret;
//return 0;
}*/

// Store decoded data
// See SDK/UserDetector.cxx for more info
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/*Int_t MOLLERTestScint::StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data)
{
    //std::cout << "[DEBUG] in StoreHit = " << std::endl;
  // Put decoded frontend data into fDetectorData. Called from Decode().
  // Data decoding is also done here - from FADCData
  // Call StoreHit for the FADC modules first to get updated pedestals
  //FADCData* fadcData = fPMTs;
  fPMT->StoreHit(hitinfo, data);

  //cout << "In StoreHit function" << endl;

  // Retrieve pedestal, if available, and update the PMTData calibrations
  // Just added the function

  // Now fill the PMTData in fDetectorData
  return THaNonTrackingDetector::StoreHit(hitinfo, data);
}*/

// Coarse process & Fine process
// Fill these functions as required
// Returns nothing for now
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::CoarseProcess(TClonesArray& )
{
    return 0;
}

Int_t MOLLERTestScint::FineProcess(TClonesArray& )
{
    return 0;
}

// Helper macro to print a single field of a structure in a std::vector
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define PrintArrayField(txt, var, field)                \
    cout << (txt) << " = ";                             \
    {                                                   \
        if ((var).empty())                              \
            cout << "(empty)";                          \
        else {                                          \
            for (auto it = (var).begin();               \
                        it != (var).end(); ++it) {      \
                cout << (*it).field;                    \
                if (it+1 != (var).end()) cout << ", ";  \
                }                                       \
        }                                               \
    } cout << endl;

// Print current config
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*void MOLLERTestScint::Print(Option_t* opt) const
{
    THaDetector::Print(opt);
    cout << "detmap = "; fDetMap->Print();
    cout << "nelem = " << fNelem << endl;
    //cout << "pedestals = "; PrintArray( fPed );
    //cout << "gains = ";     PrintArray( fGain );
    cout << "nhits = " << fEventData.size() << endl;
    PrintArrayField("channel", fEventData, fChannel)
    PrintArrayField("rawadc", fEventData, fRawADC)
    PrintArrayField("coradc", fEventData, fCalADC)
}*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(MOLLERTestScint)
