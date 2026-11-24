#include "CaloCell/CaloCellContainer.h"
#include "EventInfo/EventInfoContainer.h"
#include "TruthParticle/TruthParticleContainer.h"
#include "CaloCluster/CaloClusterContainer.h"
#include "CaloRings/CaloRingsContainer.h"
#include "Egamma/ElectronContainer.h"

#include "CaloCell/CaloCellConverter.h"
#include "CaloCell/CaloDetDescriptorConverter.h"
#include "CaloCell/CaloDetDescriptorCollection.h"
#include "CaloCell/CaloDetDescriptorContainer.h"
#include "EventInfo/EventInfoConverter.h"
#include "TruthParticle/TruthParticleConverter.h"
#include "EventInfo/SeedConverter.h"
#include "CaloCluster/CaloClusterConverter.h"
#include "CaloRings/CaloRingsConverter.h"
#include "Egamma/ElectronConverter.h"
#include "RootStreamAODReader.h"
#include "GaugiKernel/EDM.h"

using namespace SG;
using namespace Gaugi;

/**
 * @class RootStreamAODReader
 * @brief Reads AOD data from a ROOT file and reconstructs xAOD objects.
 *
 * This algorithm is the inverse of `RootStreamAODMaker`. It opens a ROOT file,
 * reads the persistent structs from the TTree, and converts them back into
 * transient `xAOD` objects (EventInfo, CaloClusters, Electrons, etc.), which
 * are then recorded into StoreGate for downstream algorithms.
 *
 * Properties:
 * - InputFile: Path to the ROOT file.
 * - Output*Key: Keys to record objects in StoreGate.
 */
RootStreamAODReader::RootStreamAODReader(std::string name) : IMsgService(name),
                                                             Algorithm()
{

  declareProperty("OutputEventKey", m_eventKey = "EventInfo");
  declareProperty("OutputSeedsKey", m_seedsKey = "Seeds");
  declareProperty("OutputTruthKey", m_truthKey = "Particles");
  declareProperty("OutputClusterKey", m_clusterKey = "Clusters");
  declareProperty("OutputRingerKey", m_ringerKey = "Rings");
  declareProperty("OutputRingerL0Key", m_ringerL0Key = "RingsL0");
  declareProperty("OutputElectronKey", m_electronKey = "Electrons");
  declareProperty("OutputTruthClusterKey", m_truthClusterKey = "TruthClusters");
  // declareProperty("OutputTruthRingerKey", m_truthRingerKey = "TruthRings");
  declareProperty("OutputTruthElectronKey", m_truthElectronKey = "TruthElectrons");
  declareProperty("OutputXTCellsKey", m_xtCellsKey = "XTCells");           // acrescentei a chave das celulas XT
  declareProperty("OutputXTClusterKey", m_xtClusterKey = "XTClusters");    // acrescentei a chave dos clusters XT
  declareProperty("OutputXTRingerL0Key", m_xtRingerL0Key = "XTRingsL0");   // acrescentei a chave dos aneis L0 XT
  declareProperty("OutputXTElectronKey", m_xtElectronKey = "XTElectrons"); // acrescentei a chave dos eletrons XT  

  declareProperty("DumpCrossTalkCells", m_doCrosstalk = false); // recebe do Python a opcao de ler os dados XT


  declareProperty("OutputLevel", m_outputLevel = 1);
  declareProperty("NtupleName", m_ntupleName = "CollectionTree");
  declareProperty("InputFile", m_inputFile = "");

  declareProperty("OutputRingerKeys", m_ringerKeys = {"Rings"});
  declareProperty("OutputTruthRingerKeys", m_truthRingerKeys = {"TruthRings"});
  declareProperty("OutputXTRingerKeys", m_xtRingerKeys = {"XTRings"}); // acrescentei a lista de aneis XT
}

//!=====================================================================

RootStreamAODReader::~RootStreamAODReader()
{
}

//!=====================================================================

