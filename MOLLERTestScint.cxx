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
using namespace Podd;

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
    Int_t ncols = 0;
    Int_t nrows = 0;
    Int_t nelem = 0;

    DBRequest config_request[] = {
        { "detmap",             &detmap,            kIntV },
        { "model_in_detmap",    &model_in_detmap,   kInt,   0,  true}, // Module number if mentioned in the db file.
        { "chanmap",            &chanmap,           kIntV },
        { "ncols",              &nelem,             kInt },
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

    // Calibration parameters need to be added

    fclose(file);

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
}

// Adding a decode method following THaDetectorBase
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::Decode( const THaEvData& evdata )
{
    // Decode raw data (evdata), and copy data to the local data members.


    const char* const here = "Decode";

    // // Loop over all modules defined for this detector
    // bool has_warning = false;
    // Int_t nhits = 0;

    // auto hitIter = fDetMap->MakeIterator(evdata);

    // while( hitIter ) {
    // const auto& hitinfo = *hitIter;
    // // should always assume that logical channel numbers start counting from zero.

    // //Check if there are ny hits on the channel
    // if( hitinfo.nhit > 0 ) {
    //     if (DEBUG) std::cout << "\nTestScint::Decode: number of hits (hitinfo.nhit) ="
    //             << hitinfo.nhit <<std::endl;

    //     // Multiple hits in a channel (usually noise)
    //     // For multifunction modules, assume "hit" is a data word index, so
    //     // don't log anything but assume the user simply wants the first word.
    //     if( hitinfo.modtype != Decoder::ChannelType::kMultiFunctionADC) {
    //     MultipleHitWarning(hitinfo, here);
    //     has_warning = true;
    //     }
    // }

    // //Get the data for this hit
    // auto* fadc = dynamic_cast<Fadc250Module*>(hitinfo.module);
    
    // if(fadc==nullptr){
    //     cout<<" No module found! exit "<<endl;
    //     return 0;
    // }
    // std::cout<<"TestScint::Decode: event number= "<<hitinfo.ev<<std::endl;

    // auto data = HallA::FADCData::LoadFADCData(hitinfo);
    
    // if( !data ) {
    //     std::cout << "TestScint::Decode = no data" << std::endl;
        
    //     // Data could not be retrieved (probably decoder bug)
    //     DataLoadWarning(hitinfo, here);
    //     has_warning = true;
    //     continue;
    // }
    
    // // Store hit data (and derived quantities) in fDetectorData.
    // fPMT->HallA::FADCData::StoreHit(hitinfo, data.value());
    
    
    // // Clear the hit-done flag which can be used in custom StoreHit methods
    // // to reorder module processing
    // for( auto& detData : fDetectorData )
    //     detData->ClearHitDone();
    
    // // Next active channel
    // ++hitIter;
    // ++nhits;
    // }

    // if( has_warning )
    // ++fNEventsWithWarnings;

    // #ifdef WITH_DEBUG
    // if ( fDebug > 3 )
    // PrintDecodedData(evdata);
    // #endif

    // return nhits;

    // This calls the LoadData function
    return THaNonTrackingDetector::Decode(evdata);

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////
ClassImp(MOLLERTestScint)
