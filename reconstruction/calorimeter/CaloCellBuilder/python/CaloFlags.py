
__all__ = ["CaloFlags", "CrossTalkFlags", "AnomalyFlags"]


from GaugiKernel import EnumStringification
from GaugiKernel.constants import *

class CaloFlags(EnumStringification):

    # cell global parameters
    SamplingnoiseStd    = 0.0 
    # crosstalk flags
    DoCrossTalk         = False
    # energy estimation flags
    DoCOF               = False
    
    DoDefects           = False
    
    
class CrossTalkFlags(EnumStringification):
    SigmaNoiseCut     = 1*GeV
    AmpCapacitive     = 4.2
    AmpInductive      = 2.3
    AmpResistive      = 1.0
    XtStdDevCap       = 0.25
    XtStdDevInd       = 0.25
    
class AnomalyFlags(EnumStringification):
    BadRunListFile   = ""