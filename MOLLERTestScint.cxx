//////////////////////////////////////////////////////////////////////////////////////
// Trying a test scintillator class according to JLab Hall A SDK/UserDetector
//
// Also, following the tstfadc_main script in podd/hana_decode/apps to writea new
// decode function
//
//////////////////////////////////////////////////////////////////////////////////////

#include "MOLLERTestScint.h"
#include "FADCData.h"

using namespace std;
using namespace HallA;

// Hard coded maximum number of channels
static const int MAXCHAN = 100;

// Debug flag
bool DEBUG = 1;

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
    //std::cout << "[DEBUG] in RDB = " << std::endl;

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
    vector<Int_t> ncols;
    Int_t nrows = 1, nlayers = 1;
    Int_t nelem = 0;

    DBRequest config_request[] = {
        { "detmap",             &detmap,            kIntV },
        { "model_in_detmap",    &model_in_detmap,   kInt,   0,  true}, // Module number if mentioned in the db file.
        { "chanmap",            &chanmap,           kIntV },
        { "ncols",        &ncols,   kIntV, 0, false },
        { "nlayers",           &nlayers,          kInt,   1,  true },
        { "nrows",              &nrows,             kInt },
        { nullptr }
    };

    err = LoadDB(file, date, config_request, fPrefix);

    // Adding the folllwing part according to the GenericDetector -
    // this is how they handle number of detector elements, and
    // skipped / ref channels

    Int_t ntemp = ncols.size();
    for(Int_t r = ntemp, i = 0; r < nrows; r++,i++) {
    if(ncols[i%ntemp]<=0) {
        Error( Here(here), "ncols cannot have negative entries!");
        fclose(file);
        return kInitError;
    }
    ncols.push_back(ncols[i%ntemp]);
    }

    for (int r = 0; r < nrows; r++) {
        nelem += ncols[r]*nlayers;
    }
    assert(int(ncols.size()) == nrows);

    fNelem = nelem;
    int nskipped = 0;
    int nrefchans = 0;
    if (!chanmap.empty()) {
        for (auto i : chanmap) {
            if (i == -1) nskipped++;
            if (i == -1000) nrefchans++;
        }
    }

    UInt_t flags = THaDetMap::kFillRefIndex; // Specify reference index/channel
    if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
      cout<<"here"<<endl;
      err = kInitError;  // Error already printed by FillDetMap
    } else {
        nelem = fDetMap->GetTotNumChan() - nskipped - nrefchans; // Exclude skipped channels in count

        if ( nelem != fNelem) {
            Error( Here(here), "Number of crate module channels (%d) "
                "inconsistent with number of blocks (%d)", nelem, fNelem);
            err = kInitError;
        }
    }

    // Now initialize the FADCData object for this detector using new
    // FADCData class
    auto ret = HallA::MakeFADCData(date, this);
    if (ret.second)
        return ret.second; // Database error

    // Debug printout of detmap
    fDetMap->Print();

    fPMT = ret.first.get();
    fDetectorData.emplace_back(move(ret.first));

    // Calibration parameters need to be added

    fclose(file);
    fIsInit = true;
    return kOK;
}

// Define/delete global vars
//////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::DefineVariables(EMode mode)
{
    // Variables are defined in the FADCData class internally
    std::cout << "[DEBUG] in def var = " << std::endl;
    return fPMT->DefineVariables(mode);
}

// Clear per-event data - this is called before Decode() function
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void MOLLERTestScint::Clear(Option_t* opt)
{
    THaNonTrackingDetector::Clear(opt);
}

// How do we use the Decode function accordingly?
////////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::Decode( const THaEvData& evdata )
{
    // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
    // the data to the local data members.
    // Additionally, apply timewalk corrections and find "paddle hits" (= hits
    // with TDC signals on both sides).

    THaNonTrackingDetector::Decode(evdata);

    // From THaScintillator.cxx
    // ApplyCorrections();
    // FindPaddleHits();

    // What's a good return value here?
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load data 
OptUInt_t MOLLERTestScint::LoadData( const THaEvData& evdata,
    const DigitizerHitInfo_t& hitinfo )
{
    // Callback from Decoder for loading the data for the 'hitinfo' channel.
    // This routine supports FADC modules and returns the pulse amplitude integral.
    // Additional info is retrieved from the FADC modules in StoreHit later.

    //std::cout << "[DEBUG] in LoadData" << std::endl;
    std::cout << "Module type = " << static_cast<int>(hitinfo.modtype) << std::endl;

    if( hitinfo.modtype == Decoder::ChannelType::kMultiFunctionADC )
        return HallA::FADCData::LoadFADCData(hitinfo);

    // Fallback for legacy modules
    return THaNonTrackingDetector::LoadData(evdata, hitinfo);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store Hit
Int_t MOLLERTestScint::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Called from Decode().
  // Data decoding is also done here - from FADCData
  // Call StoreHit for the FADC modules first to get updated pedestals
  HallA::FADCData* fadcData = fPMT;
  fadcData->StoreHit(hitinfo, data);

  //cout << "In StoreHit function" << endl;

  // Retrieve pedestal, if available, and update the PMTData calibrations
  // Just added the function

  // Now fill the PMTData in fDetectorData
  return THaNonTrackingDetector::StoreHit(hitinfo, data);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(MOLLERTestScint)
