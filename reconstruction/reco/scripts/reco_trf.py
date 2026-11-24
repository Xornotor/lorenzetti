#!/usr/bin/env python3
import argparse
import sys
import os

from pathlib import Path
from typing import List
from expand_folders import expand_folders
from GaugiKernel import LoggingLevel, get_argparser_formatter
from GaugiKernel import ComponentAccumulator
from RootStreamBuilder import RootStreamESDReader, recordable
from CaloClusterBuilder import CaloClusterMaker
from CaloRingsBuilder import CaloRingsBuilderCfg
from EgammaBuilder import ElectronBuilderCfg
from RootStreamBuilder import RootStreamAODMaker
from RootStreamBuilder.RootStreamFlags import RootStreamAODFlags as flags

from reco.reco_job import merge_args, update_args, create_parallel_job


# imports adicionados para adapataco
from CaloCellBuilder import CaloFlags # vai ser usada para saber se o crostalk esta ativo etc.



"""
Script: reco_trf.py
Purpose: Executes the offline reconstruction chain.
         Reads digitized cells (ESD), builds calorimeter clusters, computes
         ring variables (Rings), and reconstructs electron candidates.
Usage:
    reco_trf.py -i input.ESD.root -o output.AOD.root
"""
TOPOLOGY_MAP = {
    "doStdRings": ("std", "Rings"),
    "doCornerRings": ("corner", "CornerRings"),
    "doAsymRings": ("asym", "AsymRings"),
    "doStripsRings": ("strips", "StripsRings"),
    "doCrossRings": ("cross", "CrossRings"),
    "doCustomRings": ("custom", "CustomRings"),
}


def parse_args():
    """
    Parses command-line arguments for the reconstruction job.

    Returns:
        argparse.Namespace: Arguments for reconstruction configuration.
    """
    # create the top-level parser
    parser = argparse.ArgumentParser(
        description="", formatter_class=get_argparser_formatter(), add_help=False
    )

    parser.add_argument(
        "-l",
        "--output-level",
        action="store",
        dest="output_level",
        required=False,
        type=str,
        default="INFO",
        help="The output level messenger.",
    )
    parser.add_argument(
        "-c",
        "--command",
        action="store",
        dest="command",
        required=False,
        default="''",
        help="The preexec command",
    )

    return merge_args(parser)


