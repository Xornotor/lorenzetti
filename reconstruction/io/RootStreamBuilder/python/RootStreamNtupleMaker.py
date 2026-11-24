__all__ = ["RootStreamNtupleMaker"]

from GaugiKernel import Cpp
from GaugiKernel.macros import *
import ROOT


class RootStreamNtupleMaker(Cpp):

    def __init__(
        self,
        name,
        InputEventKey: str,
        InputTruthKey: str,
        InputSeedsKey: str,
        InputClusterKey: str,
        InputRingerKeys: list,
        InputRingerL0Key: str,
        InputTruthClusterKey: str,
        InputTruthRingerKeys: list,
        InputTruthElectronKey: str,
        InputElectronKey: str,
        InputXTClusterKey: str = "XTClusters",  # chave dos clusters XT
        InputXTRingerKeys: list = None,  # chaves das topologias XT
        InputXTRingerL0Key: str = "XTRingsL0",  # chave dos aneis L0 XT
        InputXTElectronKey: str = "XTElectrons",  # chave dos eletrons XT
        DumpCrossTalkCells: bool = False,  # controla a escrita das colunas XT
        OutputLevel: int = 0,
        OutputNtupleName: str = "events",
    ):

        Cpp.__init__(self, ROOT.RootStreamNtupleMaker(name))
        self.setProperty("InputEventKey", InputEventKey)
        self.setProperty("InputTruthKey", InputTruthKey)
        self.setProperty("InputSeedsKey", InputSeedsKey)
        self.setProperty("InputClusterKey", InputClusterKey)
        self.setProperty("InputRingerKeys", InputRingerKeys)
        self.setProperty("InputRingerL0Key", InputRingerL0Key)
        self.setProperty("InputTruthClusterKey", InputTruthClusterKey)
        self.setProperty("InputTruthRingerKeys", InputTruthRingerKeys)
        self.setProperty("InputTruthElectronKey", InputTruthElectronKey)
        self.setProperty("InputElectronKey", InputElectronKey)
        self.setProperty("InputXTClusterKey", InputXTClusterKey)  # informa os clusters XT ao C++
        self.setProperty("InputXTRingerKeys", InputXTRingerKeys or [])  # informa as topologias XT ao C++
        self.setProperty("InputXTRingerL0Key", InputXTRingerL0Key)  # informa os aneis L0 XT ao C++
        self.setProperty("InputXTElectronKey", InputXTElectronKey)  # informa os eletrons XT ao C++
        self.setProperty("DumpCrossTalkCells", DumpCrossTalkCells)  # habilita as colunas XT no C++
        self.setProperty("OutputNtupleName", OutputNtupleName)
        self.setProperty("OutputLevel", OutputLevel)
