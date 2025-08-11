//////////////////////////////////////////////////////////////////////////////////////
// Trying a test scintillator class according to JLab Hall A SDK
// Following UserDetector
//
//////////////////////////////////////////////////////////////////////////////////////

#include "MOLLERTestScint.h"
#include "FADCData.h"
#include "VarDef.h"
#include "THaDetMap.h"
#include "TMath.h"
#include "Helper.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "Fadc250Module.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace Podd;

namespace HallA {

// Hard coded maximum number of channels
static const int MAXCHAN = 100;

// Initial basic constructor
//////////////////////////////////////////////////////////////////////////////////////
MOLLERTestScint::MOLLERTestScint(const char* name, const char* description,
                                THaApparatus* apparatus) :
                                THaNonTrackingDetector(name, description, apparatus)
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
    // See analyzer/SDK for more information about opening the database file
    const char* const here = "RaedDatabase";

    FILE* file = OpenFile(date);
    if (!file) return kFileError;

    // Clear all variables and vectors
    Int_t nelem = 0;
    Double_t angle = 0.0;
    fPed.clear();
    fGain.clear();

    // Detector map
    vector<Int_t> detmap;

    // Read the DB. Using a try block to handle exeptions
    Int_t err = 0;
    try {
        // Ignoring the automatic data type determination for now
        const DBRequest request[] = {
            // Required items
            {"detmap",      &detmap,        kIntV},
            {"nelem",       &nelem,         kInt,       0,      false,      -1}, // Number of element. eg. PMTs
            {"angle",       &angle,         kDouble,    0,  true}, // Rotational angle (this is optional)
            {nullptr} // Last element should be a null pointer
        };

        // Read the requested values
        err = LoadDB(file, date, request);

        // If no error, parse the detmap. See THaDetMap for more details about flags
        if (err == kOK) {
            if (FillDetMap(detmap, THaDetMap::kFillLogicalChannel, here) <= 0) {
                err = kInitError;
            }
        }
    }

    // Catch the exeptions and close the file
    catch(...) {
        fclose(file);
        throw;
    }

    // Normal end of reading the DB
    fclose(file);
    if (err != kOK) return err;

    // Sanity checks
    // This has to modify according to MOLLER trigger scintillator 
    // Using the following parameters as an example
    if (nelem <= 0) {
        Error (Here(here), "Cannot have a zero or negative number of elements. "
                "Fix the DB.");
        return kInitError;
    }
    if (nelem > MAXCHAN) {
        Error ( Here(here), "Illegal number of elements = %d. Must be <= %d. "
	            "Fix database.", nelem, MAXCHAN );
        return kInitError;
    }

    // Prevent the global variables dynamic allocation
    if (fIsInit && nelem !=fNelem) {
        ostringstream ostr;
        ostr << "Cannot re-initialize with different number of elements. "
            << "(was: " << fNelem << ", now: " << nelem << "). "
            << "Detector not re-initialized.";
        Error( Here(here), "%s", ostr.str().c_str() );
        return kInitError;
    }

    // Store nelem
    fNelem = nelem;

    // Check the detector map size
    // Assuming each detector element uses only one hardware channel, for now
    // Have to change this appropreately
    UInt_t nchan = fDetMap->GetTotNumChan(), nval = nelem;
    if (nchan != nval) {
        ostringstream ostr;
        ostr << "Incorrect number of detector map channels = " << nchan
            << ". Must equal nelem = " << nval << ". Fix database.";
        Error( Here(here), "%s", ostr.str().c_str() );
        return kInitError;
    }

    // Sanity checks for pedestals and gains
    // Add these if necessary

    // If any of the pedestal or gain values are not given in the DB, set them to default values
    if (fPed.empty()) fPed.assign(nelem, 0.0);
    if (fGain.empty()) fGain.assign(nelem, 1.0);

    // Use the Rotation angle to set the axes vectors
    const Double_t degrad = TMath::Pi()/180.0;
    DefineAxes(angle*degrad);

    using namespace Decoder;
    fFadcModules.clear(); // if you define fFadcModules as vector<Fadc250Module*>
    