StatusCode RootStreamAODReader::initialize()
{
  CHECK_INIT();
  setMsgLevel(m_outputLevel);
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::finalize()
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::bookHistograms(EventContext &ctx) const
{
  auto store = ctx.getStoreGateSvc();
  TFile *file = new TFile(m_inputFile.c_str(), "read");
  store->decorate("events", file);
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::pre_execute(EventContext & /*ctx*/) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::execute(EventContext & /*ctx*/, const G4Step * /*step*/) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::execute(EventContext &ctx, int evt) const
{
  return deserialize(evt, ctx);
}

//!=====================================================================

StatusCode RootStreamAODReader::post_execute(EventContext & /*ctx*/) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

StatusCode RootStreamAODReader::fillHistograms(EventContext &ctx) const
{
  return StatusCode::SUCCESS;
}

//!=====================================================================

/**
 * @brief Performs the deserialization.
 *
 * 1. Reads the entry for the current event from the TTree.
 * 2. deserializes structs into xAOD objects using `xAOD::*Converter`.
 * 3. Re-establishes links (pointers) between objects (e.g. Cluster -> Seed, Rings -> Cluster).
 * 4. Records the containers into StoreGate.
 */
StatusCode RootStreamAODReader::deserialize(int evt, EventContext &ctx) const
{
  std::vector<xAOD::CaloDetDescriptor_t> *collection_descriptor = nullptr;
  std::vector<xAOD::CaloCell_t> *collection_cells = nullptr;
  std::vector<xAOD::EventInfo_t> *collection_event = nullptr;
  std::vector<xAOD::Seed_t> *collection_seeds = nullptr;
  std::vector<xAOD::TruthParticle_t> *collection_truth = nullptr;
  // std::vector<xAOD::CaloRings_t> *collection_rings = nullptr;
  std::vector<xAOD::CaloCluster_t> *collection_clus = nullptr;
  std::vector<xAOD::Electron_t> *collection_el = nullptr;
  std::vector<xAOD::CaloCluster_t> *collection_truth_clus = nullptr;
  // std::vector<xAOD::CaloRings_t> *collection_truth_rings = nullptr;
  std::vector<xAOD::CaloRings_t> *collection_rings_l0 = nullptr;
  std::vector<xAOD::Electron_t> *collection_truth_el = nullptr;

  std::vector<xAOD::CaloDetDescriptor_t> *collection_xtdescriptor = nullptr; // acrescentei os descritores das XTCells
  std::vector<xAOD::CaloCell_t> *collection_xtcells = nullptr;               // acrescentei as celulas XT
  std::vector<xAOD::CaloCluster_t> *collection_xt_clus = nullptr;            // acrescentei os clusters XT
  std::vector<xAOD::CaloRings_t> *collection_xt_rings_l0 = nullptr;          // acrescentei os aneis L0 XT
  std::vector<xAOD::Electron_t> *collection_xt_el = nullptr;                 // acrescentei os eletrons XT

  MSG_DEBUG("Link all branches...");

  auto store = ctx.getStoreGateSvc();
  TFile *file = (TFile *)store->decorator("events");
  TTree *tree = (TTree *)file->Get(m_ntupleName.c_str());

  if (m_doCrosstalk)
  {
    std::vector<std::string> required_xt_branches = {
        "CaloCellContainer_" + m_xtCellsKey,
        "CaloDetDescriptorContainer_" + m_xtCellsKey,
        "CaloClusterContainer_" + m_xtClusterKey,
        "CaloRingsContainer_" + m_xtRingerL0Key,
        "ElectronContainer_" + m_xtElectronKey};

    for (const auto &key : m_xtRingerKeys)
      required_xt_branches.push_back("CaloRingsContainer_" + key);

    for (const auto &branch : required_xt_branches)
    {
      const char *alias = tree->GetAlias(branch.c_str());
      const std::string branch_name = alias ? alias : branch;
      if (!tree->FindBranch(branch_name.c_str()))
      {
        MSG_ERROR("Missing required XT branch " << branch);
        return StatusCode::FAILURE;
      }
    }
  }

  InitBranch(tree, ("EventInfoContainer_" + m_eventKey).c_str(), &collection_event);
  InitBranch(tree, ("SeedContainer_" + m_seedsKey).c_str(), &collection_seeds);
  InitBranch(tree, ("TruthParticleContainer_" + m_truthKey).c_str(), &collection_truth);
  // InitBranch(tree, ("CaloRingsContainer_" + m_ringerKey).c_str(), &collection_rings);
  InitBranch(tree, ("CaloClusterContainer_" + m_clusterKey).c_str(), &collection_clus);
  InitBranch(tree, ("ElectronContainer_" + m_electronKey).c_str(), &collection_el);
  InitBranch(tree, ("CaloClusterContainer_" + m_truthClusterKey).c_str(), &collection_truth_clus);
  // InitBranch(tree, ("CaloRingsContainer_" + m_truthRingerKey).c_str(), &collection_truth_rings);
  InitBranch(tree, ("CaloRingsContainer_" + m_ringerL0Key).c_str(), &collection_rings_l0);
  InitBranch(tree, ("ElectronContainer_" + m_truthElectronKey).c_str(), &collection_truth_el);

  if (m_doCrosstalk) // so liga os ramos XT quando a leitura de crosstalk estiver ativa
  {
    InitBranch(tree, ("CaloCellContainer_" + m_xtCellsKey).c_str(), &collection_xtcells);
    InitBranch(tree, ("CaloDetDescriptorContainer_" + m_xtCellsKey).c_str(), &collection_xtdescriptor);
    InitBranch(tree, ("CaloClusterContainer_" + m_xtClusterKey).c_str(), &collection_xt_clus);
    InitBranch(tree, ("CaloRingsContainer_" + m_xtRingerL0Key).c_str(), &collection_xt_rings_l0);
    InitBranch(tree, ("ElectronContainer_" + m_xtElectronKey).c_str(), &collection_xt_el);
  }

  std::vector<std::vector<xAOD::CaloRings_t> *> collections_rings(m_ringerKeys.size(), nullptr);
  std::vector<std::vector<xAOD::CaloRings_t> *> collections_truth_rings(m_truthRingerKeys.size(), nullptr);
  std::vector<std::vector<xAOD::CaloRings_t> *> collections_xt_rings(m_xtRingerKeys.size(), nullptr); // acrescentei a lista das topologias XT

  for (size_t i = 0; i < m_ringerKeys.size(); ++i)
    InitBranch(tree, ("CaloRingsContainer_" + m_ringerKeys[i]).c_str(), &collections_rings[i]);
  for (size_t i = 0; i < m_truthRingerKeys.size(); ++i)
    InitBranch(tree, ("CaloRingsContainer_" + m_truthRingerKeys[i]).c_str(), &collections_truth_rings[i]);
  if (m_doCrosstalk) // so liga os ramos das topologias XT quando o crosstalk estiver ativo
  {
    for (size_t i = 0; i < m_xtRingerKeys.size(); ++i)
      InitBranch(tree, ("CaloRingsContainer_" + m_xtRingerKeys[i]).c_str(), &collections_xt_rings[i]);
  }

  tree->GetEntry(evt);

  MSG_DEBUG("Deserialize TruthParticle...");

  { // deserialize EventInfo
    SG::WriteHandle<xAOD::TruthParticleContainer> container(m_truthKey, ctx);
    container.record(std::unique_ptr<xAOD::TruthParticleContainer>(new xAOD::TruthParticleContainer()));

    xAOD::TruthParticleConverter cnv;
    for (auto &par_t : *collection_truth)
    {
      xAOD::TruthParticle *par = nullptr;
      cnv.convert(par_t, par);
      MSG_DEBUG("Particle in eta = " << par->eta() << ", phi = " << par->phi());
      container->push_back(par);
    }
  }

  MSG_DEBUG("Deserialize EventInfo...");

  { // deserialize EventInfo

    SG::WriteHandle<xAOD::EventInfoContainer> container(m_eventKey, ctx);
    container.record(std::unique_ptr<xAOD::EventInfoContainer>(new xAOD::EventInfoContainer()));
    xAOD::EventInfo *event = nullptr;
    xAOD::EventInfoConverter cnv;
    cnv.convert(collection_event->at(0), event);
    MSG_DEBUG("EventNumber = " << event->eventNumber() << ", Avgmu = " << event->avgmu());
    container->push_back(event);
  }

  MSG_DEBUG("Deserialize Seed...");
  xAOD::seed_links_t seed_links;

  { // deserialize Seed
    SG::WriteHandle<xAOD::SeedContainer> container(m_seedsKey, ctx);
    container.record(std::unique_ptr<xAOD::SeedContainer>(new xAOD::SeedContainer()));

    xAOD::SeedConverter cnv;
    for (auto &seed_t : *collection_seeds)
    {
      xAOD::Seed *seed = nullptr;
      cnv.convert(seed_t, seed);
      MSG_DEBUG("Seed in eta = " << seed->eta() << ", phi = " << seed->phi());
      container->push_back(seed);
      seed_links[seed->id()] = seed;
    }
  }

  MSG_DEBUG("Deserialize Clusters, Rings and Electrons...");

  { // deserialize cluster and rings
    SG::WriteHandle<xAOD::CaloClusterContainer> container_clus(m_clusterKey, ctx);
    container_clus.record(std::unique_ptr<xAOD::CaloClusterContainer>(new xAOD::CaloClusterContainer()));

    SG::WriteHandle<xAOD::CaloRingsContainer> container_rings(m_ringerKey, ctx);
    container_rings.record(std::unique_ptr<xAOD::CaloRingsContainer>(new xAOD::CaloRingsContainer()));

    SG::WriteHandle<xAOD::ElectronContainer> container_el(m_electronKey, ctx);
    container_el.record(std::unique_ptr<xAOD::ElectronContainer>(new xAOD::ElectronContainer()));

    SG::WriteHandle<xAOD::CaloCellContainer> container_truth_cells(m_truthCellsKey, ctx);
    container_truth_cells.record(std::unique_ptr<xAOD::CaloCellContainer>(new xAOD::CaloCellContainer()));

    SG::WriteHandle<xAOD::CaloClusterContainer> container_truth_clus(m_truthClusterKey, ctx);
    container_truth_clus.record(std::unique_ptr<xAOD::CaloClusterContainer>(new xAOD::CaloClusterContainer()));

    SG::WriteHandle<xAOD::CaloRingsContainer> container_truth_rings(m_truthRingerKey, ctx);
    container_truth_rings.record(std::unique_ptr<xAOD::CaloRingsContainer>(new xAOD::CaloRingsContainer()));

    SG::WriteHandle<xAOD::CaloRingsContainer> container_rings_l0(m_ringerL0Key, ctx);
    container_rings_l0.record(std::unique_ptr<xAOD::CaloRingsContainer>(new xAOD::CaloRingsContainer()));

    SG::WriteHandle<xAOD::ElectronContainer> container_truth_el(m_truthElectronKey, ctx);
    container_truth_el.record(std::unique_ptr<xAOD::ElectronContainer>(new xAOD::ElectronContainer()));

    xAOD::CaloClusterConverter clus_cnv;
    // xAOD::CaloRingsConverter rings_cnv;
    xAOD::ElectronConverter el_cnv;

    xAOD::CaloCellConverter truth_cells_cnv;
    xAOD::CaloClusterConverter truth_clus_cnv;
    xAOD::CaloRingsConverter truth_rings_cnv;

    xAOD::cluster_links_t clus_links;
    xAOD::CaloRingsConverter rings_cnv;

    for (auto &clus_t : *collection_clus)
    {
      xAOD::CaloCluster *clus = nullptr;
      clus_cnv.convert(clus_t, clus);
      clus->setSeed(seed_links[clus_t.seed_link]);
      container_clus->push_back(clus);
      clus_links[clus->seed()->id()] = clus;
    }

    // for (auto &rings_t : *collection_rings)
    // {
    //   xAOD::CaloRings *rings = nullptr;
    //   rings_cnv.convert(rings_t, rings);
    //   rings->setCaloCluster(clus_links[rings_t.cluster_link]);
    //   container_rings->push_back(rings);
    // }

    for (auto &el_t : *collection_el)
    {
      xAOD::Electron *el = nullptr;
      el_cnv.convert(el_t, el);
      el->setCaloCluster(clus_links[el_t.cluster_link]);
      container_el->push_back(el);
    }

    for (auto &rings_t : *collection_rings_l0)
    {
      xAOD::CaloRings *rings = nullptr;
      rings_cnv.convert(rings_t, rings);
      if (clus_links.count(rings_t.cluster_link))
      {
        rings->setCaloCluster(clus_links[rings_t.cluster_link]);
      }
      container_rings_l0->push_back(rings);
    }

    xAOD::cluster_links_t truth_clus_links;

    for (auto &truth_clus_t : *collection_truth_clus)
    {
      xAOD::CaloCluster *truth_clus = nullptr;
      truth_clus_cnv.convert(truth_clus_t, truth_clus);

      if (seed_links.count(truth_clus_t.seed_link))
      {
        truth_clus->setSeed(seed_links[truth_clus_t.seed_link]);
        truth_clus_links[truth_clus->seed()->id()] = truth_clus;
      }

      container_truth_clus->push_back(truth_clus);
    }

    // for (auto &truth_rings_t : *collection_truth_rings)
    // {
    //   xAOD::CaloRings *truth_rings = nullptr;
    //   truth_rings_cnv.convert(truth_rings_t, truth_rings);

    //   if (truth_clus_links.count(truth_rings_t.cluster_link))
    //   {
    //     truth_rings->setCaloCluster(truth_clus_links[truth_rings_t.cluster_link]);
    //   }
    //   container_truth_rings->push_back(truth_rings);
    // }

    for (auto &el_t : *collection_truth_el)
    {
      xAOD::Electron *el = nullptr;
      el_cnv.convert(el_t, el);
      if (truth_clus_links.count(el_t.cluster_link))
      {
        el->setCaloCluster(truth_clus_links[el_t.cluster_link]);
      }
      container_truth_el->push_back(el);
    }

    // Parte Xt
    xAOD::cluster_links_t xt_clus_links; // acrescentei os vinculos dos XTClusters

    if (m_doCrosstalk) // so reconstroi os objetos XT quando a leitura de crosstalk estiver ativa
    {
      SG::WriteHandle<xAOD::CaloClusterContainer> container_xt_clus(m_xtClusterKey, ctx); // acrescentei o container dos XTClusters
      container_xt_clus.record(std::make_unique<xAOD::CaloClusterContainer>());           // criei a colecao XT vazia em memoria

      SG::WriteHandle<xAOD::ElectronContainer> container_xt_el(m_xtElectronKey, ctx); // acrescentei o container dos XTElectrons
      container_xt_el.record(std::make_unique<xAOD::ElectronContainer>());            // criei a colecao de eletrons XT vazia em memoria

      SG::WriteHandle<xAOD::CaloRingsContainer> container_xt_rings_l0(m_xtRingerL0Key, ctx); // acrescentei o container dos XTRingsL0
      container_xt_rings_l0.record(std::make_unique<xAOD::CaloRingsContainer>());            // criei a colecao de aneis L0 XT vazia em memoria

      if (collection_xtcells && collection_xtdescriptor) // le as celulas e os descritores XT gravados no AOD
      {
        std::map<unsigned long int, xAOD::CaloDetDescriptor *> xtdescriptor_links;
        SG::WriteHandle<xAOD::CaloDetDescriptorContainer> container_xt_descriptor(m_xtCellsKey + "_Aux", ctx);
        container_xt_descriptor.record(std::make_unique<xAOD::CaloDetDescriptorContainer>());

        for (auto &descriptor_t : *collection_xtdescriptor)
        {
          xAOD::CaloDetDescriptor *descriptor = nullptr;
          xAOD::CaloDetDescriptorConverter cnv;
          cnv.convert(descriptor_t, descriptor);
          xtdescriptor_links[descriptor->hash()] = descriptor; // associa o descritor ao seu hash
          container_xt_descriptor->push_back(descriptor);
        }

        SG::WriteHandle<xAOD::CaloCellContainer> container_xt_cells(m_xtCellsKey, ctx);
        container_xt_cells.record(std::make_unique<xAOD::CaloCellContainer>());

        for (auto &cell_t : *collection_xtcells)
        {
          xAOD::CaloCell *cell = nullptr;
          xAOD::CaloCellConverter cnv;
          cnv.convert(cell_t, cell);
          auto descriptor = xtdescriptor_links.find(cell_t.descriptor_link); // o conversor grava o hash em descriptor_link
          if (descriptor == xtdescriptor_links.end())
          {
            MSG_ERROR("Missing XT descriptor for cell hash " << cell_t.descriptor_link);
            delete cell;
            return StatusCode::FAILURE;
          }
          cell->setDescriptor(descriptor->second);
          container_xt_cells->push_back(cell);
        }
      }

      if (collection_xt_clus) // so percorre os clusters XT se o ramo foi carregado do AOD
      {
        for (auto &xt_clus_t : *collection_xt_clus)
        {
          xAOD::CaloCluster *xt_clus = nullptr;
          clus_cnv.convert(xt_clus_t, xt_clus);

          if (seed_links.count(xt_clus_t.seed_link)) // confere se a Seed desse cluster existe em memoria
          {
            xt_clus->setSeed(seed_links[xt_clus_t.seed_link]);
            xt_clus_links[xt_clus->seed()->id()] = xt_clus;
          }

          container_xt_clus->push_back(xt_clus);
        }
      }

      if (collection_xt_el) // so percorre os eletrons XT se o ramo foi carregado do AOD
      {
        for (auto &el_t : *collection_xt_el)
        {
          xAOD::Electron *el = nullptr;
          el_cnv.convert(el_t, el);

          if (xt_clus_links.count(el_t.cluster_link)) // confere se o XTCluster ligado a esse eletron existe
          {
            el->setCaloCluster(xt_clus_links[el_t.cluster_link]);
          }

          container_xt_el->push_back(el);
        }
      }

      if (collection_xt_rings_l0) // so percorre os aneis L0 XT se o ramo foi carregado do AOD
      {
        for (auto &rings_t : *collection_xt_rings_l0)
        {
          xAOD::CaloRings *rings = nullptr;
          rings_cnv.convert(rings_t, rings);

          if (xt_clus_links.count(rings_t.cluster_link)) // confere se o XTCluster ligado a esses aneis existe
          {
            rings->setCaloCluster(xt_clus_links[rings_t.cluster_link]);
          }

          container_xt_rings_l0->push_back(rings);
        }
      }
    }

    // inside the deserialization block, replace the single rings WriteHandle with a loop:

    for (size_t i = 0; i < m_ringerKeys.size(); ++i)
    {
      SG::WriteHandle<xAOD::CaloRingsContainer> container_rings(m_ringerKeys[i], ctx);
      container_rings.record(std::make_unique<xAOD::CaloRingsContainer>());
      if (collections_rings[i])
      {
        for (auto &rings_t : *collections_rings[i])
        {
          xAOD::CaloRings *rings = nullptr;
          rings_cnv.convert(rings_t, rings);
          rings->setCaloCluster(clus_links[rings_t.cluster_link]);
          container_rings->push_back(rings);
        }
      }
    }

    for (size_t i = 0; i < m_truthRingerKeys.size(); ++i)
    {
      SG::WriteHandle<xAOD::CaloRingsContainer> container_truth_rings(m_truthRingerKeys[i], ctx);
      container_truth_rings.record(std::make_unique<xAOD::CaloRingsContainer>());
      if (collections_truth_rings[i])
      {
        for (auto &rings_t : *collections_truth_rings[i])
        {
          xAOD::CaloRings *rings = nullptr;
          rings_cnv.convert(rings_t, rings);
          if (truth_clus_links.count(rings_t.cluster_link))
            rings->setCaloCluster(truth_clus_links[rings_t.cluster_link]);
          container_truth_rings->push_back(rings);
        }
      }
    }

    if (m_doCrosstalk) // so reconstroi os aneis XT quando a leitura de crosstalk estiver ativa
    {
      for (size_t i = 0; i < m_xtRingerKeys.size(); ++i) // acrescentei o caminho dos aneis XT
      {
        SG::WriteHandle<xAOD::CaloRingsContainer> container_xt_rings(m_xtRingerKeys[i], ctx);
        container_xt_rings.record(std::make_unique<xAOD::CaloRingsContainer>());

        if (collections_xt_rings[i]) // so percorre os aneis XT se esse ramo foi carregado do AOD
        {
          for (auto &rings_t : *collections_xt_rings[i])
          {
            xAOD::CaloRings *rings = nullptr;
            rings_cnv.convert(rings_t, rings);

            if (xt_clus_links.count(rings_t.cluster_link)) // confere se o XTCluster ligado a esses aneis existe
              rings->setCaloCluster(xt_clus_links[rings_t.cluster_link]);

            container_xt_rings->push_back(rings);
          }
        }
      }
    }

    // cleanup
    for (auto *c : collections_rings)
      delete c;
    for (auto *c : collections_truth_rings)
      delete c;
    for (auto *c : collections_xt_rings) // acrescentei a limpeza dos aneis XT lidos do AOD
      delete c;
  }

  delete collection_descriptor;
  delete collection_seeds;
  delete collection_event;
  delete collection_truth;
  // delete collection_rings;
  delete collection_clus;
  delete collection_el;
  delete collection_truth_clus;
  // delete collection_truth_rings;
  delete collection_rings_l0;
  delete collection_truth_el;
  // delete collection_xt_rings;
  delete collection_xtdescriptor; // acrescentei a limpeza dos descritores XT
  delete collection_xtcells;      // acrescentei a limpeza das celulas XT
  delete collection_xt_clus;      // acrescentei a limpeza dos clusters XT
  delete collection_xt_rings_l0;  // acrescentei a limpeza dos aneis L0 XT
  delete collection_xt_el;        // acrescentei a limpeza dos eletrons XT

  return StatusCode::SUCCESS;
}

//!=====================================================================

template <class T>
void RootStreamAODReader::InitBranch(TTree *fChain, std::string branch_name, T *param) const
{
  std::string bname = branch_name;
  if (fChain->GetAlias(bname.c_str()))
    bname = std::string(fChain->GetAlias(bname.c_str()));

  if (!fChain->FindBranch(bname.c_str()))
  {
    MSG_WARNING("unknown branch " << bname);
    return;
  }
  fChain->SetBranchStatus(bname.c_str(), 1.);
  fChain->SetBranchAddress(bname.c_str(), param);
}
