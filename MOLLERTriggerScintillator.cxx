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
#include "TMath.h"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <memory>

using namespace std;
using namespace Podd;

#if __cplusplus >= 201402L
# define MKPMTDATA(name,title,nelem) make_unique<PMTData>((name),(title),(nelem))
#else
# define MKPMTDATA(name,title,nelem) unique_ptr<PMTData>(new PMTData((name),(title),(nelem)))
#endif

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

    cout << "Value of flags: " << flags << endl;

    Int_t test = fDetMap->Fill(detmap, flags);
    cout << "Value of ret: " << test << endl;

    fclose(file);
    return kOK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define global analysis variables
Int_t MOLLERTriggerScintillator::DefineVariables(EMode mode)
{
    // Define global analysis variables
    return 0;
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
    THaNonTrackingDetector::Clear(opt);
    // Fill this up
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print decoded data for debugging
void MOLLERTriggerScintillator::PrintDecodedData(const THaEvData& evdata) const
{
    // Fill this up
    // Left/Right PMTs
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode Scintillator data
Int_t MOLLERTriggerScintillator::Decode(const THaEvData& evdata)
{
    // This was written according to TDC output data
    THaNonTrackingDetector::Decode(evdata);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADC/TDC corrections which are possible without tracking
Int_t MOLLERTriggerScintillator::ApplyCorrections()
{
    // Fill this up
    // Left/Right PMTs

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TDC timewalk correction
/*Data_t MOLLERTriggerScintillator::TimeWalkCorrection(Idx_t idx, Data_t adc)
{
    return 0;
}*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find paddles with TDC hits on both sides (likely true hits)
Int_t MOLLERTriggerScintillator::FindPaddleHits()
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator coarse processing
Int_t MOLLERTriggerScintillator::CoarseProcess(TClonesArray& tracks)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator fine processing
Int_t MOLLERTriggerScintillator::FineProcess(TClonesArray& tracks)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(MOLLERTriggerScintillator)
