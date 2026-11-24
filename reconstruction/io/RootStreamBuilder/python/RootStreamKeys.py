__all__ = ["recordable"]

valid_keys = [
    "EventInfo",
    "Hits",
    # CaloCellMerge
    "Cells",
    "TruthCells",
    "XTCells",
    # CaloClusterMaker
    "Events",
    "Seeds",
    "Clusters",
    "TruthClusters",
    "XTClusters",  # acrescentei o do XT
    "Particles",
    "TruthParticles",
    # CaloRingsBuilder
    "Rings",
    "XTRings",  # acrescentei os aneis XT da topologia padrao
    "RingsSigmaNoise",
    "TruthRings",
    "AsymRings",
    "TruthAsymRings",
    "XTAsymRings",  # acrescentei os aneis Asym do XT
    "CornerRings",
    "TruthCornerRings",
    "XTCornerRings",  # acrescentei os aneis Corner do XT
    "StripsRings",
    "TruthStripsRings",
    "XTStripsRings",  # acrescentei os aneis Strips do XT
    "CrossRings",
    "TruthCrossRings",
    "XTCrossRings",  # acrescentei os aneis Cross do XT
    "CustomRings",
    "TruthCustomRings",
    "XTCustomRings",  # acrescentei os aneis Custom do XT
    "Electrons",
    "TruthElectrons",
    "XTElectrons",  # acrescentei o do XT
    "RingsL0",
    "TruthRingsL0",
    "XTRingsL0",  # acrescentei o do XT
]


def recordable(key):
    if key not in valid_keys:
        raise RuntimeError(f"Key {key} not allow to be recorded.")
    return key