    for (UInt_t i = 0; i < fDetMap->GetSize(); ++i) {
        /*const THaDetMap::Module* mod = fDetMap->GetModule(i);
        Int_t crate = mod->crate;
        Int_t slot  = mod->slot;*/
    
        TString fFadcName;
        //fadcName.Form("crate%uslot%u", crate, slot);
    
        fFadc = dynamic_cast<Fadc250Module*>(
            FindModule(fFadcName.Data(), "Fadc250Module", /*do_error=*/true));
        if (!fFadc) {
            Error("ReadDatabase", "FADC250 module %s not found", fFadcName.Data());
            return kInitError;
        }
    
        fFadcModules.push_back(fFadc);
    }

    // Finish up
    fIsInit = true;
    return kOK;
}

// Define/delete global vars
//////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::DefineVariables(EMode mode)
{
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

// Adding LoadData function according to FADCScintillator
///////////////////////////////////////////////////////////////////////////////////////////////////////////
OptUInt_t MOLLERTestScint::LoadData( const THaEvData& evdata,
    const DigitizerHitInfo_t& hitinfo )
{
    // Callback from Decoder for loading the data for the 'hitinfo' channel.
    // This routine supports FADC modules and returns the pulse amplitude integral.
    // Additional info is retrieved from the FADC modules in StoreHit later.

    cout << "hit type: " << static_cast<int>(hitinfo.type) << endl;

    // Only handle FADC hits directly
    if (hitinfo.type == Decoder::ChannelType::kMultiFunctionADC) {
        // Loop over all FADC modules to find matching crate/slot
        for (const auto& fadc : fFadcModules) {
            if (!fadc) continue;

            if (fadc->GetCrate() == hitinfo.crate && fadc->GetSlot() == hitinfo.slot) {
                UInt_t chan_hw = hitinfo.chan;
                UInt_t npulses = fadc->GetNumFadcEvents(chan_hw);
                if (npulses > 0) {
                    // Get the pulse integral (e.g., from first pulse)
                    UInt_t pulse_integral = fadc->GetEmulatedPulseIntegralData(chan_hw);
                    return pulse_integral;
                } else {
                    return 0; // No pulse data
                }
            }
        }

        // If no matching module found
        return 0;
    }

    // Fallback for legacy modules
    return THaNonTrackingDetector::LoadData(evdata, hitinfo);
}

// Store decoded data
// See SDK/UserDetector.cxx for more info
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Int_t MOLLERTestScint::StoreHit(const DigitizerHitInfo_t& hitinfo, UInt_t data)
{
    Int_t chan = hitinfo.lchan; // Logical channel according to detmap

    // Bug check if "chan" is in range
    #ifndef NDEBUG
        if (chan < 0 || chan >= fNelem)
        throw std::logic_error("MOLLERTestScint::StoreHit: invalid logical channel");
    #endif

    // Default values
    Double_t adc = (data - fPed[chan]) * fGain[chan];
    Double_t time = -999.0;

    // Loop over all FADC modules to find the one matching this hit
    for (const auto& fadc : fFadcModules) {
        if (!fadc) continue;

        if (fadc->GetCrate() == hitinfo.crate && fadc->GetSlot() == hitinfo.slot) {
            UInt_t npulses = fadc->GetNumFadcEvents(hitinfo.chan);
            if (npulses > 0) {
                UInt_t raw_time = fadc->GetPulseTimeData(hitinfo.chan, 0);
                constexpr Double_t tick_ns = 4.0; // 250 MHz
                time = raw_time * tick_ns;
            }
            break; // We found the right module
        }
    }
    // Store into your internal structure
    fEventData.emplace_back(chan, data, adc, time);
    return 0;
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
void MOLLERTestScint::Print(Option_t* opt) const
{
    THaDetector::Print(opt);
    cout << "detmap = "; fDetMap->Print();
    cout << "nelem = " << fNelem << endl;
    /*cout << "pedestals = "; PrintArray( fPed );
    cout << "gains = ";     PrintArray( fGain );*/
    cout << "nhits = " << fEventData.size() << endl;
    PrintArrayField("channel", fEventData, fChannel)
    PrintArrayField("rawadc", fEventData, fRawADC)
    PrintArrayField("coradc", fEventData, fCalADC)
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace HallA
ClassImp(HallA::MOLLERTestScint)
