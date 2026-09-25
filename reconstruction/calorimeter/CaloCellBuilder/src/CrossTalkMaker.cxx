#include "CrossTalkMaker.h"

#include "CaloCell/CaloDetDescriptor.h"
#include "CaloCell/CaloCellContainer.h"
#include "CaloCell/CaloDetDescriptorCollection.h"
#include "G4Kernel/CaloPhiRange.h"
#include "G4Kernel/constants.h"
#include "CaloCell/enumeration.h"

#include "G4SystemOfUnits.hh"

#include <map>
#include <random>

using namespace Gaugi;


/**
 * @class CrossTalkMaker
 * @brief Simulates cell-to-cell cross-talk effects.
 * 
 * This algorithm modifies the pulse shapes of calorimeter cells by mixing in
 * contributions from their neighbors. It models both capacitive and inductive
 * coupling. It creates a new collection of "cross-talked" cells.
 * 
 * Properties:
 * - AmpCapacitive/Inductive: Coupling amplitudes.
 * - MinEnergy: Process only cells above this energy threshold.
 */
CrossTalkMaker::CrossTalkMaker( std::string name ) : 
  IMsgService(name),
  Algorithm()
{

  declareProperty( "InputCollectionKey"     , m_collectionKey="Cells"               ); // input
  declareProperty( "OutputCollectionKey"    , m_xtcollectionKey="XTCells"           ); // output
  declareProperty( "SigmaNoiseCut"          , m_sigmaNoiseCut=0                     );
  declareProperty( "HistogramPath"          , m_histPath="/CrossTalkMakerSimulator" );
  declareProperty( "OutputLevel"            , m_outputLevel=1                       );
  declareProperty( "AmpCapacitive"          , m_AmpXt_C=4.2                         );
  declareProperty( "AmpInductive"           , m_AmpXt_L=2.3                         );
  declareProperty( "AmpResistive"           , m_AmpXt_R=1.0                         );
  declareProperty( "XtStdDevCap"            , m_RMSXt_C=0.25                        );
  declareProperty( "XtStdDevInd"            , m_RMSXt_L=0.25                        );
}\

//!=====================================================================

StatusCode CrossTalkMaker::initialize()
{
  CHECK_INIT();
  setMsgLevel(m_outputLevel);

  // initialize tools
  for ( auto tool : m_toolHandles )
  {
    if (tool->initialize().isFailure() )
    {
      MSG_FATAL( "It's not possible to iniatialize " << tool->name() << " tool." );
    }
  }
  return StatusCode::SUCCESS;
}
//!=====================================================================

void CrossTalkMaker::push_back( Gaugi::AlgTool* tool )
{
  m_toolHandles.push_back(tool);
}

//!=====================================================================

