//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MOLLERTriggerScintillator
// 
// Class for a Trigger Scintillator with one PMT 
// in MOLLER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "MOLLERTriggerScintillator.h"

#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaTrackProj.h"
#include "VarDef.h"
#include "VarType.h"
#include "Helper.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"
#include "MOLLERManager.h"
#include "THaCrateMap.h"
#include "FADCData.h"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <memory>

using namespace std;
namespace HallA {

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
MOLLERTriggerScintillator::MOLLERTriggerScintillator(const char* name, const char* description,
                                                    THaApparatus* apparatus)
                        : THaNonTrackingDetector(name, description, apparatus), 
                        fCn(0), fAttenuation(0), fResolution(0), fPMTs(nullptr)
{
    // Which mode is going to use, Sigle PMT or Dual PMT
    fNviews = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Default constructor for ROOT RTTI
MOLLERTriggerScintillator::MOLLERTriggerScintillator()
    : THaNonTrackingDetector(),
    fCn(0), fAttenuation(0), fResolution(0), fPMTs(nullptr)
{
    // Fill the default constructor body if necessary
    fNviews = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read the Scintillators parameters from the database
Int_t MOLLERTriggerScintillator::ReadDatabase(const TDatime& date)
{
    // Read detector's parameters from the DB
    // If detectors can't be added to detmap 
    // or entry for Trigger Scintillators is missing        -> kInitError
    // Success                                              -> kOK

    const char* const here = "ReadDatabase";

    VarType kDataType  = std::is_same<Data_t, Float_t>::value ? kFloat  : kDouble;
    VarType kDataTypeV = std::is_same<Data_t, Float_t>::value ? kFloatV : kDoubleV;

    FILE* file = OpenFile(date);
    if (!file) return kFileError;

    // Read fOrigin and fsize (required!)
    Int_t err = ReadGeometry(file, date, true);
    if (err) {
        fclose(file);
        return err;
    }

    enum {kModeUnset = -255, kCommonStop = 0, kCommonStart = 1};

    vector<Int_t> detmap;
    vector<Int_t> chanmap;
    Int_t model_in_detmap = 0;
    Int_t ncols = 0;
    Int_t nrows = 0;

    DBRequest config_request[] = {
        { "detmap",             &detmap,            kIntV },
        { "model_in_detmap",    &model_in_detmap,   kInt,   0,  true},
        { "chanmap",            &chanmap,           kIntV },
        { "ncols",              &ncols,             kInt },
        { "nrows",              &nrows,             kInt },
        { nullptr }
    };

    err = LoadDB(file, date, config_request, fPrefix);

    // Sanity checks
    // Going to use columns as the number of paddles and rows as number of PMTs
    if (!err && (ncols <= 0 || nrows <= 0)) {
        Error( Here(here), "Illigal number of paddles and/or PMTs, " 
        "paddles: %d, PMTs: %d", ncols, nrows);
        fclose(file);
        return kInitError;
    }

    // Reinitialization
    if (!err) {
        if (fIsInit && ncols != fNelem) {
            // Assume ncols as the number of paddles
            Error(Here(here), "Cannot re-initialize with different number of paddles. "
        "(was: %d, now: %d). Detector not re-initialized.", fNelem, ncols);
        err = kInitError;
        } else
        fNelem = ncols;
    }

    // kFillLogicalChannel:     Logical channel number for start channel
    // kFillModel:              Module's hardware model number 
    // kFillRefIndex:           Specify reference index/channel
    // see THaDetMap::Fill for further comments

    //UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel; // According to THaScintillator
    UInt_t flags = THaDetMap::kFillRefIndex; // According to Generic detector
    if(model_in_detmap) {
        flags |= THaDetMap::kFillModel;
    }
    if (!err && FillDetMap(detmap, flags, here) <= 0) {
        err = kInitError;
    }

    cout << "In ReadDatabase function" << endl;

    UInt_t nval = fNelem; // Variable to store total number of channels
    if (!err) {
        UInt_t tot_nchan = fDetMap->GetTotNumChan();
        if (tot_nchan != 2 * nval) {
            // This number should be changed according to the number of PMTs per detector
            Error(Here(here), "Number of detector map channels (%u) "
                              "inconsistent with 2 * number of paddles (%d)",
                            tot_nchan, 2 * fNelem);
            err = kInitError;
        }
    }

    // Modifying line 153 - 161 in THaScintillator
    UInt_t nmodules = fDetMap->GetSize();
    for (UInt_t i = 0; i < nmodules; i++) {
        THaDetMap::Module* d = fDetMap->GetModule(i);
        if (!d->model) {
            // Model number of the module
            d->MakeADC();
            // Skipping TDC mode
        }
    }

    if (err) {
        fclose(file);
        return err;
    }

    // Set up storage for detector per event data
    // No need to loop over kRight and kLeft 
    fDetectorData.clear();
    auto ret = MakeFADCData(date, this);
    if (ret.second)
        return ret.second; // Database error

    fPMTs = ret.first.get();

    fDetectorData.emplace_back(move(ret.first));

    //fPadData.resize(nval);
    //fHits.reserve(nval);

    // Calibration

    // Debug

    fIsInit = true;
    return kOK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define global analysis variables
Int_t MOLLERTriggerScintillator::DefineVariables(EMode mode)
{
    // Define global analysis variables
    // Modified to include FADCData
    // Add variables for raw PMT data
    class VarDefInfo {
    public:
        FADCData* pmtData;
        const char* key_prefix;
        const char* comment_subst;
        Int_t DefineVariables(EMode mode) const
        {return pmtData->DefineVariables(mode, key_prefix, comment_subst);} // FADCData::DefineVariables function is called here
    };
    if (Int_t ret = VarDefInfo{fPMTs, "p", "all-PMTs"}.DefineVariables(mode))
        return ret;

    cout << "In DefineVariables function" << endl;
    // Define variables on the remaining event data
    // copied from THaScintillator for now - needs to be changed accordingly
    /*RVarDef vars[] = {
    { "nthit",  "Number of paddles with L&R TDCs",   "GetNHits()" },
    { "t_pads", "Paddles with L&R coincidence TDCs", "fHits.pad" },
    { "y_t",    "y-position from timing (m)",        "fPadData.yt" },
    { "y_adc",  "y-position from amplitudes (m)",    "fPadData.ya" },
    { "time",   "Time of hit at plane (s)",          "fPadData.time" },
    { "dtime",  "Est. uncertainty of time (s)",      "fPadData.dtime" },
    { "dedx",   "dEdX-like deposited in paddle",     "fPadData.ampl" },
    { "hit.y_t","y-position from timing (m)",        "fHits.yt" },
    { "hit.y_adc", "y-position from amplitudes (m)", "fHits.ya" },
    { "hit.time",  "Time of hit at plane (s)",       "fHits.time" },
    { "hit.dtime", "Est. uncertainty of time (s)",   "fHits.dtime" },
    { "hit.dedx"  ,"dEdX-like deposited in paddle",  "fHits.ampl" },
    { "trdx",   "track deviation in x-position (m)", "fTrackProj.THaTrackProj.fdX" },
    { "trpad",  "paddle-hit associated with track",  "fTrackProj.THaTrackProj.fChannel" },
    { nullptr }
    };
    Int_t ret = DefineVarsFromList( vars, mode );
    if( ret )
    return ret;*/

    // Define general detector variables (track crossing coordinates etc.)
    // Objects in fDetectorData whose variables are not yet set up will be set up
    // as well. Our PMTData have already been initialized above & will be skipped.
    return THaNonTrackingDetector::DefineVariables(mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
MOLLERTriggerScintillator::~MOLLERTriggerScintillator()
{
    RemoveVariables();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset per-event data
void MOLLERTriggerScintillator::Clear(Option_t* opt)
{
    // Reset per event data
    cout << "In Clear function" << endl;

    THaNonTrackingDetector::Clear(opt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load data 
OptUInt_t MOLLERTriggerScintillator::LoadData( const THaEvData& evdata,
    const DigitizerHitInfo_t& hitinfo )
{
// Callback from Decoder for loading the data for the 'hitinfo' channel.
// This routine supports FADC modules and returns the pulse amplitude integral.
// Additional info is retrieved from the FADC modules in StoreHit later.

if( hitinfo.type == Decoder::ChannelType::kMultiFunctionADC ) return FADCData::LoadFADCData(hitinfo);

cout << "In LoadData function" << endl;

return THaNonTrackingDetector::LoadData(evdata, hitinfo);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store Hit
Int_t MOLLERTriggerScintillator::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Called from Decode().
  // Data decoding is also done here - from FADCData
  // Call StoreHit for the FADC modules first to get updated pedestals
  FADCData* fadcData = fPMTs;
  fadcData->StoreHit(hitinfo, data);

  cout << "In StoreHit function" << endl;

  // Retrieve pedestal, if available, and update the PMTData calibrations
  // Just added the function

  // Now fill the PMTData in fDetectorData
  return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print decoded data for debugging
void MOLLERTriggerScintillator::PrintDecodedData(const THaEvData& evdata) const
{
    // Fill this up
    // Left/Right PMTs
    cout << "In PrintDecodedData function" << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator coarse processing
Int_t MOLLERTriggerScintillator::CoarseProcess(TClonesArray& tracks)
{
    cout << "In CoarseProcessing" << endl;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator fine processing
Int_t MOLLERTriggerScintillator::FineProcess(TClonesArray& tracks)
{
    cout << "In FineProcessing" << endl;
    return 0;
}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(HallA::MOLLERTriggerScintillator)
