__all__ = ["RootStreamAODReader"]

from GaugiKernel import Cpp
from GaugiKernel.macros import *
import ROOT


class RootStreamAODReader(Cpp):

    def __init__(
        self,
        name,
        InputFile: str,
        OutputEventKey: str,
        OutputTruthKey: str,
        OutputClusterKey: str,
        OutputRingerKeys: list,  # Alterado para aceitar uma lista de strings
        OutputRingerL0Key: str,
        OutputSeedsKey: str,
        OutputElectronKey: str,
        OutputTruthClusterKey: str,
        OutputTruthRingerKeys: list,  # Alterado para aceitar uma lista de strings
        OutputTruthElectronKey: str,
        OutputXTCellsKey: str,  # acrescentei para entregar as XTCells lidas do AOD
        OutputXTClusterKey: str,  # acrescentei para entregar os XTClusters
        OutputXTRingerKeys: list,  # acrescentei a lista dos aneis XT
        OutputXTRingerL0Key: str,  # acrescentei para entregar os XTRingsL0
        OutputXTElectronKey: str,  # acrescentei para entregar os XTElectrons
        DumpCrossTalkCells: bool = False,  # indica se este AOD deve conter os dados XT
        OutputLevel: int = 0,
        NtupleName: str = "CollectionTree",
    ):

        Cpp.__init__(self, ROOT.RootStreamAODReader(name))

        # Propriedades padrão
        self.setProperty("OutputEventKey", OutputEventKey)
        self.setProperty("OutputTruthKey", OutputTruthKey)
        self.setProperty("OutputClusterKey", OutputClusterKey)
        self.setProperty("OutputRingerL0Key", OutputRingerL0Key)
        self.setProperty("OutputSeedsKey", OutputSeedsKey)
        self.setProperty("OutputElectronKey", OutputElectronKey)
        self.setProperty("OutputTruthClusterKey", OutputTruthClusterKey)
        self.setProperty("OutputTruthElectronKey", OutputTruthElectronKey)
        self.setProperty("OutputXTCellsKey", OutputXTCellsKey)  # passei a chave das XTCells ao C++
        self.setProperty("OutputXTClusterKey", OutputXTClusterKey)  # passei a chave dos XTClusters ao C++
        self.setProperty("OutputXTRingerKeys", OutputXTRingerKeys)  # passei as chaves dos aneis XT ao C++
        self.setProperty("OutputXTRingerL0Key", OutputXTRingerL0Key)  # passei a chave dos XTRingsL0 ao C++
        self.setProperty("OutputXTElectronKey", OutputXTElectronKey)  # passei a chave dos XTElectrons ao C++
        self.setProperty("DumpCrossTalkCells", DumpCrossTalkCells)  # passa ao C++ a decisao de ler os ramos XT

        # Novas propriedades de listas (Múltiplas coleções)
        self.setProperty("OutputRingerKeys", OutputRingerKeys)
        self.setProperty("OutputTruthRingerKeys", OutputTruthRingerKeys)

        # Configurações gerais
        self.setProperty("OutputLevel", OutputLevel)
        self.setProperty("NtupleName", NtupleName)
        self.setProperty("InputFile", InputFile)

        f = ROOT.TFile(self.InputFile, "read")
        t = f.Get(self.NtupleName)
        self.__entries = t.GetEntries()

    def GetEntries(self):
        return self.__entries

    def merge(self, acc):
        acc.SetReader(self)