def main(
    events: List[int],
    logging_level: str,
    input_file: str | Path,
    output_file: str | Path,
    command: str,
):
    """
    Main function for the reconstruction workflow.

    Orchestrates the reconstruction sequence:
    1. Reads ESD file (Cells, Particles, Seeds).
    2. Runs CaloClusterMaker to group cells into clusters.
    3. Runs CaloRingsBuilder to extract concentric ring energy sums.
    4. Runs ElectronBuilder to create electron candidates.
    5. Writes the results to an Analysis Object Data (AOD) file.

    Args:
        events (List[int]): List of event indices.
        logging_level (str): Logging verbosity.
        input_file (str | Path): Path to input ESD file.
        output_file (str | Path): Path to output AOD file.
        command (str): Optional command to execute before the sequence.
    """
    if isinstance(input_file, Path):
        input_file = str(input_file)
    if isinstance(output_file, Path):
        output_file = str(output_file)

    outputLevel = LoggingLevel.toC(logging_level)

    exec(command)

    acc = ComponentAccumulator("ComponentAccumulator", output_file)


    # leitura do aquivo ESD gerado na etapa de Digitalizacao
    # PROBLEMA :
    #
    ESD = RootStreamESDReader(
        "ESDReader",
        InputFile=input_file,
        OutputCellsKey=recordable("Cells"),
        OutputCellsTruthKey=recordable("TruthCells"),
        OutputEventKey=recordable("Events"),
        OutputTruthKey=recordable("Particles"),
        OutputSeedsKey=recordable("Seeds"),
        OutputLevel=outputLevel,
        # O  RootStreamESDReader definido NESTE LOCAL    reconstruction/io/RootStreamBuilder/python/RootStreamESDReader.py,
        # exige TAMBEM  estes dois argumentos, que faltavam nesta chamada. ENTAO ACRESCENTEI SO DOOIS 
        #  1 - A chave nomeia a coleção XTCells criada em memória 
        #  2-  a flag habilita
        # o nosso arquivo ESD tem esses parametros.
        OutputXTCellsKey=recordable("XTCells"),
        DumpCrossTalkCells=CaloFlags.DoCrossTalk, # foi adicioando o importe no inicio para esse flag
    )
    ESD.merge(acc)

    # cria o cluster para o sinal sem crostalk - dados sem crostalk - energia e tempo - estimados pelo OF
    cluster = CaloClusterMaker(
        "CaloClusterMaker",
        InputCellsKey=recordable("Cells"),
        InputSeedsKey=recordable("Seeds"),
        # output as
        OutputClusterKey=recordable("Clusters"),
        # other configs
        HistogramPath="Expert/Clusters",
        OutputLevel=outputLevel,
    )

   # cria o cluster para o sinal da verdade da simulacao - o tempo e energai vindos diretor da simulacao sem pasa pelo OF
    cluster_truth = CaloClusterMaker(
        "CaloClusterMaker_Truth",
        InputCellsKey=recordable("TruthCells"),
        InputSeedsKey=recordable("Seeds"),
        # output as
        OutputClusterKey=recordable("TruthClusters"),
        # other configs
        HistogramPath="Expert/TruthClusters",
        OutputLevel=outputLevel,
    )

    # Eu acrescentei um para o CROSSTALK igual ao primeiro so que com o sinal contaminado com crosstalk

    if CaloFlags.DoCrossTalk: # aqui tem o if diferente dos demais porque depende do crostakl estar ativo
        cluster_xt = CaloClusterMaker(
            "CaloClusterMaker_XT",
            InputCellsKey=recordable("XTCells"),
            InputSeedsKey=recordable("Seeds"),
            OutputClusterKey=recordable("XTClusters"),
            HistogramPath="Expert/XTClusters",
            OutputLevel=outputLevel,
        )

        



    # DEVO REPLICAR ESSE BLOCO PARA OS OUTROS 2
    ringsL0 = CaloRingsBuilderCfg(
        "CaloRingsBuilderL0",
        InputClusterKey=recordable("Clusters"),
        OutputRingerKey=recordable("RingsL0"),
        HistogramPath="Expert/RingsL0",
        OutputLevel=outputLevel,
        DoSigmaCut=True,
        SigmaCut=flags.SigmaCut,
    )


    # Acrescentei anEis para o caminho Truth, usando os clusters de referência[VERDADE]
    # Mantive o mesmo corte em sigma para comparar com os RingsL0 normais
    #MAS NAO TENHO CERTEZA NESSE SigmaCut ja que o dado da simulacao Truth é perfeito    

    ringsL0_truth = CaloRingsBuilderCfg(
        "CaloRingsBuilderL0_Truth",
        InputClusterKey=recordable("TruthClusters"),
        OutputRingerKey=recordable("TruthRingsL0"),
        HistogramPath="Expert/TruthRingsL0",
        OutputLevel=outputLevel,
        DoSigmaCut=True,
        SigmaCut=flags.SigmaCut,
    )

    # Acrescentei anéis para o caminho XT, usando os clusters com crosstalk.
    if CaloFlags.DoCrossTalk:  # Só configura esse caminho quando o crosstalk está ativo.
        ringsL0_xt = CaloRingsBuilderCfg(
            "CaloRingsBuilderL0_XT",
            InputClusterKey=recordable("XTClusters"),
            OutputRingerKey=recordable("XTRingsL0"),
            HistogramPath="Expert/XTRingsL0",
            OutputLevel=outputLevel,
            DoSigmaCut=True,
            SigmaCut=flags.SigmaCut,
        )


    hypo = ElectronBuilderCfg(
        "ElectronBuilder",
        InputClusterKey=recordable("Clusters"),
        OutputElectronKey=recordable("Electrons"),
        OutputLevel=outputLevel,
    )

    hypo_truth = ElectronBuilderCfg(
        "ElectronBuilder_Truth",
        InputClusterKey=recordable("TruthClusters"),
        OutputElectronKey=recordable("TruthElectrons"),
        OutputLevel=outputLevel,
    )

    # eu acerscentei o caminho do Crosstalk igual ao Normal so que usando XTClusters
    # a Saida fica em XTElectrons para nao misturar com os Electrons normais
    if CaloFlags.DoCrossTalk:  # esse if so monta o caminho quando o Crosstalk esta ativo
        hypo_xt = ElectronBuilderCfg(
            "ElectronBuilder_XT",
            InputClusterKey=recordable("XTClusters"),
            OutputElectronKey=recordable("XTElectrons"),
            OutputLevel=outputLevel,
        )


    ring_keys = []
    truth_ring_keys = []
    xt_ring_keys = [] # nomes das Saidas de aneis do XT

    acc_rings = []
    acc_rings_truth = []
    acc_rings_xt = [] # algoritmos que vao criar os aneis do XT

    for flag_name, (topology, key_suffix) in TOPOLOGY_MAP.items():
        if not getattr(flags, flag_name):
            continue

        extra = {}
        if topology == "corner":
            extra["CornerShift"] = flags.CornerShift
        elif topology == "cross":
            extra["CrossShift"] = flags.CrossShift
        elif topology == "custom":
            extra["RingsShiftEta"] = flags.CustomRingsShiftEta
            extra["RingsShiftPhi"] = flags.CustomRingsShiftPhi
        elif topology == "strips":
            extra["Axis"] = flags.StripsAxis

        acc_rings.append(
            CaloRingsBuilderCfg(
                f"CaloRingsBuilder_{topology}",
                InputClusterKey=recordable("Clusters"),
                OutputRingerKey=recordable(key_suffix),
                HistogramPath=f"Expert/{key_suffix}",
                OutputLevel=outputLevel,
                RingerTopology=topology,
                **extra,
            )
        )

        acc_rings_truth.append(
            CaloRingsBuilderCfg(
                f"CaloRingsBuilder_Truth_{topology}",
                InputClusterKey=recordable("TruthClusters"),
                OutputRingerKey=recordable(f"Truth{key_suffix}"),
                HistogramPath=f"Expert/Truth{key_suffix}",
                OutputLevel=outputLevel,
                RingerTopology=topology,
                **extra,
            )
        )

        # acerscentei o caminho XT para criar os aneis usando os XTClusters
        if CaloFlags.DoCrossTalk:  # so cria os aneis XT se o Crosstalk estiver ativo
            acc_rings_xt.append(
                CaloRingsBuilderCfg(
                    f"CaloRingsBuilder_XT_{topology}",
                    InputClusterKey=recordable("XTClusters"),
                    OutputRingerKey=recordable(f"XT{key_suffix}"),
                    HistogramPath=f"Expert/XT{key_suffix}",
                    OutputLevel=outputLevel,
                    RingerTopology=topology,
                    **extra,
                )
            )
            

        ring_keys.append(recordable(key_suffix))
        truth_ring_keys.append(recordable(f"Truth{key_suffix}"))
        # acerscentei a chave dos anEis XT separada das outras Saidas
        if CaloFlags.DoCrossTalk:
            xt_ring_keys.append(recordable(f"XT{key_suffix}"))



