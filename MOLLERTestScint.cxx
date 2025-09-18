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

    // Loop over all modules defined for this detector
    bool has_warning = false;
    Int_t nhits = 0;

    auto hitIter = fDetMap->MakeIterator(evdata);

    while( hitIter ) {
    const auto& hitinfo = *hitIter;
    // should always assume that logical channel numbers start counting from zero.

    //Check if there are ny hits on the channel
    if( hitinfo.nhit > 0 ) {
        if (DEBUG) std::cout << "\nTestScint::Decode: number of hits (hitinfo.nhit) ="
                << hitinfo.nhit <<std::endl;

        // Multiple hits in a channel (usually noise)
        // For multifunction modules, assume "hit" is a data word index, so
        // don't log anything but assume the user simply wants the first word.
        if( hitinfo.modtype != Decoder::ChannelType::kMultiFunctionADC) {
        MultipleHitWarning(hitinfo, here);
        has_warning = true;
        }
    }

    //Get the data for this hit
    auto* fadc = dynamic_cast<Fadc250Module*>(hitinfo.module);
    
    if(fadc==nullptr){
        cout<<" No module found! exit "<<endl;
        return 0;
    }


    std::cout<<"TestScint::Decode: event number= "<<hitinfo.ev<<std::endl;


    // BEGIN NOTE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //following are from analyzer/hana_decode/apps/tstfadc_main.cxx
    //this is just a way to temp test the code while  waiting for the changes needed in FADCDetector class
    // for mode selection
    
    static const uint32_t NADCCHAN     = 16;
    
    // Loop over all channels
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
            
        // Acquire the FADC mode
        static int fadc_mode_const;
        Int_t fadc_mode = fadc->GetFadcMode(); fadc_mode_const = fadc_mode;
        
        if (DEBUG) std::cout<<"TestScint::Decode: fadc channel "<<chan
                <<" is in mode = "<<fadc->GetFadcMode()<<std::endl;

        /*    
        from https://www.jlab.org/Hall-B/ftof/manuals/FADC250UsersManual.pdf
        pg 39
        FADC readout processing modes are;
        Mode 0 Raw Mode:
        Samples in Window are read back
        
        Mode 1 Pulse Raw Mode:
        Samples in Window from NSB and
        NSA and Time only when Sample >
        Threshold (TET) are read back
        
        Mode 2 Integral Mode:
        SUM of samples in Window from NSB
        and NSA and Time only when Sample
        > Threshold (TET) are read back
        
        Mode 3 TDC Mode:
        Time when Vmid occurred, Vmin,
        Vmax are read back only when
        samples are greater then TET
        
        Note: I am not sure what the rest of the modes are 8, 10 eg. But for our purpose
        they are irrelavant.- Buddhini
        */
        
        // Check if the fadc is in a "raw" mode (see above)
        Bool_t raw_mode  = ((fadc_mode == 1) || (fadc_mode == 8) || (fadc_mode == 10));
        
        // Acquire the number of FADC events
        UInt_t num_fadc_events = fadc->GetNumFadcEvents(hitinfo.chan);
        if (DEBUG)
    std::cout<<"TestScint::Decode: fadc in chan = "<<chan
        <<" have events ="<<num_fadc_events<<std::endl;

        
        
        // If in raw mode, acquire the number of FADC samples
        UInt_t num_fadc_samples = 0;
        if (raw_mode) {
    num_fadc_samples = fadc->GetNumFadcSamples(chan, hitinfo.ev);
    if (DEBUG) 
        cout<<"TestScint:: For event "<<hitinfo.ev
            <<", number of events in channel "<<chan<<" is "<< num_fadc_events
            <<" & number of FADC samples is "<<num_fadc_samples<<endl;
    //return 0;
        }
        
        //what is the use of this ^ ?

        
        // if we have events
        if (num_fadc_events > 0) {

    // for all events do:
    for (UInt_t jevent = 0; jevent < num_fadc_events; jevent++) {
        
        // Debug output
        if (DEBUG) {
        cout<<"fadc samples "<<num_fadc_samples<<endl;
        }
        
        if ((fadc_mode == 1 || fadc_mode == 8) && num_fadc_samples > 0){
        cout << "FADC EMULATED PI DATA = "
        << fadc->GetEmulatedPulseIntegralData(hitinfo.chan) << endl;
        }
        if (fadc_mode == 7 || fadc_mode == 8 || fadc_mode == 9 || fadc_mode == 10) {
        if( fadc_mode != 8 )
            cout << "FADC PI DATA = "
            << fadc->GetPulseIntegralData(hitinfo.chan, jevent) << endl;
        
        cout << "FADC PT DATA = " << fadc->GetPulseTimeData(hitinfo.chan, jevent) << endl;
        // cout << "FADC PPED DATA = " << fadc->GetPulsePedestalData(hitinfo.chan, jevent) / NPED << endl;
        cout << "FADC PPEAK DATA = " << fadc->GetPulsePeakData(hitinfo.chan, jevent) << endl;
        }
    } //end event loop
        } // ened event>0 check
    } // end channels


    //if(hitinfo.hit < fadc->GetNumEvents(kPulseIntegral, hitinfo.chan)) std::cout<<"this is wrong"<<std::endl;
    //OptUInt_t val;
    //val = fadc->GetData(kSampleADC, hitinfo.chan, hitinfo.hit);
    
    //if( val == kMaxUInt ) std::cout<<"val is "<<kMaxUInt<<std::endl;
    
    EModuleType type = kSampleADC;
    
    assert(fadc);
    OptUInt_t val;
    if( fadc->HasCapability(type) &&
    hitinfo.hit < fadc->GetNumEvents(type, hitinfo.chan) ) {
        
        val = fadc->GetData(type, hitinfo.chan, hitinfo.ev);
        
        std::cout<<" val = "<<fadc->GetData(type, hitinfo.chan, hitinfo.ev)<<std::endl;
        
        if( val == kMaxUInt ) // error return code
    val = nullopt;
    }
    // Get the data for this hit
    
    auto data = val;
    
    // all of the above is from analyzer/hana_decode/apps/tstfadc_main.cxx
    // 
    // END NOTE!!!
    
    //this commented out line is replaces all of the above once FADCData mode selection is fixed.
    //auto data = HallA::FADCData::LoadFADCData(hitinfo);
    
    
    if( !data ) {
        std::cout << "TestScint::Decode = no data" << std::endl;
        
        // Data could not be retrieved (probably decoder bug)
        DataLoadWarning(hitinfo, here);
        has_warning = true;
        continue;
    }
    
    // Store hit data (and derived quantities) in fDetectorData.
    fPMT->HallA::FADCData::StoreHit(hitinfo, data.value());
    
    
    // Clear the hit-done flag which can be used in custom StoreHit methods
    // to reorder module processing
    for( auto& detData : fDetectorData )
        detData->ClearHitDone();
    
    // Next active channel
    ++hitIter;
    ++nhits;
    }

    if( has_warning )
    ++fNEventsWithWarnings;

    #ifdef WITH_DEBUG
    if ( fDebug > 3 )
    PrintDecodedData(evdata);
    #endif

    return nhits;

    // This calls the LoadData function
    THaNonTrackingDetector::Decode(evdata);

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
