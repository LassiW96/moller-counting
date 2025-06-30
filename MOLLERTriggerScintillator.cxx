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
                        fCn(0), fAttenuation(0), fResolution(0), 
                        fRightPMTs(nullptr), fLeftPMTs(nullptr), fPMTs(nullptr)
{
    // Which mode is going to use, Sigle PMT or Dual PMT
    fNviews = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Default constructor for ROOT RTTI
MOLLERTriggerScintillator::MOLLERTriggerScintillator()
    : THaNonTrackingDetector(),
    fCn(0), fAttenuation(0), fResolution(0), 
    fRightPMTs(nullptr), fLeftPMTs(nullptr), fPMTs(nullptr)
{
    // Fill the default constructor body if necessary
    fNviews = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read the Scintillators parameters from the database
Int_t MOLLERTriggerScintillator::ReadDatabase(const TDatime& date)
{
    // Read detector's parameters from the DB
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
    Int_t nelem = 0;

    /*DBRequest config_request[] = {

    };*/

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Define global analysis variables
Int_t MOLLERTriggerScintillator::DefineVariables(EMode mode)
{
    // Define global analysis variables
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
// Put decoded frontend data into fDetectorData
Int_t MOLLERTriggerScintillator::StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data)
{
    // Fill this up
    // Left/Right PMTs

    return 0;
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
Data_t MOLLERTriggerScintillator::TimeWalkCorrection(Idx_t idx, Data_t adc)
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find paddles with TDC hits on both sides (likely true hits)
Int_t MOLLERTriggerScintillator::FindPaddleHits()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator coarse processing
Int_t MOLLERTriggerScintillator::CoarseProcess(TClonesArray& tracks)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scintillator fine processing
Int_t MOLLERTriggerScintillator::FineProcess(TClonesArray& tracks)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(MOLLERTriggerScintillator)
