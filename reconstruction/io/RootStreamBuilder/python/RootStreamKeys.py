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
    "XTClusters",
    "Particles",
    "TruthParticles",
    # CaloRingsBuilder
    "Rings",
    "XTRings",
    "RingsSigmaNoise",
    "TruthRings",
    "AsymRings",
    "TruthAsymRings",
    "XTAsymRings",
    "CornerRings",
    "TruthCornerRings",
    "XTCornerRings",
    "StripsRings",
    "TruthStripsRings",
    "XTStripsRings",
    "CrossRings",
    "TruthCrossRings",
    "XTCrossRings",
    "CustomRings",
    "TruthCustomRings",
    "XTCustomRings",
    "Electrons",
    "TruthElectrons",
    "XTElectrons",
    "RingsL0",
    "TruthRingsL0",
    "XTRingsL0",
]


def recordable(key):
    if key not in valid_keys:
        raise RuntimeError(f"Key {key} not allow to be recorded.")
    return key