StatusCode CrossTalkMaker::bookHistograms( SG::EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CrossTalkMaker::pre_execute( SG::EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CrossTalkMaker::execute( SG::EventContext &/*ctx*/ , const G4Step * /*step*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

/**
 * @brief Executes the crosstalk simulation within a 3x3 window around the central cell.
 * 
 * 1. Copies the original cells to a new container and iterates over valid central cells (above threshold and in specific calo layers).
 * 2. Builds the 3x3 window around the central cell.
 * 3. Calculates the distorted pulse for the central cell by summing contributions from neighbors (XTalkTF) with energy conservation corrections.
 * 4. Updates the central cell's pulse with the distorted version.
 * 5. Changes pulse value on central cell with adjacent crosstalk effects.
 * 6. Consolidates charge excess corrections.
 * 7. Runs downstream estimation tools (e.g. OptimalFilter) on the modified cells.
 */
StatusCode CrossTalkMaker::execute( SG::EventContext &ctx , int /*evt*/ ) const
{
  MSG_INFO("Executing CrossTalkMaker module...");

  std::vector < float > samples_xtalk_ind    ;
  std::vector < float > samples_xtalk_cap    ;
  std::vector < float > samples_signal       ;
  std::vector < float > samples_signal_xtalk ;

  std::random_device rd; // random device class instance, source of 'true' randomness for initializing random seed  
  std::mt19937 gen(rd()); // Mersenne twister PRNG, initialized with seed from previous random device instance

  SG::ReadHandle<xAOD::CaloDetDescriptorCollection> collection( m_collectionKey, ctx );

  MSG_INFO( "Creating reco XT cells containers with key " << m_xtcollectionKey);
  SG::WriteHandle<xAOD::CaloDetDescriptorCollection> xtCollection( m_xtcollectionKey , ctx );
  xtCollection.record( std::unique_ptr<xAOD::CaloDetDescriptorCollection>(new xAOD::CaloDetDescriptorCollection()) );
  
  // create xt energy excess container for further corrections
  SG::WriteHandle<xAOD::CaloDetDescriptorCollection> xtEneExcess( "XTDescriptorEneExcess" , ctx );
  xtEneExcess.record( std::unique_ptr<xAOD::CaloDetDescriptorCollection>(new xAOD::CaloDetDescriptorCollection()) );
  
  MSG_DEBUG("Before execution: collection.size: "<< collection->operator*().size() << ", xtCollection.size(): "<< xtCollection->operator*().size());


  // loop over ordinary cell container
  for (const auto &pair : **collection.ptr() )
  {
    auto descriptor = pair.second;
    xAOD::CaloDetDescriptor *xtdescriptor = descriptor->copy();

    auto pulseBefore  = descriptor->pulse();
    auto energyBefore = descriptor->e();

    // Step 1: check if we need to apply cx method for current cell. Only for cells higher than
    // min energy. Here, lets use the truth energy from the main bunch crossing
    // 
    // BEFORE: m_SigmaNoiseCut*xtdescriptor->noise().
    bool bCrossTalkMakerConditions = ( !(xtdescriptor->edep() < m_sigmaNoiseCut) && !(xtdescriptor->pulse().size() == 0) && !((xtdescriptor->sampling() != CaloSampling(EMB2)) && (xtdescriptor->sampling() != CaloSampling(EMEC2))) );

    // ------------------------------------------------------------------------------
    // If there IS xtalk conditions, apply XT model to cell 1st neighbors,
    // change the current cell pulse, then add cell to new XT cell container.
    // ------------------------------------------------------------------------------
    if (bCrossTalkMakerConditions)
    {

      MSG_DEBUG("Sampling/Detector "<< xtdescriptor->sampling() <<"/"<< xtdescriptor->detector() <<", hash "<< xtdescriptor->hash() 
                << ", nsamples " << xtdescriptor->pulse().size() << ", truthEne "<< xtdescriptor->edep() << ", ene " << xtdescriptor->e() 
                << ", eta/phi "<< xtdescriptor->eta() << "/"<< xtdescriptor->phi() );

      // Step 2: build a 3x3 window around the central cell.
      //    Since this is a cell candidate, lets take all cells around this cells using a 3x3 window.
      //    First lets retrieve the full container in memory (not const objects inside of the collection)
      std::vector<const xAOD::CaloDetDescriptor*> cells_around;

      // loop over ordinary cell container
      for (auto &pairAround : **collection.ptr() ){
        auto neighborDescriptor = pairAround.second;
        if ( neighborDescriptor->pulse().size() == 0) continue; // protection: if there is no pulseShape, skip that cell. 
        if ( xtdescriptor->sampling() != neighborDescriptor->sampling() ) continue;  // cells_around must belong to the same sampling of central_cell
        if ( xtdescriptor->hash() == neighborDescriptor->hash()) continue; // central_cell must not belong to cells_around
       
        // build a 3x3 window around the central cell
        float diffEta = std::abs( xtdescriptor->eta() - neighborDescriptor->eta() );
        float diffPhi = std::abs( CaloPhiRange::fix( xtdescriptor->phi() - neighborDescriptor->phi() ) );
        if( diffEta <= 3*xtdescriptor->deltaEta()/2 && diffPhi <= 3*xtdescriptor->deltaPhi()/2 ){
          cells_around.push_back(neighborDescriptor);
        }
      }

      // Step 3: Loop over cells_around to extract xtalk effect from the central_cell surroundings.
      std::vector<float> final_xt_pulse(5);
      std::vector<float> leaked_xt_pulse(5);

      for (auto cell : cells_around){

        // Compute XT amplitude values with uncertainties (in %)
        std::normal_distribution<float> cap_amp_normal( m_AmpXt_C , m_RMSXt_C );
        std::normal_distribution<float> ind_amp_normal( m_AmpXt_L , m_RMSXt_L );

        // get random number with normal distribution using gen as random source
        float leakedXTAmp_cap = cap_amp_normal(gen);
        float leakedXTAmp_ind = ind_amp_normal(gen);
    
        auto pulseCellXT = cell->pulse();
        std::vector<float> neighbor_xt_pulse;
        float distorted_sample_ind=0, distorted_sample_cap=0;
        
        // case 1: diagonal from central cell
        if (cell->eta() != descriptor->eta() && cell->phi() != descriptor->phi())
        {
          for (unsigned samp_index=0; samp_index<5; ++samp_index)
          {
            distorted_sample_ind = XTalkTF(pulseCellXT[samp_index], samp_index, true, true, leakedXTAmp_cap, leakedXTAmp_ind);
            distorted_sample_cap = 0; // there is no capacitive cross-talk effect in the cell diagonal
            samples_xtalk_ind.push_back(distorted_sample_ind); // histogram
            samples_xtalk_cap.push_back(distorted_sample_cap);  // histogram
            neighbor_xt_pulse.push_back(distorted_sample_ind + distorted_sample_cap);
            // leaked_xt_pulse[samp_index] = pulseCellXT[samp_index]*m_AmpXt_L/100; // it leaks using the same pulse shape.
            leaked_xt_pulse[samp_index] = pulseCellXT[samp_index]*leakedXTAmp_ind/100; // it leaks using the same pulse shape.
          }
        }else {
          // case 2: is inside central cross position        
          for (int samp_index=0; samp_index<5; samp_index++)
          {
            distorted_sample_ind = XTalkTF(pulseCellXT[samp_index], samp_index, true, true, leakedXTAmp_cap, leakedXTAmp_ind);
            distorted_sample_cap = XTalkTF(pulseCellXT[samp_index], samp_index, true, false, leakedXTAmp_cap, leakedXTAmp_ind);
            samples_xtalk_ind.push_back(distorted_sample_ind); // histogram
            samples_xtalk_cap.push_back(distorted_sample_cap); // histogram
            neighbor_xt_pulse.push_back(distorted_sample_ind + distorted_sample_cap);

            leaked_xt_pulse[samp_index] = pulseCellXT[samp_index]*leakedXTAmp_ind/100 + pulseCellXT[samp_index]*leakedXTAmp_cap/100; // it leaks using the same pulse shape (for correction).
          }
        }

        // sum all xtalk effects around center cell
        for (int i=0; i<5; i++){
          final_xt_pulse[i] += neighbor_xt_pulse[i];
        }

        // Energy Conservation Correction
        // Try get the current neighbor cell descriptor from 'excess energy container.
        // Here, if that descriptor hash already exists in the container, then integrate the new computed energy to its 'Pulse'.
        xAOD::CaloDetDescriptor *excessEneDescriptor=nullptr;
        if ( xtEneExcess->retrieve( cell->hash(), excessEneDescriptor ) ){
          std::vector<float> integratedEne(5);
          std::vector<float> oldPulse = excessEneDescriptor->pulse();
          for (int i=0; i<5; i++){
            // integratedEne[i] = oldPulse[i] + neighbor_xt_pulse[i];
            integratedEne[i] = oldPulse[i] + leaked_xt_pulse[i];
          }

          MSG_DEBUG("(XTChargeConservation) Hash "<<cell->hash() << " exists on container, integrating its energy from "<< excessEneDescriptor->pulse() <<", to " << integratedEne);
          excessEneDescriptor->setPulse(integratedEne);
        }
        else{ // if hash do not exist in container, add new descriptor to it.
          auto *newExcessEneDescriptor = new xAOD::CaloDetDescriptor( *(cell) );
          // newExcessEneDescriptor->setPulse( neighbor_xt_pulse );
          newExcessEneDescriptor->setPulse( leaked_xt_pulse );

          if (!xtEneExcess->insert(newExcessEneDescriptor->hash() , newExcessEneDescriptor)){
            MSG_FATAL("(XTChargeConservation) Descriptor is unique and it's not possible to insert new descriptor into collection!");
            return StatusCode::FAILURE;
          }
          MSG_DEBUG("(XTChargeConservation) New cell added with hash "<< newExcessEneDescriptor->hash() << " and signal " << newExcessEneDescriptor->pulse());
        }

      }// end-for in cells_around

      // Step 4: add total pulse distortion from neighbor cells into the central cell of the 3x3 window.
      auto centralCellPulse = xtdescriptor->pulse(); 

      for (int i=0; i<5; i++)
      {
        samples_signal.push_back(centralCellPulse[i]); // add to fillHistograms
        centralCellPulse[i] = centralCellPulse[i] + final_xt_pulse[i]; 
        samples_signal_xtalk.push_back(centralCellPulse[i]); // add to fillHistograms
      }

      // Step 5: change pulse value of central cell of the 3x3 window with adjacent xtalk effects.
      xtdescriptor->setPulse(centralCellPulse);

      MSG_DEBUG("Cell "<< descriptor->hash() <<", sampling "<< descriptor->sampling() <<", pulse() = "<< descriptor->pulse() << ", edep/tof= "<< descriptor->edep() <<"/"<< descriptor->tof() <<", e/tau=" << descriptor->tau() << "/"<< descriptor->e() );
      

      samples_xtalk_ind.clear();
      samples_xtalk_cap.clear();
      samples_signal.clear();
      samples_signal_xtalk.clear(); 
    }

    // ------------------------------------------------------------------------------
    // If there is NO xtalk conditions, add the cell normally into new XT Container.
    //  Look, here, the current descriptor hasn't been changed.
    // -------------------------------------------------------------------------------
    if ( !xtCollection->insert( xtdescriptor->hash(), xtdescriptor ) ){
        MSG_FATAL( "It is not possible to include cell hash ("<< xtdescriptor->hash() << ") into the collection. Hash already exist.");
    }

  }

  // Step 6: Correct for charge excess
  // for (auto excessEneDescriptor : *xtEneExcess){
  for (auto &pair : **xtCollection){

    xAOD::CaloDetDescriptor *xtdescriptor = pair.second;
    xAOD::CaloDetDescriptor *excessDescriptor = nullptr;

    if ( !xtEneExcess->retrieve( xtdescriptor->hash() , excessDescriptor) ){
      MSG_ERROR("Cannot find hash "<< xtdescriptor->hash()  << " on EnergyExcess Collection!");
      continue;
    }

    std::vector<float> correctedPulse(5);
    std::vector<float> xtCellPulse = xtdescriptor->pulse();
    std::vector<float> excessPulse = excessDescriptor->pulse();

    for (int i=0; i<5; i++){
      correctedPulse[i] = xtCellPulse[i] - excessPulse[i];
    }
    xtdescriptor->setPulse(correctedPulse);

    // Step 7: Call for Estimation Methods tool
    for ( auto tool : m_toolHandles )
    {
      // digitalization
      if( tool->execute( ctx, xtdescriptor ).isFailure() ){
        MSG_ERROR( "It's not possible to execute the tool with name " << tool->name() );
        return StatusCode::FAILURE;
      }
    }
    
    MSG_DEBUG("(ChargeCorrection) Hash " << xtdescriptor->hash() << " corrected pulse from " << xtCellPulse << " to " << xtdescriptor->pulse() << ", e/tau = " << xtdescriptor->e() << "/" << xtdescriptor->tau());
    MSG_DEBUG("XTCell " << xtdescriptor->hash() << ", sampling " << xtdescriptor->sampling() << ", pulse() = " << xtdescriptor->pulse() << ", edep/tof= " << xtdescriptor->edep() << "/" << xtdescriptor->tof() );
  }

  return StatusCode::SUCCESS;
  
  // return post_execute(ctx);

}


//!=====================================================================


StatusCode CrossTalkMaker::fillHistograms( SG::EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}


//!=====================================================================


StatusCode CrossTalkMaker::finalize()
{
  // MSG_INFO("Finalizing CrossTalkMaker module...");
  for ( auto tool : m_toolHandles )
  {
    if (tool->finalize().isFailure() )
    {
      MSG_ERROR( "It's not possible to finalize " << tool->name() << " tool." );
    }
  }

  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CrossTalkMaker::post_execute( SG::EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}


float CrossTalkMaker::XTalkTF(float sample, int samp_index, bool diagonal, bool inductive, float cap_xt_amp, float ind_xt_amp) const
{

  // float BaseAmpXTc = m_AmpXt_C/100*sample ;
  // float BaseAmpXTl = m_AmpXt_L/100*sample ;
  float BaseAmpXTc = cap_xt_amp/100*sample ;
  float BaseAmpXTl = ind_xt_amp/100*sample ;
  // float BaseAmpXTr = m_AmpXt_R*sample ;
  float XTcSamples = BaseAmpXTc * XTalk       (25*(samp_index+1) , false ); //+ delayPerCell[cell] + m_tau_0, false ) ) ;
  float XTlSamples = BaseAmpXTl * XTalk       (25*(samp_index+1) , false ); //+ delayPerCell[cell] + m_tau_0, false ) ) ;
  // float XTrSamples = BaseAmpXTr * CellFunction(25*(samp_index+1) , false ); //+ delayPerCell[cell] + m_tau_0, false ) ) ;

  if (diagonal && inductive){
    // ind_part = XTlSamples;
    return XTlSamples;
  }
  else{
    // return XTcSamples + XTlSamples;
    if (inductive){
      return XTlSamples;
      }
    else{
      return XTcSamples;
    }
  }
  // SampClusNoise.push_back( noise->Gaus(0, 2) ) ;

}

double CrossTalkMaker::XTalk(double x, bool type) const
{
  TF1* XT_cellTF = new TF1("XT_cellTF","((exp(-x/[0])*x*x)/(2 *[0]*[0]*([0] - [1])) - (exp(-(x/[0]))*x*[1])/([0]*pow([0] - [1],2)) + exp(-(x/[0]))*[1]*[1]/pow([0] - [1],3) + (exp(-(x/[1]))*[1]*[1])/pow(-[0] + [1],3) + (1/(2*[2]*[0] *pow(([0] - [1]),3)))*exp(-x* (1/[0] + 1/[1]))* (-2 *exp(x *(1/[0] + 1/[1]))*[0] *pow(([0] - [1]),3) - 2 *exp(x/[0])*[0]*pow([1],3) + exp(x/[1]) *(x*x *pow(([0] - [1]),2) + 2*x*[0]*([0]*[0] - 3*[0]*[1] + 2*[1]*[1]) + 2*[0]*[0]*([0]*[0] - 3*[0]*[1] + 3*[1]*[1]))) + ((1 - (exp((-x + [2])/[0])*(x - [2])*([0] - 2*[1]))/pow(([0] - [1]),2) - (exp((-x + [2])/[0])*(x - [2])*(x- [2]))/(2*[0]*([0] - [1])) + (exp((-x + [2])/[1])*[1]*[1]*[1])/pow(([0] - [1]),3) - (exp((-x + [2])/[0])*[0]*([0]*[0] - 3*[0]*[1] + 3*[1]*[1]))/pow(([0] - [1]),3))* 0.5*( 1+sign(1, x -[2]) ) )/[2])*[3]*[4]*[3]*[0]*[0]",0., m_tmax2);
  XT_cellTF->SetParameter(0, m_taud);
  XT_cellTF->SetParameter(1, m_taupa);
  XT_cellTF->SetParameter(2, m_td);
  XT_cellTF->SetParameter(3, m_Rf);
  XT_cellTF->SetParameter(4, m_C1);

  double xt_cell = 0 ;

  if (type){
    xt_cell = XT_cellTF->Derivative(x) ;
  }
  else {
    xt_cell = XT_cellTF->Eval(x) ;
  }

  delete XT_cellTF ;

  return xt_cell ;
}// end of function


double CrossTalkMaker::CellFunction(double x, bool type) const
{
  TF1* CellM = new TF1("CellM","[5]*((exp(-x/[0])*x*x)/(2 *[0]*[0]*([0] - [1])) - (exp(-(x/[0]))*x*[1])/([0]*pow([0] - [1],2)) + exp(-(x/[0]))*[1]*[1]/pow([0] - [1],3) + (exp(-(x/[1]))*[1]*[1])/pow(-[0] + [1],3) + (1/(2*[2]*[0] *pow(([0] - [1]),3)))*exp(-x* (1/[0] + 1/[1]))* (-2 *exp(x *(1/[0] + 1/[1]))*[0] *pow(([0] - [1]),3) - 2 *exp(x/[0])*[0]*pow([1],3) + exp(x/[1]) *(x*x *pow(([0] - [1]),2) + 2*x*[0]*([0]*[0] - 3*[0]*[1] + 2*[1]*[1]) + 2*[0]*[0]*([0]*[0] - 3*[0]*[1] + 3*[1]*[1]))) + ((1 - (exp((-x + [2])/[0])*(x - [2])*([0] - 2*[1]))/pow(([0] - [1]),2) - (exp((-x + [2])/[0])*(x - [2])*(x- [2]))/(2*[0]*([0] - [1])) + (exp((-x + [2])/[1])*[1]*[1]*[1])/pow(([0] - [1]),3) - (exp((-x + [2])/[0])*[0]*([0]*[0] - 3*[0]*[1] + 3*[1]*[1]))/pow(([0] - [1]),3))* 0.5*( 1+sign(1,x -[2]) ) )/[2])*[3]*[4]*[3]*[0]*[0]",0., m_tmax2);

  CellM->SetParameter(0, m_taud);
  CellM->SetParameter(1, m_taupa);
  CellM->SetParameter(2, m_td);
  CellM->SetParameter(3, m_Rf);
  CellM->SetParameter(4, m_C1);

  double cell = 0 ;

  if (type){
      cell = CellM->Derivative(x) ;        
  }
  else {
      cell = CellM->Eval(x) ;
  }

  delete CellM ;

  return cell ;
}// end of function





