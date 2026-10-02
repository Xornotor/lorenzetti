
#include "CaloCell/CaloCellContainer.h"
#include "CaloCell/CaloDetDescriptorCollection.h"

#include "CaloCellMerge.h"
#include "TVector3.h"
#include <cstdlib>

#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH2Poly.h"


using namespace Gaugi;
using namespace SG;


/**
 * @class CaloCellMerge
 * @brief Algorithm to merge multiple cell collections into a single container.
 * 
 * This algorithm gathers partial cell collections (e.g. from different samplings
 * like EMB1, EMB2, Tile1, etc.) produced by separate CaloCellMaker instances
 * and merges them into a single global CaloCellContainer. It handles both
 * reconstructed cells and truth cells.
 * 
 * Properties:
 * - InputCollectionKeys: List of keys for the partial collections.
 * - OutputCellsKey: Key for the final merged reconstructed cell container.
 * - OutputTruthCellsKey: Key for the final merged truth cell container.
 */
CaloCellMerge::CaloCellMerge( std::string name ) : 
  IMsgService(name),
  Algorithm()
{
  declareProperty( "InputCollectionKeys"    , m_collectionKeys={}           );
  declareProperty( "InputXTCollectionKeys"  , m_xtCollectionKeys={}         );
  declareProperty( "OutputCellsKey"         , m_cellsKey="Cells"            );
  declareProperty( "OutputXTCellsKey"       , m_xtcellsKey="XTCells"        );
  declareProperty( "OutputTruthCellsKey"    , m_truthCellsKey="TruthCells"  );
  declareProperty( "DumpCrossTalkCells"     , m_dumpXTCells=false           );  
  declareProperty( "OutputLevel"            , m_outputLevel=1               );
}

//!=====================================================================

CaloCellMerge::~CaloCellMerge()
{;}

//!=====================================================================

StatusCode CaloCellMerge::initialize()
{
  CHECK_INIT();
  setMsgLevel( m_outputLevel );
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::finalize()
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::bookHistograms( SG::EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::pre_execute( EventContext &/*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::execute( EventContext &/*ctx*/ , const G4Step * /*step*/ ) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::execute( EventContext &ctx , int /*evt*/ ) const
{
  return post_execute(ctx);
}

//!=====================================================================

/**
 * @brief Core merge logic.
 * 
 * Iterates through all input keys, retrieves the corresponding `CaloDetDescriptorCollection`,
 * creates new `xAOD::CaloCell` objects (copying info from descriptors), and pushes them
 * into the output containers.
 */
StatusCode CaloCellMerge::post_execute( EventContext &ctx ) const
{

  MSG_DEBUG( "Starting collection merge algorithm..." );

  MSG_DEBUG( "Creating reco cells containers with key " << m_cellsKey);
  SG::WriteHandle<xAOD::CaloCellContainer> recoContainer( m_cellsKey , ctx );
  recoContainer.record( std::unique_ptr<xAOD::CaloCellContainer>(new xAOD::CaloCellContainer()) );
  
  MSG_DEBUG( "Creating truth cells containers with key " << m_truthCellsKey);
  SG::WriteHandle<xAOD::CaloCellContainer> truthContainer( m_truthCellsKey , ctx );
  truthContainer.record( std::unique_ptr<xAOD::CaloCellContainer>(new xAOD::CaloCellContainer()) );

  //unsigned long int hash = 0;

  for ( auto key : m_collectionKeys ){

    MSG_DEBUG( "Reading all cells from collection with key " << key );
    SG::ReadHandle<xAOD::CaloDetDescriptorCollection> collection( key, ctx );
    MSG_DEBUG("Collection.size: "<< collection->operator*().size());
    
    if( !collection.isValid() ){
      MSG_WARNING( "It's not possible to read the xAOD::CaloCellCollection from this Context using this key: " << key );
      continue;
    }

    MSG_DEBUG( "Creating new cells and attach the object into the container" ); 
    for (const auto &pair : **collection.ptr() )
    {
      // descriptor Cell with all geant/bunch/pulse information
      const xAOD::CaloDetDescriptor* descriptor=pair.second;
     
      // Create the truth cell 
      auto truth_cell = new xAOD::CaloCell();

      truth_cell->setEta( descriptor->eta() );
      truth_cell->setPhi( descriptor->phi() );
      truth_cell->setDeltaEta( descriptor->deltaEta() );
      truth_cell->setDeltaPhi( descriptor->deltaPhi() );
      truth_cell->setE( descriptor->edep() ); // The truth will be the energy deposity
      truth_cell->setEt( truth_cell->e() / std::cosh( truth_cell->eta() ) );
      truth_cell->setTau( -1 );
      
      truth_cell->setDescriptor( descriptor );
      truthContainer->push_back( truth_cell );

      // Create the Reco cell
      auto cell = new xAOD::CaloCell();
      cell->setEta( descriptor->eta() );
      cell->setPhi( descriptor->phi() );
      cell->setDeltaEta( descriptor->deltaEta() );
      cell->setDeltaPhi( descriptor->deltaPhi() );
      cell->setE( descriptor->e() ); // Estimated energy from OF
      cell->setEt( cell->e() / std::cosh( cell->eta() ) );
      cell->setTau( descriptor->tau());

      cell->setDescriptor( descriptor );
      recoContainer->push_back( cell );
    }// Loop over all descriptorCells
  }// Loop over all collections


  if(m_dumpXTCells){
    MSG_DEBUG( "Creating crosstalk-contaminated reco cells containers with key " << m_xtcellsKey );
    SG::WriteHandle<xAOD::CaloCellContainer> xtRecoContainer( m_xtcellsKey, ctx );
    xtRecoContainer.record( std::unique_ptr<xAOD::CaloCellContainer>(new xAOD::CaloCellContainer()) );

    for ( auto key : m_xtCollectionKeys ){
      MSG_DEBUG( "Reading all cells from collection with key " << key );
      SG::ReadHandle<xAOD::CaloDetDescriptorCollection> collection( key, ctx );
      MSG_DEBUG( "xtCollection.size: "<< collection->operator*().size() );

      if( !collection.isValid() ){
        MSG_WARNING( "It's not possible to read the xAOD::CaloCellCollection from this Context using this key: " << key );
        continue;
      }

      MSG_DEBUG( "Creating new cells and attach the object into the container" ); 
      for (const auto &pair : **collection.ptr() )
      {
        // descriptor Cell with all geant/bunch/pulse information
        const xAOD::CaloDetDescriptor* descriptor=pair.second;
        // Create the Reco cell
        auto cell = new xAOD::CaloCell();
        cell->setEta( descriptor->eta() );
        cell->setPhi( descriptor->phi() );
        cell->setDeltaEta( descriptor->deltaEta() );
        cell->setDeltaPhi( descriptor->deltaPhi() );
        cell->setE( descriptor->e() ); // Estimated energy from OF
        cell->setEt( cell->e() / std::cosh( cell->eta() ) );
        cell->setTau( descriptor->tau());

        cell->setDescriptor( descriptor );
        xtRecoContainer->push_back( cell );
      }// Loop over all descriptorCells
    }// Loop over all collections
  }

  MSG_DEBUG( "All collections were merged into two CaloCellContainer" );
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode CaloCellMerge::fillHistograms( EventContext & /*ctx*/ ) const
{
  return StatusCode::SUCCESS;
}



