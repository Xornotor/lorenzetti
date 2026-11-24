#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path
from typing import List

import ROOT as _ROOT
from expand_folders import expand_folders
from GaugiKernel import LoggingLevel, get_argparser_formatter, ComponentAccumulator
from RootStreamBuilder import RootStreamAODReader, recordable
from RootStreamBuilder import RootStreamNtupleMaker
from reco.reco_job import merge_args, update_args, create_parallel_job


def _detect_ring_keys(input_file: str) -> tuple[list, list, list, bool]:
    """Le as topologias de aneis e identifica se o AOD contem o caminho XT"""
    f = _ROOT.TFile.Open(input_file)
    if not f or f.IsZombie():
        return ["Rings"], ["TruthRings"], [], False
    tree = f.Get("CollectionTree")
    if not tree:
        f.Close()
        return ["Rings"], ["TruthRings"], [], False

    ring_keys, truth_ring_keys, xt_ring_keys = [], [], []  # separa os aneis dos tres caminhos
    branch_names = {branch.GetName() for branch in tree.GetListOfBranches()}
    do_crosstalk = "CaloClusterContainer_XTClusters" in branch_names  # reconhece um AOD com XT
    for branch in tree.GetListOfBranches():
        name = branch.GetName()
        if not name.startswith("CaloRingsContainer_"):
            continue
        key = name[len("CaloRingsContainer_") :]
        if key.endswith("RingsL0"):
            continue
        if key.startswith("XT"):
            xt_ring_keys.append(key)  # evita misturar aneis XT com os normais
        elif key.startswith("Truth"):
            truth_ring_keys.append(key)
        else:
            ring_keys.append(key)

    f.Close()
    return ring_keys or ["Rings"], truth_ring_keys or ["TruthRings"], xt_ring_keys, do_crosstalk


def parse_args():
    parser = argparse.ArgumentParser(
        description="", formatter_class=get_argparser_formatter(), add_help=False
    )
    parser.add_argument(
        "-l",
        "--output-level",
        dest="output_level",
        default="INFO",
        help="Logging verbosity.",
    )
    return merge_args(parser)


def main(
    events: List[int],
    logging_level: str,
    input_file: str | Path,
    output_file: str | Path,
):
    if isinstance(input_file, Path):
        input_file = str(input_file)
    if isinstance(output_file, Path):
        output_file = str(output_file)

    outputLevel = LoggingLevel.toC(logging_level)

    ring_keys, truth_ring_keys, xt_ring_keys, do_crosstalk = _detect_ring_keys(input_file)

    acc = ComponentAccumulator("ComponentAccumulator", output_file)

    aod = RootStreamAODReader(
        "AODReader",
        InputFile=input_file,
        OutputLevel=outputLevel,
        OutputEventKey=recordable("Events"),
        OutputTruthKey=recordable("Particles"),
        OutputClusterKey=recordable("Clusters"),
        OutputRingerKeys=[recordable(k) for k in ring_keys],
        OutputElectronKey=recordable("Electrons"),
        OutputSeedsKey=recordable("Seeds"),
        OutputTruthClusterKey=recordable("TruthClusters"),
        OutputTruthRingerKeys=[recordable(k) for k in truth_ring_keys],
        OutputRingerL0Key=recordable("RingsL0"),
        OutputTruthElectronKey=recordable("TruthElectrons"),
        OutputXTCellsKey=recordable("XTCells"),  # entrega as celulas XT lidas do AOD
        OutputXTClusterKey=recordable("XTClusters"),  # entrega os clusters XT
        OutputXTRingerKeys=[recordable(k) for k in xt_ring_keys],  # entrega as topologias XT
        OutputXTRingerL0Key=recordable("XTRingsL0"),  # entrega os aneis L0 XT
        OutputXTElectronKey=recordable("XTElectrons"),  # entrega os eletrons XT
        DumpCrossTalkCells=do_crosstalk,  # ativa a leitura XT somente para um AOD com XT
        NtupleName="CollectionTree",
    )
    aod.merge(acc)

    ntuple = RootStreamNtupleMaker(
        "NtupleMaker",
        OutputLevel=outputLevel,
        InputEventKey=recordable("Events"),
        InputTruthKey=recordable("Particles"),
        InputClusterKey=recordable("Clusters"),
        InputRingerKeys=[recordable(k) for k in ring_keys],
        InputRingerL0Key=recordable("RingsL0"),
        InputElectronKey=recordable("Electrons"),
        InputSeedsKey=recordable("Seeds"),
        InputTruthClusterKey=recordable("TruthClusters"),
        InputTruthRingerKeys=[recordable(k) for k in truth_ring_keys],
        InputTruthElectronKey=recordable("TruthElectrons"),
        InputXTClusterKey=recordable("XTClusters"),  # le os clusters XT do AODReader
        InputXTRingerKeys=[recordable(k) for k in xt_ring_keys],  # le as topologias XT
        InputXTRingerL0Key=recordable("XTRingsL0"),  # le os aneis L0 XT
        InputXTElectronKey=recordable("XTElectrons"),  # le os eletrons XT
        DumpCrossTalkCells=do_crosstalk,  # escreve as colunas XT quando presentes no AOD
        OutputNtupleName="physics",
    )
    acc += ntuple
    acc.run(events)


if __name__ == "__main__":
    parser = parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)
    args = parse_args().parse_args()
    args = update_args(args)
    pool = create_parallel_job(args)
    pool(
        main,
        logging_level=args.output_level,
    )
