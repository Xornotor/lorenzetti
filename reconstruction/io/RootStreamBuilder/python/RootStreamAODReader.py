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
        OutputXTCellsKey: str,
        OutputXTClusterKey: str,
        OutputXTRingerKeys: list,
        OutputXTRingerL0Key: str,
        OutputXTElectronKey: str,
        DumpCrossTalkCells: bool = False,
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
        self.setProperty("DumpCrossTalkCells", DumpCrossTalkCells)

        # Novas propriedades de listas (Múltiplas coleções)
        self.setProperty("OutputRingerKeys", OutputRingerKeys)
        self.setProperty("OutputTruthRingerKeys", OutputTruthRingerKeys)

        # Configurações gerais
        self.setProperty("OutputLevel", OutputLevel)
        self.setProperty("NtupleName", NtupleName)
        self.setProperty("InputFile", InputFile)

        if(DumpCrossTalkCells):
            self.setProperty("OutputXTCellsKey", OutputXTCellsKey)
            self.setProperty("OutputXTClusterKey", OutputXTClusterKey)
            self.setProperty("OutputXTRingerKeys", OutputXTRingerKeys)
            self.setProperty("OutputXTRingerL0Key", OutputXTRingerL0Key)
            self.setProperty("OutputXTElectronKey", OutputXTElectronKey)

        f = ROOT.TFile(self.InputFile, "read")
        t = f.Get(self.NtupleName)
        self.__entries = t.GetEntries()

    def GetEntries(self):
        return self.__entries

    def merge(self, acc):
        acc.SetReader(self)