# agora precisa ajustar o AOD 
    AOD = RootStreamAODMaker(
        "RootStreamAODMaker",
        InputEventKey=recordable("Events"),
        InputSeedsKey=recordable("Seeds"),
        InputTruthKey=recordable("Particles"),
        InputCellsKey=recordable("Cells"),
        InputTruthCellsKey=recordable("TruthCells"),
        InputXTCellsKey=recordable("XTCells"),  # acrescentei as celulas com Crosstalk
        InputClusterKey=recordable("Clusters"),
        InputTruthClusterKey=recordable("TruthClusters"),
        InputXTClusterKey=recordable("XTClusters"),  # acrescentei os clusters do caminho XT
        InputRingerKeys=ring_keys,
        InputTruthRingerKeys=truth_ring_keys,
        InputXTRingerKeys=xt_ring_keys,  # acrescentei a lista de aneis do caminho XT
        InputElectronKey=recordable("Electrons"),
        InputTruthElectronKey=recordable("TruthElectrons"),
        InputXTElectronKey=recordable("XTElectrons"),  # acrescentei os eletrons do caminho XT
        InputRingerL0Key=recordable("RingsL0"),
        InputTruthRingerL0Key=recordable("TruthRingsL0"),  # acrescentei o RingsL0 da linha Truth
        InputXTRingerL0Key=recordable("XTRingsL0"),  # acrescentei o RingsL0 do caminho XT
        DumpCrossTalkCells=CaloFlags.DoCrossTalk,  # acrescentei a flag para gravar o caminho XT quando estiver ativo
        OutputLevel=outputLevel,
    )

    #AJUSTAR A SEQUENCIA     
    # sequence
    acc += cluster
    for rings in acc_rings:
        acc += rings
    acc += ringsL0
    acc += hypo

    acc += cluster_truth
    for rings_truth in acc_rings_truth:
        acc += rings_truth
    acc += ringsL0_truth  # acrescentei o RingsL0 da linha Truth [ESSE TEM QUE CONSIDERAR DEPOIS SE FAZ SENTINDO OU NAO]
    acc += hypo_truth

    if CaloFlags.DoCrossTalk:  # acrescentei a sequencia XT so quando o Crosstalk estiver ativo
        acc += cluster_xt
        for rings_xt in acc_rings_xt:
            acc += rings_xt
        acc += ringsL0_xt  # acrescentei o RingsL0 do caminho XT
        acc += hypo_xt  # acrescentei os eletrons do caminho XT

    acc += AOD
    acc.run(events)


if __name__ == "__main__":
    parser = parse_args()
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)
    args = parser.parse_args()
    args = update_args(args)
    pool = create_parallel_job(args)
    pool(
        main,
        logging_level=args.output_level,
        command=args.command,
    )
