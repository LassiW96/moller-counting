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

using namespace std;
using namespace Podd;

// Hard coded maximum number of channels
static const int MAXCHAN = 100;

// Initial basic constructor
//////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::MOLLERTestScint(const char* name, const char* description,
                                THaApparatus* apparatus) :
                                THaNonTrackingDetector(name, description, apparatus), fPMT(nullptr)
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

    //VarType kDataType  = std::is_same<Data_t, Float_t>::value ? kFloat  : kDouble;
    //VarType kDataTypeV = std::is_same<Data_t, Float_t>::value ? kFloatV : kDoubleV;

    FILE* file = OpenFile(date);
    if (!file) return kFileError;

    // Read fOrigin and fsize (required!)
    Int_t err = ReadGeometry(file, date, true);
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
        { "model_in_detmap",    &model_in_detmap,   kInt,   0,  true},
        { "chanmap",            &chanmap,           kIntV },
        { "ncols",              &ncols,             kInt },
        { "nrows",              &nrows,             kInt },
        { nullptr }
    };

    err = LoadDB(file, date, config_request, fPrefix);

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
    std::cout << "[DEBUG] Def var = " << std::endl;

    // Define more variables as required
    // Only including following for now
    RVarDef vars[] = {
        {"nhits",       "Number of hits",       "GetNhits()"},
        {"chan",        "Channel number",       "fEventData.fChannel"},
        {"adc",         "Raw ADC value",        "fEventData.fRawADC"},
        {"adc_c",       "Calibrated ADC value", "fEventData.fCalADC"},
        {nullptr}
    };
    return DefineVarsFromList(vars, mode);
}

// Clear per-event data - this is called before Decode() function
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void MOLLERTestScint::Clear(Option_t* opt)
{
    THaNonTrackingDetector::Clear(opt);
    fEventData.clear();
}

// Adding a decode method according to THaScintillator
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::Decode( const THaEvData& evdata )
{

    std::cout << "[DEBUG] in Decode = " << std::endl;
  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.
  // Additionally, apply timewalk corrections and find "paddle hits" (= hits
  // with TDC signals on both sides).

  THaNonTrackingDetector::Decode(evdata);
  // Figure out how to put LoadData and StoreHit here.

  return kOK;
}

// Adding LoadData function according to FADCScintillator
///////////////////////////////////////////////////////////////////////////////////////////////////////////
OptUInt_t MOLLERTestScint::LoadData( const THaEvData& evdata,
    const DigitizerHitInfo_t& hitinfo )
{
    std::cout << "[DEBUG] in Load data= " << std::endl;

// Callback from Decoder for loading the data for the 'hitinfo' channel.
// This routine supports FADC modules and returns the pulse amplitude integral.
// Additional info is retrieved from the FADC modules in StoreHit later.

// figure this out
std::cout << "[DEBUG] ChannelType = " << static_cast<int>(hitinfo.type) << std::endl;
if (hitinfo.type == Decoder::ChannelType::kMultiFunctionADC) {
    std::cout << "[DEBUG] Identified kMultiFunctionADC\n";
    return HallA::FADCData::LoadFADCData(hitinfo);
}

// Fallback to legacy modules
return THaNonTrackingDetector::LoadData(evdata, hitinfo);
//if (ret) return ret;

//return 0;
}

// Store decoded data
// See SDK/UserDetector.cxx for more info
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data)
{
    std::cout << "[DEBUG] in StoreHit = " << std::endl;
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
}

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
