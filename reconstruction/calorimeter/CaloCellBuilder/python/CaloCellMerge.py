__all__ = ["CaloCellMerge"]

from GaugiKernel import Cpp, LoggingLevel
from GaugiKernel.macros import *
import ROOT

class CaloCellMerge( Cpp ):


  def __init__( self, name            : str, 
                InputCollectionKeys   : str="Collection" ,
                InputXTCollectionKeys : str="XTCollection" ,
                OutputCellsKey        : str="Cells",
                OutputTruthCellsKey   : str="TruthCells",
                OutputXTCellsKey      : str="XTCells",
                DumpCrossTalkCells    : bool=False,
                OutputLevel           : int=LoggingLevel.toC('INFO'), 
                ): 
    
    Cpp.__init__(self, ROOT.CaloCellMerge(name) )
    # Create the algorithm
    self.setProperty( "InputCollectionKeys"   , InputCollectionKeys )
    self.setProperty( "InputXTCollectionKeys" , InputXTCollectionKeys ) 
    self.setProperty( "OutputCellsKey"        , OutputCellsKey      ) 
    self.setProperty( "OutputTruthCellsKey"   , OutputTruthCellsKey ) 
    self.setProperty( "OutputXTCellsKey"      , OutputXTCellsKey    )
    self.setProperty( "DumpCrossTalkCells"    , DumpCrossTalkCells  )
    self.setProperty( "OutputLevel"           , OutputLevel         ) 

  








