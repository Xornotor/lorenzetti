__all__ = ["RootStreamAODMaker"]

from GaugiKernel import Cpp
from GaugiKernel.macros import *
from RootStreamBuilder import RootStreamAODFlags as flags
import ROOT


class RootStreamAODMaker(Cpp):

    def __init__(
        self,
        name,
        InputEventKey: str,
        InputSeedsKey: str,
        InputTruthKey: str,
        InputCellsKey: str,
        InputClusterKey: str,
        InputRingerKeys: list,
        InputRingerL0Key: str,
        InputElectronKey: str,
        InputTruthCellsKey: str,
        InputTruthClusterKey: str,
        InputTruthRingerKeys: list,
        InputTruthElectronKey: str,
        InputXTCellsKey: str = "XTCells",  # chave das celulas com crosstalk
        InputXTClusterKey: str = "XTClusters",  # chave dos clusters com crosstalk
        InputXTRingerKeys: list = None,  # chaves dos aneis com crosstalk
        InputXTRingerL0Key: str = "XTRingsL0",  # chave dos aneis L0 com crosstalk
        InputXTElectronKey: str = "XTElectrons",  # chave dos eletrons com crosstalk
        InputTruthRingerL0Key: str = "TruthRingsL0",  # chave dos aneis L0 da verdade
        OutputEventKey: str = None,
        OutputTruthKey: str = None,
        OutputCellsKey: str = None,
        OutputClusterKey: str = None,
        OutputRingerKeys: list = None,
        OutputRingerL0Key: str = None,
        OutputSeedsKey: str = None,
        OutputElectronKey: str = None,
        OutputTruthCellsKey: str = None,
        OutputTruthClusterKey: str = None,
        OutputTruthRingerKeys: list = None,
        OutputTruthElectronKey: str = None,
        OutputXTCellsKey: str = None,  # nome dos ramos de celulas XT no AOD
        OutputXTClusterKey: str = None,  # nome do ramo de clusters XT no AOD
        OutputXTRingerKeys: list = None,  # nomes dos ramos de aneis XT no AOD
        OutputXTRingerL0Key: str = None,  # nome do ramo de aneis L0 XT no AOD
        OutputXTElectronKey: str = None,  # nome do ramo de eletrons XT no AOD
        OutputLevel: int = 0,
        NtupleName: str = "CollectionTree",
        DumpCells: bool = flags.DumpCells,
        DumpCrossTalkCells: bool = False,  # grava o caminho XT quando ativo
    ):

        Cpp.__init__(self, ROOT.RootStreamAODMaker(name))
        self.setProperty("InputEventKey", InputEventKey)
        self.setProperty("InputTruthKey", InputTruthKey)
        self.setProperty("InputCellsKey", InputCellsKey)
        self.setProperty("InputClusterKey", InputClusterKey)
        self.setProperty("InputRingerKeys", InputRingerKeys)
        self.setProperty("InputSeedsKey", InputSeedsKey)
        self.setProperty("InputElectronKey", InputElectronKey)
        self.setProperty("InputTruthCellsKey", InputTruthCellsKey)
        self.setProperty("InputTruthClusterKey", InputTruthClusterKey)
        self.setProperty("InputTruthRingerKeys", InputTruthRingerKeys)
        self.setProperty("InputXTCellsKey", InputXTCellsKey)  # informa a entrada de celulas XT
        self.setProperty("InputXTClusterKey", InputXTClusterKey)  # informa a entrada de clusters XT
        self.setProperty("InputXTRingerKeys", InputXTRingerKeys or [])  # informa as entradas de aneis XT
        self.setProperty("InputXTRingerL0Key", InputXTRingerL0Key)  # informa a entrada de aneis L0 XT
        self.setProperty("InputXTElectronKey", InputXTElectronKey)  # informa a entrada de eletrons XT
        self.setProperty("InputTruthRingerL0Key", InputTruthRingerL0Key)  # informa a entrada de aneis L0 da verdade
        self.setProperty(
            "OutputEventKey", OutputEventKey if OutputEventKey else InputEventKey
        )
        self.setProperty(
            "OutputTruthKey", OutputTruthKey if OutputTruthKey else InputTruthKey
        )
        self.setProperty(
            "OutputCellsKey", OutputCellsKey if OutputCellsKey else InputCellsKey
        )
        self.setProperty(
            "OutputClusterKey",
            OutputClusterKey if OutputClusterKey else InputClusterKey,
        )
        self.setProperty("OutputRingerKeys", OutputRingerKeys or InputRingerKeys)
        self.setProperty(
            "OutputSeedsKey", OutputSeedsKey if OutputSeedsKey else InputSeedsKey
        )
        self.setProperty(
            "OutputElectronKey",
            OutputElectronKey if OutputElectronKey else InputElectronKey,
        )
        self.setProperty(
            "OutputTruthCellsKey",
            OutputTruthCellsKey if OutputTruthCellsKey else InputTruthCellsKey,
        )
        self.setProperty(
            "OutputTruthClusterKey",
            OutputTruthClusterKey if OutputTruthClusterKey else InputTruthClusterKey,
        )
        self.setProperty(
            "OutputTruthRingerKeys", OutputTruthRingerKeys or InputTruthRingerKeys
        )
        self.setProperty(
            "OutputRingerL0Key",
            OutputRingerL0Key if OutputRingerL0Key else InputRingerL0Key,
        )
        self.setProperty(
            "OutputTruthElectronKey",
            OutputTruthElectronKey if OutputTruthElectronKey else InputTruthElectronKey,
        )
        self.setProperty("OutputLevel", OutputLevel)
        self.setProperty("NtupleName", NtupleName)
        self.setProperty("DumpCells", DumpCells)
        self.setProperty("OutputXTCellsKey", OutputXTCellsKey or InputXTCellsKey)  # define os ramos de celulas XT
        self.setProperty("OutputXTClusterKey", OutputXTClusterKey or InputXTClusterKey)  # define o ramo de clusters XT
        self.setProperty("OutputXTRingerKeys", OutputXTRingerKeys or InputXTRingerKeys or [])  # define os ramos de aneis XT
        self.setProperty("OutputXTRingerL0Key", OutputXTRingerL0Key or InputXTRingerL0Key)  # define o ramo de aneis L0 XT
        self.setProperty("OutputXTElectronKey", OutputXTElectronKey or InputXTElectronKey)  # define o ramo de eletrons XT
        self.setProperty("DumpCrossTalkCells", DumpCrossTalkCells)  # habilita a gravacao do caminho XT
        self.setProperty("OutputTruthRingerL0Key", InputTruthRingerL0Key)  # preserva a chave dos aneis L0 da verdade
