/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//  Copyright 2013-2014 Mickael Philit.

#include "vtkCGNSReader.h"
#include "vtkCGNSReaderInternal.h" // For parsing information request

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformationKeys.h"
#include "vtkPointData.h"
#include "vtkPolyhedron.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <map>
#include <string.h>
#include <string>
#include <vector>

#include <vtksys/SystemTools.hxx>

#include "cgio_helpers.h"

vtkStandardNewMacro(vtkCGNSReader);

namespace
{
struct duo_t
{
  duo_t()
  {
    pair[0] = 0;
    pair[1] = 0;
  }

  int& operator[](std::size_t n) { return pair[n]; }
  const int& operator[](std::size_t n) const { return pair[n]; }
private:
  int pair[2];
};

class SectionInformation
{
public:
  CGNSRead::char_33 name;
  CGNS_ENUMT(ElementType_t) elemType;
  cgsize_t range[2];
  int bound;
  cgsize_t eDataSize;
};
}

// vtkCGNSReader has several method that used types from CGNS
// which resulted in CGNS include being exposed to the users of this class
// causing build complications. This makes that easier.
class vtkCGNSReader::vtkPrivate
{
public:
  static bool IsVarEnabled(
    CGNS_ENUMT(GridLocation_t) varcentering, const CGNSRead::char_33 name, vtkCGNSReader* self);
  static int getGridAndSolutionName(int base, CGNSRead::char_33 GridCoordName,
    CGNSRead::char_33 SolutionName, bool& readGridCoordName, bool& readSolutionName,
    vtkCGNSReader* self);
  static int getCoordsIdAndFillRind(const CGNSRead::char_33 GridCoordName, const int physicalDim,
    std::size_t& nCoordsArray, std::vector<double>& gridChildId, int* rind, vtkCGNSReader* self);
  static int getVarsIdAndFillRind(const double cgioSolId, std::size_t& nVarArray,
    CGNS_ENUMT(GridLocation_t) & varCentering, std::vector<double>& solChildId, int* rind,
    vtkCGNSReader* self);

  static int fillArrayInformation(const std::vector<double>& solChildId, const int physicalDim,
    std::vector<CGNSRead::CGNSVariable>& cgnsVars, std::vector<CGNSRead::CGNSVector>& cgnsVectors,
    vtkCGNSReader* self);

  static int AllocateVtkArray(const int physicalDim, const vtkIdType nVals,
    const CGNS_ENUMT(GridLocation_t) varCentering,
    const std::vector<CGNSRead::CGNSVariable>& cgnsVars,
    const std::vector<CGNSRead::CGNSVector>& cgnsVectors, std::vector<vtkDataArray*>& vtkVars,
    vtkCGNSReader* self);

  static int AttachReferenceValue(const int base, vtkDataSet* ds, vtkCGNSReader* self);
};

//----------------------------------------------------------------------------
vtkCGNSReader::vtkCGNSReader()
  : Internal(new CGNSRead::vtkCGNSMetaData())
{
  this->FileName = NULL;

  this->LoadBndPatch = 0;
  this->NumberOfBases = 0;
  this->ActualTimeStep = 0;
  this->DoublePrecisionMesh = 1;
  this->CreateEachSolutionAsBlock = 0;

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->BaseSelection = vtkDataArraySelection::New();

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkCGNSReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->BaseSelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->ProcRank = 0;
  this->ProcSize = 1;
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCGNSReader::~vtkCGNSReader()
{
  this->SetFileName(0);

  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->CellDataArraySelection->Delete();
  this->BaseSelection->RemoveObserver(this->SelectionObserver);
  this->BaseSelection->Delete();
  this->SelectionObserver->Delete();
  this->SetController(NULL);

  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetController(vtkMultiProcessController* c)
{
  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller)
  {
    this->Controller->UnRegister(this);
  }

  this->Controller = c;

  if (this->Controller)
  {
    this->Controller->Register(this);
    this->ProcRank = this->Controller->GetLocalProcessId();
    this->ProcSize = this->Controller->GetNumberOfProcesses();
  }

  if (!this->Controller || this->ProcSize <= 0)
  {
    this->ProcRank = 0;
    this->ProcSize = 1;
  }
}

//------------------------------------------------------------------------------
bool vtkCGNSReader::vtkPrivate::IsVarEnabled(
  CGNS_ENUMT(GridLocation_t) varcentering, const CGNSRead::char_33 name, vtkCGNSReader* self)
{
  vtkDataArraySelection* DataSelection = 0;
  if (varcentering == CGNS_ENUMV(Vertex))
  {
    DataSelection = self->PointDataArraySelection;
  }
  else
  {
    DataSelection = self->CellDataArraySelection;
  }

  return (DataSelection->ArrayIsEnabled(name) != 0);
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getGridAndSolutionName(const int base,
  CGNSRead::char_33 GridCoordName, CGNSRead::char_33 SolutionName, bool& readGridCoordName,
  bool& readSolutionName, vtkCGNSReader* self)
{
  //
  // Get Coordinates and FlowSolution node names
  readGridCoordName = true;
  readSolutionName = true;

  if ((self->Internal->GetBase(base).useGridPointers == true) ||
    (self->Internal->GetBase(base).useFlowPointers == true))
  {

    CGNSRead::char_33 zoneIterName;
    double ziterId = 0;
    std::size_t ptSize = 32 * self->Internal->GetBase(base).steps.size() + 1;
    char* pointers = new char[ptSize];

    if (CGNSRead::getFirstNodeId(self->cgioNum, self->currentId, "ZoneIterativeData_t", &ziterId) ==
      CG_OK)
    {
      cgio_get_name(self->cgioNum, ziterId, zoneIterName);
      //
      CGNSRead::char_33 nodeLabel;
      CGNSRead::char_33 nodeName;
      std::vector<double> iterChildId;

      CGNSRead::getNodeChildrenId(self->cgioNum, ziterId, iterChildId);

      for (std::size_t nn = 0; nn < iterChildId.size(); nn++)
      {

        if (cgio_get_name(self->cgioNum, iterChildId[nn], nodeName) != CG_OK)
        {
          return 1;
        }
        if (cgio_get_label(self->cgioNum, iterChildId[nn], nodeLabel) != CG_OK)
        {
          return 1;
        }
        bool isDataArray = (strcmp(nodeLabel, "DataArray_t") == 0);
        if (isDataArray && (strcmp(nodeName, "GridCoordinatesPointers") == 0))
        {
          cgio_read_block_data(self->cgioNum, iterChildId[nn],
            (cgsize_t)(self->ActualTimeStep * 32 + 1), (cgsize_t)(self->ActualTimeStep * 32 + 32),
            (void*)GridCoordName);
          GridCoordName[32] = '\0';
          readGridCoordName = false;
        }
        else if (isDataArray && (strcmp(nodeName, "FlowSolutionPointers") == 0))
        {
          cgio_read_block_data(self->cgioNum, iterChildId[nn],
            (cgsize_t)(self->ActualTimeStep * 32 + 1), (cgsize_t)(self->ActualTimeStep * 32 + 32),
            (void*)SolutionName);
          SolutionName[32] = '\0';
          readSolutionName = false;
        }
        cgio_release_id(self->cgioNum, iterChildId[nn]);
      }
    }
    else
    {
      strcpy(GridCoordName, "GridCoordinates");
      strcpy(SolutionName, "FlowSolution");
    }
    delete[] pointers;
  }
  //
  if (readGridCoordName)
  {
    // By default the grid name should be GridCoordinates
    strcpy(GridCoordName, "GridCoordinates");
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getCoordsIdAndFillRind(const CGNSRead::char_33 GridCoordName,
  const int physicalDim, std::size_t& nCoordsArray, std::vector<double>& gridChildId, int* rind,
  vtkCGNSReader* self)
{
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  std::size_t na;

  nCoordsArray = 0;
  // Get GridCoordinate node ID for low level access
  double gridId;
  if (cgio_get_node_id(self->cgioNum, self->currentId, GridCoordName, &gridId) != CG_OK)
  {
    char message[81];
    cgio_error_message(message);
    vtkErrorWithObjectMacro(self, << "Error while reading mesh coordinates node :" << message);
    return 1;
  }

  // Get the number of Coordinates in GridCoordinates node
  CGNSRead::getNodeChildrenId(self->cgioNum, gridId, gridChildId);

  for (int n = 0; n < 6; n++)
  {
    rind[n] = 0;
  }
  for (nCoordsArray = 0, na = 0; na < gridChildId.size(); ++na)
  {
    if (cgio_get_label(self->cgioNum, gridChildId[na], nodeLabel) != CG_OK)
    {
      vtkErrorWithObjectMacro(self, << "Not enough coordinates in node " << GridCoordName << "\n");
      continue;
    }

    if (strcmp(nodeLabel, "DataArray_t") == 0)
    {
      if (nCoordsArray < na)
      {
        gridChildId[nCoordsArray] = gridChildId[na];
      }
      nCoordsArray++;
    }
    else if (strcmp(nodeLabel, "Rind_t") == 0)
    {
      // check for rind
      CGNSRead::setUpRind(self->cgioNum, gridChildId[na], rind);
    }
    else
    {
      cgio_release_id(self->cgioNum, gridChildId[na]);
    }
  }
  if (nCoordsArray < static_cast<std::size_t>(physicalDim))
  {
    vtkErrorWithObjectMacro(self, << "Not enough coordinates in node " << GridCoordName << "\n");
    return 1;
  }
  cgio_release_id(self->cgioNum, gridId);
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::getVarsIdAndFillRind(const double cgioSolId, std::size_t& nVarArray,
  CGNS_ENUMT(GridLocation_t) & varCentering, std::vector<double>& solChildId, int* rind,
  vtkCGNSReader* self)
{
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  std::size_t na;

  nVarArray = 0;
  for (int n = 0; n < 6; ++n)
  {
    rind[n] = 0;
  }

  CGNSRead::getNodeChildrenId(self->cgioNum, cgioSolId, solChildId);

  for (nVarArray = 0, na = 0; na < solChildId.size(); ++na)
  {
    if (cgio_get_label(self->cgioNum, solChildId[na], nodeLabel) != CG_OK)
    {
      vtkErrorWithObjectMacro(self, << "Error while reading node label in solution\n");
      continue;
    }

    if (strcmp(nodeLabel, "DataArray_t") == 0)
    {
      if (nVarArray < na)
      {
        solChildId[nVarArray] = solChildId[na];
      }
      nVarArray++;
    }
    else if (strcmp(nodeLabel, "Rind_t") == 0)
    {
      CGNSRead::setUpRind(self->cgioNum, solChildId[na], rind);
    }
    else if (strcmp(nodeLabel, "GridLocation_t") == 0)
    {
      CGNSRead::char_33 dataType;

      if (cgio_get_data_type(self->cgioNum, solChildId[na], dataType) != CG_OK)
      {
        return 1;
      }

      if (strcmp(dataType, "C1") != 0)
      {
        std::cerr << "Unexpected data type for GridLocation_t node" << std::endl;
        return 1;
      }

      std::string location;
      CGNSRead::readNodeStringData(self->cgioNum, solChildId[na], location);

      if (location == "Vertex")
      {
        varCentering = CGNS_ENUMV(Vertex);
      }
      else if (location == "CellCenter")
      {
        varCentering = CGNS_ENUMV(CellCenter);
      }
      else
      {
        varCentering = CGNS_ENUMV(GridLocationNull);
      }
    }
    else
    {
      cgio_release_id(self->cgioNum, solChildId[na]);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::fillArrayInformation(const std::vector<double>& solChildId,
  const int physicalDim, std::vector<CGNSRead::CGNSVariable>& cgnsVars,
  std::vector<CGNSRead::CGNSVector>& cgnsVectors, vtkCGNSReader* self)
{
  // Read variable names
  for (std::size_t ff = 0; ff < cgnsVars.size(); ++ff)
  {
    cgio_get_name(self->cgioNum, solChildId[ff], cgnsVars[ff].name);
    cgnsVars[ff].isComponent = false;
    cgnsVars[ff].xyzIndex = 0;

    // read node data type
    CGNSRead::char_33 dataType;
    cgio_get_data_type(self->cgioNum, solChildId[ff], dataType);
    if (strcmp(dataType, "R8") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(RealDouble);
    }
    else if (strcmp(dataType, "R4") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(RealSingle);
    }
    else if (strcmp(dataType, "I4") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(Integer);
    }
    else if (strcmp(dataType, "I8") == 0)
    {
      cgnsVars[ff].dt = CGNS_ENUMV(LongInteger);
    }
    else
    {
      continue;
    }
  }
  // Create vector name from available variable
  // when VarX, VarY, VarZ is detected
  CGNSRead::fillVectorsFromVars(cgnsVars, cgnsVectors, physicalDim);
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::AllocateVtkArray(const int physicalDim, const vtkIdType nVals,
  const CGNS_ENUMT(GridLocation_t) varCentering,
  const std::vector<CGNSRead::CGNSVariable>& cgnsVars,
  const std::vector<CGNSRead::CGNSVector>& cgnsVectors, std::vector<vtkDataArray*>& vtkVars,
  vtkCGNSReader* self)
{
  for (std::size_t ff = 0; ff < cgnsVars.size(); ff++)
  {
    vtkVars[ff] = 0;

    if (cgnsVars[ff].isComponent == false)
    {
      if (vtkPrivate::IsVarEnabled(varCentering, cgnsVars[ff].name, self) == false)
      {
        continue;
      }

      switch (cgnsVars[ff].dt)
      {
        // Other case to handle
        case CGNS_ENUMV(Integer):
          vtkVars[ff] = vtkIntArray::New();
          break;
        case CGNS_ENUMV(LongInteger):
          vtkVars[ff] = vtkLongArray::New();
          break;
        case CGNS_ENUMV(RealSingle):
          vtkVars[ff] = vtkFloatArray::New();
          break;
        case CGNS_ENUMV(RealDouble):
          vtkVars[ff] = vtkDoubleArray::New();
          break;
        case CGNS_ENUMV(Character):
          vtkVars[ff] = vtkCharArray::New();
          break;
        default:
          continue;
      }
      vtkVars[ff]->SetName(cgnsVars[ff].name);
      vtkVars[ff]->SetNumberOfComponents(1);
      vtkVars[ff]->SetNumberOfTuples(nVals);
    }
  }

  for (std::vector<CGNSRead::CGNSVector>::const_iterator iter = cgnsVectors.begin();
       iter != cgnsVectors.end(); ++iter)
  {
    vtkDataArray* arr = 0;

    if (vtkPrivate::IsVarEnabled(varCentering, iter->name, self) == false)
    {
      continue;
    }

    int nv = iter->xyzIndex[0];
    switch (cgnsVars[nv].dt)
    {
      // TODO: other cases
      case CGNS_ENUMV(Integer):
        arr = vtkIntArray::New();
        break;
      case CGNS_ENUMV(LongInteger):
        arr = vtkLongArray::New();
        break;
      case CGNS_ENUMV(RealSingle):
        arr = vtkFloatArray::New();
        break;
      case CGNS_ENUMV(RealDouble):
        arr = vtkDoubleArray::New();
        break;
      case CGNS_ENUMV(Character):
        arr = vtkCharArray::New();
        break;
      default:
        continue;
    }

    arr->SetName(iter->name);
    arr->SetNumberOfComponents(physicalDim);
    arr->SetNumberOfTuples(nVals);

    for (int dim = 0; dim < physicalDim; ++dim)
    {
      arr->SetComponentName(static_cast<vtkIdType>(dim), cgnsVars[iter->xyzIndex[dim]].name);
      vtkVars[iter->xyzIndex[dim]] = arr;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::vtkPrivate::AttachReferenceValue(
  const int base, vtkDataSet* ds, vtkCGNSReader* self)
{
  // Handle Reference Values (Mach Number, ...)
  const std::map<std::string, double>& arrState = self->Internal->GetBase(base).referenceState;
  std::map<std::string, double>::const_iterator iteRef = arrState.begin();
  for (iteRef = arrState.begin(); iteRef != arrState.end(); iteRef++)
  {
    vtkDoubleArray* refValArray = vtkDoubleArray::New();
    refValArray->SetNumberOfComponents(1);
    refValArray->SetName(iteRef->first.c_str());
    refValArray->InsertNextValue(iteRef->second);
    ds->GetFieldData()->AddArray(refValArray);
    refValArray->Delete();
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::GetCurvilinearZone(
  int base, int zone, int cellDim, int physicalDim, void* v_zsize, vtkMultiBlockDataSet* mbase)
{
  cgsize_t* zsize = reinterpret_cast<cgsize_t*>(v_zsize);

  int rind[6];
  int n;
  // int ier;

  // Source Layout
  cgsize_t srcStart[3] = { 1, 1, 1 };
  cgsize_t srcStride[3] = { 1, 1, 1 };
  cgsize_t srcEnd[3];
  // Memory Destination Layout
  cgsize_t memStart[3] = { 1, 1, 1 };
  cgsize_t memStride[3] = { 3, 1, 1 };
  cgsize_t memEnd[3] = { 1, 1, 1 };
  cgsize_t memDims[3] = { 1, 1, 1 };

  vtkIdType nPts = 0;

  // Get Coordinates and FlowSolution node names
  bool readGridCoordName = true;
  bool readSolutionName = true;
  CGNSRead::char_33 GridCoordName;
  CGNSRead::char_33 SolutionName;

  std::vector<double> gridChildId;
  std::size_t nCoordsArray = 0;

  vtkPrivate::getGridAndSolutionName(
    base, GridCoordName, SolutionName, readGridCoordName, readSolutionName, this);

  vtkPrivate::getCoordsIdAndFillRind(
    GridCoordName, physicalDim, nCoordsArray, gridChildId, rind, this);

  // Rind was parsed (or not) then populate dimensions :
  // Compute structured grid coordinate range
  for (n = 0; n < cellDim; n++)
  {
    srcStart[n] = rind[2 * n] + 1;
    srcEnd[n] = rind[2 * n] + zsize[n];
    memEnd[n] = zsize[n];
    memDims[n] = zsize[n];
  }

  // Compute number of points
  nPts = static_cast<vtkIdType>(memEnd[0] * memEnd[1] * memEnd[2]);

  // Populate the extent array
  int extent[6] = { 0, 0, 0, 0, 0, 0 };
  extent[1] = memEnd[0] - 1;
  extent[3] = memEnd[1] - 1;
  extent[5] = memEnd[2] - 1;

  // wacky hack ...
  // memory aliasing is done
  // since in vtk points array stores XYZ contiguously
  // and they are stored separatly in cgns file
  // the memory layout is set so that one cgns file array
  // will be filling every 3 chuncks in memory
  memEnd[0] *= 3;

  // Set up points
  vtkPoints* points = vtkPoints::New();
  //
  // vtkPoints assumes float data type
  //
  if (this->DoublePrecisionMesh != 0)
  {
    points->SetDataTypeToDouble();
  }
  //
  // Resize vtkPoints to fit data
  //
  points->SetNumberOfPoints(nPts);

  //
  // Populate the coordinates.  Put in 3D points with z=0 if the mesh is 2D.
  //
  if (this->DoublePrecisionMesh != 0) // DOUBLE PRECISION MESHPOINTS
  {
    CGNSRead::get_XYZ_mesh<double, float>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points);
  }
  else // SINGLE PRECISION MESHPOINTS
  {
    CGNSRead::get_XYZ_mesh<float, double>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points);
  }

  //----------------------------------------------------------------------------
  // Handle solutions
  //----------------------------------------------------------------------------
  vtkMultiBlockDataSet* mzone = vtkMultiBlockDataSet::New();

  bool createEachSolutionAsBlock = (this->CreateEachSolutionAsBlock != 0); // Debugging mode !
  bool nosolutionread = false;

  if (readSolutionName != true)
  {
    CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);
    double cgioSolId;
    double cgioVarId;

    if (cgio_get_node_id(this->cgioNum, this->currentId, SolutionName, &cgioSolId) != CG_OK)
    {
      nosolutionread = true;
    }
    else
    {
      std::vector<double> solChildId;
      std::size_t nVarArray = 0;

      vtkPrivate::getVarsIdAndFillRind(cgioSolId, nVarArray, varCentering, solChildId, rind, this);

      vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
      sgrid->SetExtent(extent);
      sgrid->SetPoints(points);

      bool skip = false;
      if ((varCentering != CGNS_ENUMV(Vertex)) && (varCentering != CGNS_ENUMV(CellCenter)))
      {
        vtkWarningMacro(<< "Solution " << SolutionName << " centering is not supported\n");
        skip = true;
      }

      if (skip != true)
      {

        vtkDebugMacro(<< "Reading solution :" << SolutionName << "\n");

        std::vector<CGNSRead::CGNSVariable> cgnsVars(nVarArray);
        std::vector<CGNSRead::CGNSVector> cgnsVectors;
        vtkPrivate::fillArrayInformation(solChildId, physicalDim, cgnsVars, cgnsVectors, this);

        // Source
        cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
        cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
        cgsize_t fieldSrcEnd[3];

        // Destination Memory
        cgsize_t fieldMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldMemStride[3] = { 1, 1, 1 };
        cgsize_t fieldMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldMemDims[3] = { 1, 1, 1 };

        vtkIdType nVals = 0;

        // Get solution data range
        int nsc = varCentering == CGNS_ENUMV(Vertex) ? 0 : cellDim;

        for (n = 0; n < cellDim; ++n)
        {
          fieldSrcStart[n] = rind[2 * n] + 1;
          fieldSrcEnd[n] = rind[2 * n] + zsize[n + nsc];
          fieldMemEnd[n] = zsize[n + nsc];
          fieldMemDims[n] = zsize[n + nsc];
        }

        // compute number of field values
        nVals = static_cast<vtkIdType>(fieldMemEnd[0] * fieldMemEnd[1] * fieldMemEnd[2]);
        //
        // VECTORS aliasing ...
        // destination
        cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
        cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

        fieldVectMemStride[0] = static_cast<cgsize_t>(physicalDim);

        fieldVectMemDims[0] = fieldMemDims[0] * fieldVectMemStride[0];
        fieldVectMemDims[1] = fieldMemDims[1];
        fieldVectMemDims[2] = fieldMemDims[2];
        fieldVectMemEnd[0] = fieldMemEnd[0] * fieldVectMemStride[0];
        fieldVectMemEnd[1] = fieldMemEnd[1];
        fieldVectMemEnd[2] = fieldMemEnd[2];

        //
        std::vector<vtkDataArray*> vtkVars(nVarArray);
        // Count number of vars and vectors
        // Assign vars and vectors to a vtkvars array
        vtkPrivate::AllocateVtkArray(
          physicalDim, nVals, varCentering, cgnsVars, cgnsVectors, vtkVars, this);

        // Load Data
        for (std::size_t ff = 0; ff < nVarArray; ++ff)
        {
          // only read allocated fields
          if (vtkVars[ff] == 0)
          {
            continue;
          }
          cgioVarId = solChildId[ff];

          // quick transfer of data because data types is given by cgns database
          if (cgnsVars[ff].isComponent == false)
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldMemDims, fieldMemStart, fieldMemEnd, fieldMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          else
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldVectMemDims, fieldVectMemStart, fieldVectMemEnd, fieldVectMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          cgio_release_id(this->cgioNum, cgioVarId);
        }
        cgio_release_id(this->cgioNum, cgioSolId);
        // Append data to StructuredGrid
        vtkDataSetAttributes* dsa = 0;
        if (varCentering == CGNS_ENUMV(Vertex)) // ON_NODES
        {
          dsa = sgrid->GetPointData();
        }
        if (varCentering == CGNS_ENUMV(CellCenter)) // ON_CELL
        {
          dsa = sgrid->GetCellData();
        }
        // SetData in vtk Structured Zone + Clean Pointers
        for (std::size_t nv = 0; nv < nVarArray; ++nv)
        {
          // only transfer allocated fields
          if (vtkVars[nv] == 0)
          {
            continue;
          }

          int arrIndex(-1);
          if ((cgnsVars[nv].isComponent == false) || (cgnsVars[nv].xyzIndex == 1))
          {
            arrIndex = dsa->AddArray(vtkVars[nv]);
            vtkVars[nv]->Delete();
          }
          if (arrIndex >= 0 && cgnsVars[nv].isComponent && cgnsVars[nv].xyzIndex == 1)
          {
            vtkDataArray* arr = dsa->GetArray(arrIndex);
            dsa->SetVectors(arr);
          }
          vtkVars[nv] = 0;
        }
      }
      //
      vtkPrivate::AttachReferenceValue(base, sgrid, this);
      //
      mbase->SetBlock(zone, sgrid);
      sgrid->Delete();
    }
  }
  else if (!createEachSolutionAsBlock)
  {
    nosolutionread = true;

    //
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    sgrid->SetExtent(extent);
    sgrid->SetPoints(points);
    //
    int requiredSol = 1;
    int cellSolution = 0;
    int pointSolution = 0;

    // Read List of Solution ids
    std::vector<double> zoneChildId;
    CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildId);
    char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
    //
    for (std::size_t nn = 0; nn < zoneChildId.size(); nn++)
    {
      bool skip = false;
      cgio_get_label(cgioNum, zoneChildId[nn], nodeLabel);
      if (strcmp(nodeLabel, "FlowSolution_t") == 0)
      {
        CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);
        double cgioSolId = zoneChildId[nn];
        double cgioVarId;

        std::size_t nVarArray = 0;
        std::vector<double> solChildId;

        cgio_get_name(this->cgioNum, zoneChildId[nn], SolutionName);

        vtkPrivate::getVarsIdAndFillRind(
          cgioSolId, nVarArray, varCentering, solChildId, rind, this);

        if (varCentering != CGNS_ENUMV(Vertex))
        {
          pointSolution++;
          skip = (pointSolution != requiredSol);
        }
        else if (varCentering != CGNS_ENUMV(CellCenter))
        {
          cellSolution++;
          skip = (cellSolution != requiredSol);
        }
        else
        {
          vtkWarningMacro(<< "Solution " << SolutionName << " centering is not supported\n");
          skip = true;
        }

        if (skip)
        {
          cgio_release_id(cgioNum, zoneChildId[nn]);
          continue;
        }

        nosolutionread = false;
        // Read Variables
        vtkDebugMacro(<< "Reading solution :" << SolutionName << "\n");

        std::vector<CGNSRead::CGNSVariable> cgnsVars(nVarArray);
        std::vector<CGNSRead::CGNSVector> cgnsVectors;

        vtkPrivate::fillArrayInformation(solChildId, physicalDim, cgnsVars, cgnsVectors, this);

        // Source
        cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
        cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
        cgsize_t fieldSrcEnd[3];

        // Destination Memory
        cgsize_t fieldMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldMemStride[3] = { 1, 1, 1 };
        cgsize_t fieldMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldMemDims[3] = { 1, 1, 1 };

        vtkIdType nVals = 0;

        // Get solution data range
        int nsc = varCentering == CGNS_ENUMV(Vertex) ? 0 : cellDim;

        for (n = 0; n < cellDim; ++n)
        {
          fieldSrcStart[n] = rind[2 * n] + 1;
          fieldSrcEnd[n] = rind[2 * n] + zsize[n + nsc];
          fieldMemEnd[n] = zsize[n + nsc];
          fieldMemDims[n] = zsize[n + nsc];
        }

        // compute number of field values
        nVals = static_cast<vtkIdType>(fieldMemEnd[0] * fieldMemEnd[1] * fieldMemEnd[2]);
        //
        // VECTORS aliasing ...
        // destination
        cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
        cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

        fieldVectMemStride[0] = static_cast<cgsize_t>(physicalDim);

        fieldVectMemDims[0] = fieldMemDims[0] * fieldVectMemStride[0];
        fieldVectMemDims[1] = fieldMemDims[1];
        fieldVectMemDims[2] = fieldMemDims[2];
        fieldVectMemEnd[0] = fieldMemEnd[0] * fieldVectMemStride[0];
        fieldVectMemEnd[1] = fieldMemEnd[1];
        fieldVectMemEnd[2] = fieldMemEnd[2];

        //
        // Count number of vars and vectors
        // Assign vars and vectors to a vtkvars array
        std::vector<vtkDataArray*> vtkVars(nVarArray);

        vtkPrivate::AllocateVtkArray(
          physicalDim, nVals, varCentering, cgnsVars, cgnsVectors, vtkVars, this);

        // Load Data
        for (std::size_t ff = 0; ff < nVarArray; ++ff)
        {
          // only read allocated fields
          if (vtkVars[ff] == 0)
          {
            continue;
          }
          cgioVarId = solChildId[ff];

          // quick transfer of data because data types is given by cgns database
          if (cgnsVars[ff].isComponent == false)
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldMemDims, fieldMemStart, fieldMemEnd, fieldMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          else
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldVectMemDims, fieldVectMemStart, fieldVectMemEnd, fieldVectMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          cgio_release_id(this->cgioNum, cgioVarId);
        }
        // Append data to StructuredGrid
        vtkDataSetAttributes* dsa = 0;
        if (varCentering == CGNS_ENUMV(Vertex)) // ON_NODES
        {
          dsa = sgrid->GetPointData();
        }
        if (varCentering == CGNS_ENUMV(CellCenter)) // ON_CELL
        {
          dsa = sgrid->GetCellData();
        }
        // SetData in vtk Structured Zone + Clean Pointers
        for (std::size_t nv = 0; nv < nVarArray; ++nv)
        {
          // only transfer allocated fields
          if (vtkVars[nv] == 0)
          {
            continue;
          }

          int arrIndex(-1);
          if ((cgnsVars[nv].isComponent == false) || (cgnsVars[nv].xyzIndex == 1))
          {
            arrIndex = dsa->AddArray(vtkVars[nv]);
            vtkVars[nv]->Delete();
          }
          if (arrIndex >= 0 && cgnsVars[nv].isComponent && cgnsVars[nv].xyzIndex == 1)
          {
            vtkDataArray* arr = dsa->GetArray(arrIndex);
            dsa->SetVectors(arr);
          }
          vtkVars[nv] = 0;
        }
      }
      cgio_release_id(cgioNum, zoneChildId[nn]);
    }
    vtkPrivate::AttachReferenceValue(base, sgrid, this);
    if (nosolutionread == false)
    {
      mbase->SetBlock((zone), sgrid);
      sgrid->Delete();
    }
    else
    {
      sgrid->Delete();
    }
  }
  else if (createEachSolutionAsBlock)
  {
    nosolutionread = true;

    std::vector<vtkStructuredGrid*> StructuredGridList;
    // Read List of Solution ids
    char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
    std::vector<double> zoneChildId;
    CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildId);

    // Hacky to be cleaned off
    CGNSRead::char_33* SolutionNameList = new CGNSRead::char_33[zoneChildId.size()];
    std::size_t ns = 0;
    for (std::size_t nn = 0; nn < zoneChildId.size(); nn++)
    {
      cgio_get_label(cgioNum, zoneChildId[nn], nodeLabel);
      if (strcmp(nodeLabel, "FlowSolution_t") == 0)
      {
        CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);
        double cgioSolId = zoneChildId[nn];
        double cgioVarId;

        cgio_get_name(cgioNum, zoneChildId[nn], SolutionNameList[ns]);

        std::size_t nVarArray = 0;
        std::vector<double> solChildId;

        vtkPrivate::getVarsIdAndFillRind(
          cgioSolId, nVarArray, varCentering, solChildId, rind, this);

        if (varCentering != CGNS_ENUMV(Vertex) && varCentering != CGNS_ENUMV(CellCenter))
        {
          vtkWarningMacro(<< "Solution " << SolutionName << " centering is not supported\n");
          cgio_release_id(cgioNum, zoneChildId[nn]);
          continue;
        }

        ns++;
        vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
        sgrid->SetExtent(extent);
        sgrid->SetPoints(points);
        StructuredGridList.push_back(sgrid);

        nosolutionread = false;

        // Read Variables
        vtkDebugMacro(<< "Reading solution :" << SolutionName << "\n");
        std::vector<CGNSRead::CGNSVariable> cgnsVars(nVarArray);
        std::vector<CGNSRead::CGNSVector> cgnsVectors;

        vtkPrivate::fillArrayInformation(solChildId, physicalDim, cgnsVars, cgnsVectors, this);

        // Source
        cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
        cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
        cgsize_t fieldSrcEnd[3];

        // Destination Memory
        cgsize_t fieldMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldMemStride[3] = { 1, 1, 1 };
        cgsize_t fieldMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldMemDims[3] = { 1, 1, 1 };

        vtkIdType nVals = 0;

        // Get solution data range
        int nsc = varCentering == CGNS_ENUMV(Vertex) ? 0 : cellDim;

        for (n = 0; n < cellDim; ++n)
        {
          fieldSrcStart[n] = rind[2 * n] + 1;
          fieldSrcEnd[n] = rind[2 * n] + zsize[n + nsc];
          fieldMemEnd[n] = zsize[n + nsc];
          fieldMemDims[n] = zsize[n + nsc];
        }

        // compute number of field values
        nVals = static_cast<vtkIdType>(fieldMemEnd[0] * fieldMemEnd[1] * fieldMemEnd[2]);
        //
        // VECTORS aliasing ...
        // destination
        cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
        cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
        cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

        fieldVectMemStride[0] = static_cast<cgsize_t>(physicalDim);

        fieldVectMemDims[0] = fieldMemDims[0] * fieldVectMemStride[0];
        fieldVectMemDims[1] = fieldMemDims[1];
        fieldVectMemDims[2] = fieldMemDims[2];
        fieldVectMemEnd[0] = fieldMemEnd[0] * fieldVectMemStride[0];
        fieldVectMemEnd[1] = fieldMemEnd[1];
        fieldVectMemEnd[2] = fieldMemEnd[2];

        //
        // Count number of vars and vectors
        // Assign vars and vectors to a vtkvars array
        std::vector<vtkDataArray*> vtkVars(nVarArray);

        vtkPrivate::AllocateVtkArray(
          physicalDim, nVals, varCentering, cgnsVars, cgnsVectors, vtkVars, this);

        // Load Data
        for (std::size_t ff = 0; ff < nVarArray; ++ff)
        {
          // only read allocated fields
          if (vtkVars[ff] == 0)
          {
            continue;
          }
          cgioVarId = solChildId[ff];

          // quick transfer of data because data types is given by cgns database
          if (cgnsVars[ff].isComponent == false)
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldMemDims, fieldMemStart, fieldMemEnd, fieldMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          else
          {
            if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
                  cellDim, fieldVectMemDims, fieldVectMemStart, fieldVectMemEnd, fieldVectMemStride,
                  (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
            {
              char message[81];
              cgio_error_message(message);
              vtkErrorMacro(<< "cgio_read_data :" << message);
            }
          }
          cgio_release_id(this->cgioNum, cgioVarId);
        }
        // Append data to StructuredGrid
        vtkDataSetAttributes* dsa = 0;
        if (varCentering == CGNS_ENUMV(Vertex)) // ON_NODES
        {
          dsa = sgrid->GetPointData();
        }
        if (varCentering == CGNS_ENUMV(CellCenter)) // ON_CELL
        {
          dsa = sgrid->GetCellData();
        }
        // SetData in vtk Structured Zone + Clean Pointers
        for (std::size_t nv = 0; nv < nVarArray; ++nv)
        {
          // only transfer allocated fields
          if (vtkVars[nv] == 0)
          {
            continue;
          }

          int arrIndex(-1);
          if ((cgnsVars[nv].isComponent == false) || (cgnsVars[nv].xyzIndex == 1))
          {
            arrIndex = dsa->AddArray(vtkVars[nv]);
            vtkVars[nv]->Delete();
          }
          vtkVars[nv] = 0;

          if (arrIndex >= 0 && cgnsVars[nv].isComponent && cgnsVars[nv].xyzIndex == 1)
          {
            vtkDataArray* arr = dsa->GetArray(arrIndex);
            dsa->SetVectors(arr);
          }
        }
      }
      cgio_release_id(cgioNum, zoneChildId[nn]);
    }
    mzone->SetNumberOfBlocks(StructuredGridList.size());
    for (std::size_t sol = 0; sol < StructuredGridList.size(); sol++)
    {
      mzone->GetMetaData(static_cast<unsigned int>(sol))
        ->Set(vtkCompositeDataSet::NAME(), SolutionNameList[sol]);
      vtkPrivate::AttachReferenceValue(base, StructuredGridList[sol], this);
      mzone->SetBlock((sol), StructuredGridList[sol]);
      StructuredGridList[sol]->Delete();
    }
    delete[] SolutionNameList;
    if (StructuredGridList.size() > 0)
    {
      mbase->SetBlock(zone, mzone);
    }
  }
  else
  {
    nosolutionread = true;
  }

  if (nosolutionread == true)
  {
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    sgrid->SetExtent(extent);
    sgrid->SetPoints(points);
    mbase->SetBlock(zone, sgrid);
    sgrid->Delete();
  }
  points->Delete();
  mzone->Delete();
  return 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::GetUnstructuredZone(
  int base, int zone, int cellDim, int physicalDim, void* v_zsize, vtkMultiBlockDataSet* mbase)
{
  cgsize_t* zsize = reinterpret_cast<cgsize_t*>(v_zsize);

  //========================================================================
  // Test at compilation time with static assert ...
  // In case  cgsize_t < vtkIdType one could try to start from the array end
  // Next line is commented since static_assert requires c++11
  // static_assert(!(sizeof(cgsize_t) > sizeof(vtkIdType)),
  //              "Impossible to load data with sizeof cgsize_t bigger than sizeof vtkIdType\n");
  typedef int static_assert_sizeOfvtkIdType[!(sizeof(cgsize_t) > sizeof(vtkIdType)) ? 1 : -1];
  //=========================================================================
  // TODO Warning at compilation time ??
  const bool warningIdTypeSize = sizeof(cgsize_t) != sizeof(vtkIdType);
  if (warningIdTypeSize == true)
  {
    vtkDebugMacro(<< "Warning cgsize_t do not have same size as vtkIdType\n"
                  << "sizeof vtkIdType = " << sizeof(vtkIdType) << "\n"
                  << "sizeof cgsize_t = " << sizeof(cgsize_t) << "\n");
  }
  //========================================================================

  int rind[6];
  // source layout
  cgsize_t srcStart[3] = { 1, 1, 1 };
  cgsize_t srcStride[3] = { 1, 1, 1 };
  cgsize_t srcEnd[3];

  // memory destination layout
  cgsize_t memStart[3] = { 1, 1, 1 };
  cgsize_t memStride[3] = { 3, 1, 1 };
  cgsize_t memEnd[3] = { 1, 1, 1 };
  cgsize_t memDims[3] = { 1, 1, 1 };

  vtkIdType nPts = 0;

  // Get Coordinates and FlowSolution node names
  bool readGridCoordName = true;
  bool readSolutionName = true;
  CGNSRead::char_33 GridCoordName;
  CGNSRead::char_33 SolutionName;

  std::vector<double> gridChildId;
  std::size_t nCoordsArray = 0;

  vtkPrivate::getGridAndSolutionName(
    base, GridCoordName, SolutionName, readGridCoordName, readSolutionName, this);

  vtkPrivate::getCoordsIdAndFillRind(
    GridCoordName, physicalDim, nCoordsArray, gridChildId, rind, this);

  // Rind was parsed or not then populate dimensions :
  // get grid coordinate range
  srcStart[0] = rind[0] + 1;
  srcEnd[0] = rind[0] + zsize[0];
  memEnd[0] = zsize[0];
  memDims[0] = zsize[0];

  // Compute number of points
  nPts = static_cast<vtkIdType>(zsize[0]);

  // Set up points
  vtkPoints* points = vtkPoints::New();

  //
  // wacky hack ...
  memEnd[0] *= 3; // for memory aliasing
  //
  // vtkPoints assumes float data type
  //
  if (this->DoublePrecisionMesh != 0)
  {
    points->SetDataTypeToDouble();
  }
  //
  // Resize vtkPoints to fit data
  //
  points->SetNumberOfPoints(nPts);

  //
  // Populate the coordinates. Put in 3D points with z=0 if the mesh is 2D.
  //
  if (this->DoublePrecisionMesh != 0) // DOUBLE PRECISION MESHPOINTS
  {
    CGNSRead::get_XYZ_mesh<double, float>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points);
  }
  else // SINGLE PRECISION MESHPOINTS
  {
    CGNSRead::get_XYZ_mesh<float, double>(this->cgioNum, gridChildId, nCoordsArray, cellDim, nPts,
      srcStart, srcEnd, srcStride, memStart, memEnd, memStride, memDims, points);
  }

  this->UpdateProgress(0.2);
  // points are now loaded
  //----------------------
  // Read List of zone children ids
  // and Get connectivities and solutions
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  std::vector<double> zoneChildId;
  CGNSRead::getNodeChildrenId(this->cgioNum, this->currentId, zoneChildId);
  //
  std::vector<double> solIdList;
  std::vector<double> elemIdList;

  for (std::size_t nn = 0; nn < zoneChildId.size(); nn++)
  {
    cgio_get_label(cgioNum, zoneChildId[nn], nodeLabel);
    if (strcmp(nodeLabel, "Elements_t") == 0)
    {
      elemIdList.push_back(zoneChildId[nn]);
    }
    else if (strcmp(nodeLabel, "FlowSolution_t") == 0)
    {
      solIdList.push_back(zoneChildId[nn]);
    }
    else
    {
      cgio_release_id(this->cgioNum, zoneChildId[nn]);
    }
  }

  //---------------------------------------------------------------------
  //  Handle connectivities
  //---------------------------------------------------------------------
  // Read the number of sections, for the zone.
  int nsections = 0;
  nsections = elemIdList.size();

  std::vector<SectionInformation> sectionInfoList(nsections);

  // Find section layout
  // Section is composed of => 1 Volume + bnd surfaces
  //                        => multi-Volume + Bnd surfaces
  // determine dim to allocate for connectivity reading
  cgsize_t elementCoreSize = 0;
  vtkIdType numCoreCells = 0;
  //
  std::vector<int> coreSec;
  std::vector<int> bndSec;
  std::vector<int> sizeSec;
  std::vector<int> startSec;
  //
  numCoreCells = 0; // force initialize
  for (int sec = 0; sec < nsections; ++sec)
  {

    CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
    cgsize_t elementSize = 0;

    //
    sectionInfoList[sec].elemType = CGNS_ENUMV(ElementTypeNull);
    sectionInfoList[sec].range[0] = 1;
    sectionInfoList[sec].range[1] = 1;
    sectionInfoList[sec].bound = 0;
    sectionInfoList[sec].eDataSize = 0;

    //
    CGNSRead::char_33 dataType;
    std::vector<int> mdata;

    if (cgio_get_name(this->cgioNum, elemIdList[sec], sectionInfoList[sec].name) != CG_OK)
    {
      vtkErrorMacro(<< "Error while getting section node name\n");
    }
    if (cgio_get_data_type(cgioNum, elemIdList[sec], dataType) != CG_OK)
    {
      vtkErrorMacro(<< "Error in cgio_get_data_type for section node\n");
    }
    if (strcmp(dataType, "I4") != 0)
    {
      vtkErrorMacro(<< "Unexpected data type for dimension data of Element\n");
    }

    CGNSRead::readNodeData<int>(cgioNum, elemIdList[sec], mdata);
    if (mdata.size() != 2)
    {
      vtkErrorMacro(<< "Unexpected data for Elements_t node\n");
    }
    sectionInfoList[sec].elemType = static_cast<CGNS_ENUMT(ElementType_t)>(mdata[0]);
    sectionInfoList[sec].bound = mdata[1];

    // ElementRange
    double elemRangeId;
    double elemConnectId;
    cgio_get_node_id(this->cgioNum, elemIdList[sec], "ElementRange", &elemRangeId);
    // read node data type
    if (cgio_get_data_type(this->cgioNum, elemRangeId, dataType) != CG_OK)
    {
      std::cerr << "Error in cgio_get_data_type for ElementRange" << std::endl;
      continue;
    }

    if (strcmp(dataType, "I4") == 0)
    {
      std::vector<int> mdata2;
      CGNSRead::readNodeData<int>(this->cgioNum, elemRangeId, mdata2);
      if (mdata2.size() != 2)
      {
        vtkErrorMacro(<< "Unexpected data for ElementRange node\n");
      }
      sectionInfoList[sec].range[0] = static_cast<cgsize_t>(mdata2[0]);
      sectionInfoList[sec].range[1] = static_cast<cgsize_t>(mdata2[1]);
    }
    else if (strcmp(dataType, "I8") == 0)
    {
      std::vector<cglong_t> mdata2;
      CGNSRead::readNodeData<cglong_t>(this->cgioNum, elemRangeId, mdata2);
      if (mdata2.size() != 2)
      {
        vtkErrorMacro(<< "Unexpected data for ElementRange node\n");
      }
      sectionInfoList[sec].range[0] = static_cast<cgsize_t>(mdata2[0]);
      sectionInfoList[sec].range[1] = static_cast<cgsize_t>(mdata2[1]);
    }
    else
    {
      std::cerr << "Unexpected data type for dimension data of Element" << std::endl;
      continue;
    }

    elementSize =
      sectionInfoList[sec].range[1] - sectionInfoList[sec].range[0] + 1; // Interior Volume + Bnd
    elemType = sectionInfoList[sec].elemType;

    cgio_get_node_id(this->cgioNum, elemIdList[sec], "ElementConnectivity", &elemConnectId);
    cgsize_t dimVals[12];
    int ndim;
    if (cgio_get_dimensions(cgioNum, elemConnectId, &ndim, dimVals) != CG_OK)
    {
      cgio_error_exit("cgio_get_dimensions");
      vtkErrorMacro(<< "Could not determine ElementDataSize\n");
      continue;
    }
    if (ndim != 1)
    {
      vtkErrorMacro(<< "ElementConnectivity wrong dimension\n");
      continue;
    }
    sectionInfoList[sec].eDataSize = dimVals[0];

    // Skip if it is a boundary
    if (sectionInfoList[sec].range[0] > zsize[1])
    {
      vtkDebugMacro(<< "@@ Boundary Section not accounted\n");
      bndSec.push_back(sec);
      continue;
    }

    cgsize_t eDataSize = 0;
    eDataSize = dimVals[0];
    if (elemType != CGNS_ENUMV(MIXED))
    {
      eDataSize += elementSize;
    }
    //
    sizeSec.push_back(eDataSize);
    startSec.push_back(sectionInfoList[sec].range[0] - 1);
    elementCoreSize += (eDataSize);
    numCoreCells += elementSize;
    coreSec.push_back(sec);
    //
  }
  //
  std::vector<vtkIdType> startArraySec(coreSec.size());
  for (std::size_t sec = 0; sec < coreSec.size(); sec++)
  {
    int curStart = startSec[sec];
    vtkIdType curArrayStart = 0;
    for (std::size_t lse = 0; lse < coreSec.size(); lse++)
    {
      if (startSec[lse] < curStart)
      {
        curArrayStart += sizeSec[lse];
      }
    }
    startArraySec[sec] = curArrayStart;
  }

  // Create Cell Array
  vtkNew<vtkCellArray> cells;
  // Modification for memory reliability
  vtkNew<vtkIdTypeArray> cellLocations;
  cellLocations->SetNumberOfValues(elementCoreSize);
  vtkIdType* elements = cellLocations->GetPointer(0);

  if (elements == 0)
  {
    vtkErrorMacro(<< "Could not allocate memory for connectivity\n");
    return 1;
  }

  int* cellsTypes = new int[numCoreCells];
  if (cellsTypes == 0)
  {
    vtkErrorMacro(<< "Could not allocate memory for connectivity\n");
    return 1;
  }

  // Set up ugrid - we need to refer to it if we're building an NFACE_n or NGON_n grid
  // Create an unstructured grid to contain the points.
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
  ugrid->SetPoints(points);

  bool buildGrid(true);
  // Iterate over core sections.
  for (std::vector<int>::iterator iter = coreSec.begin(); iter != coreSec.end(); ++iter)
  {
    size_t sec = *iter;
    CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
    cgsize_t start = 1, end = 1;
    cgsize_t elementSize = 0;

    start = sectionInfoList[sec].range[0];
    end = sectionInfoList[sec].range[1];
    elemType = sectionInfoList[sec].elemType;

    elementSize = end - start + 1; // Interior Volume + Bnd

    double cgioSectionId;
    cgioSectionId = elemIdList[sec];

    if (elemType != CGNS_ENUMV(MIXED) && elemType != CGNS_ENUMV(NFACE_n) &&
      elemType != CGNS_ENUMV(NGON_n)) // MDvR: test for NFACE_n, NGON_n too
    {
      // All cells are of the same type.
      int numPointsPerCell = 0;
      int cellType;
      bool higherOrderWarning;
      bool reOrderElements;
      //
      if (cg_npe(elemType, &numPointsPerCell) || numPointsPerCell == 0)
      {
        vtkErrorMacro(<< "Invalid numPointsPerCell\n");
      }

      cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
      //
      for (vtkIdType i = start - 1; i < end; i++)
      {
        cellsTypes[i] = cellType;
      }
      //
      cgsize_t eDataSize = 0;
      cgsize_t EltsEnd = elementSize + start - 1;
      eDataSize = sectionInfoList[sec].eDataSize;
      vtkDebugMacro(<< "Element data size for sec " << sec << " is: " << eDataSize << "\n");

      if (eDataSize != numPointsPerCell * elementSize)
      {
        vtkErrorMacro(<< "FATAL wrong elements dimensions\n");
      }

      // pointer on start !!
      vtkIdType* localElements = &(elements[startArraySec[sec]]);

      cgsize_t memDim[2];
      cgsize_t npe = numPointsPerCell;
      // How to handle per process reading for unstructured mesh
      // + npe* ( wantedstartperprocess-start ) ; startoffset
      srcStart[0] = 1;
      srcStart[1] = 1;

      srcEnd[0] = (EltsEnd - start + 1) * npe;
      srcEnd[1] = 1;
      srcStride[0] = 1;
      srcStride[1] = 1;

      memStart[0] = 2;
      memStart[1] = 1;
      memEnd[0] = npe + 1;
      memEnd[1] = EltsEnd - start + 1;
      memStride[0] = 1;
      memStride[1] = 1;
      memDim[0] = npe + 1;
      memDim[1] = EltsEnd - start + 1;

      memset(localElements, 1, sizeof(vtkIdType) * (npe + 1) * (EltsEnd - start + 1));

      CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 2, srcStart, srcEnd,
        srcStride, memStart, memEnd, memStride, memDim, localElements);

      // Add numptspercell and do -1 on indexes
      for (vtkIdType icell = 0; icell < elementSize; ++icell)
      {
        vtkIdType pos = icell * (numPointsPerCell + 1);
        localElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
        for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
        {
          pos++;
          localElements[pos] = localElements[pos] - 1;
        }
      }
      if (reOrderElements == true)
      {
        CGNSRead::CGNS2VTKorderMonoElem(elementSize, cellType, localElements);
      }
    }
    else if (elemType == CGNS_ENUMV(MIXED))
    {
      //
      int numPointsPerCell = 0;
      int cellType;
      bool higherOrderWarning;
      bool reOrderElements;
      // pointer on start !!
      vtkIdType* localElements = &(elements[startArraySec[sec]]);

      cgsize_t eDataSize = 0;
      eDataSize = sectionInfoList[sec].eDataSize;

      cgsize_t memDim[2];

      srcStart[0] = 1;
      srcEnd[0] = eDataSize;
      srcStride[0] = 1;

      memStart[0] = 1;
      memStart[1] = 1;
      memEnd[0] = eDataSize;
      memEnd[1] = 1;
      memStride[0] = 1;
      memStride[1] = 1;
      memDim[0] = eDataSize;
      memDim[1] = 1;

      CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
        srcStride, memStart, memEnd, memStride, memDim, localElements);

      vtkIdType pos = 0;
      reOrderElements = false;
      for (vtkIdType icell = 0, i = start - 1; icell < elementSize; ++icell, ++i)
      {
        bool orderFlag;
        elemType = static_cast<CGNS_ENUMT(ElementType_t)>(localElements[pos]);
        cg_npe(elemType, &numPointsPerCell);
        cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, orderFlag);
        reOrderElements = reOrderElements | orderFlag;
        cellsTypes[i] = cellType;
        localElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
        pos++;
        for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
        {
          localElements[ip + pos] = localElements[ip + pos] - 1;
        }
        pos += numPointsPerCell;
      }

      if (reOrderElements == true)
      {
        CGNSRead::CGNS2VTKorder(elementSize, &cellsTypes[start - 1], localElements);
      }
    }
    else if (elemType == CGNS_ENUMV(NFACE_n))
    {
      buildGrid = false;

      // the faces are in NGON_n format, and are in another section - or multiple sections!
      std::vector<vtkIdType> faceElements;
      vtkIdType numFaces(0);
      cgsize_t fDataSize(0);
      for (size_t osec = 0, n = 0; osec < sectionInfoList.size(); ++osec)
      {
        // the documentation specifies that the faces of NFACE_n are always in NGON_n
        // format, so look for another section that has that element type.
        if (osec == sec)
          continue; // skip self
        if (sectionInfoList[osec].elemType == CGNS_ENUMV(NGON_n))
        {
          fDataSize = sectionInfoList[osec].eDataSize;
          // resize to fit the next batch of element connectivity values
          faceElements.resize(faceElements.size() + fDataSize);

          numFaces = 1 + sectionInfoList[osec].range[1] - sectionInfoList[osec].range[0];

          cgsize_t memDim[2];

          srcStart[0] = 1;
          srcEnd[0] = fDataSize;
          srcStride[0] = 1;

          memStart[0] = 1;
          memStart[1] = 1;
          memEnd[0] = fDataSize;
          memEnd[1] = 1;
          memStride[0] = 1;
          memStride[1] = 1;
          memDim[0] = fDataSize;
          memDim[1] = 1;

          if (0 != CGNSRead::get_section_connectivity(this->cgioNum, elemIdList[osec], 1, srcStart,
                     srcEnd, srcStride, memStart, memEnd, memStride, memDim, &faceElements[n]))
          {
            vtkErrorMacro(<< "FAILED to read NGON_n cells\n");
            return 1;
          }

          n += fDataSize; // points to next index
        }
      }

      vtkNew<vtkIdTypeArray> cellArray;
      cgsize_t eDataSize(0);
      eDataSize = sectionInfoList[sec].eDataSize;

      cellArray->SetNumberOfValues(eDataSize);
      vtkIdType* cellElements = cellArray->GetPointer(0);
      cgsize_t memDim[2];

      srcStart[0] = 1;
      srcEnd[0] = eDataSize;
      srcStride[0] = 1;

      memStart[0] = 1;
      memStart[1] = 1;
      memEnd[0] = eDataSize;
      memEnd[1] = 1;
      memStride[0] = 1;
      memStride[1] = 1;
      memDim[0] = eDataSize;
      memDim[1] = 1;

      if (0 != CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
                 srcStride, memStart, memEnd, memStride, memDim, cellElements))
      {
        vtkErrorMacro(<< "FAILED to read NFACE_n cells\n");
        return 1;
      }

      // ok, now we have the face-to-node connectivity array and the cell-to-face connectivity
      // array.
      // VTK, however, has no concept of faces, and uses cell-to-node connectivity, so the
      // intermediate faces
      // need to be taken out of the description.

      std::vector<vtkIdType> faceNodeLookupTable(numFaces);

      vtkIdType p(0);
      for (vtkIdType nf = 0; nf < numFaces; ++nf)
      {
        faceNodeLookupTable[nf] = p;
        p += 1 + faceElements[p];
      }

      p = 0;
      for (vtkIdType nc = 0; nc < numCoreCells; ++nc)
      {
        int numCellFaces = cellElements[p];

        vtkNew<vtkIdList> faces;
        faces->InsertNextId(numCellFaces);

        for (vtkIdType nf = 0; nf < numCellFaces; ++nf)
        {
          vtkIdType faceId = cellElements[p + nf + 1];
          bool mustReverse = faceId > 0;
          faceId = std::abs(faceId);

          // the following is needed because when the NGON_n face data preceeds the
          // NFACE_n cell data, the indices are continuous, so a "global-to-local" mapping must be
          // done.
          faceId -= sectionInfoList[sec].range[1];
          faceId -= 1; // CGNS uses FORTRAN ID style, starting at 1

          vtkIdType q = faceNodeLookupTable[faceId];
          vtkIdType numNodes = faceElements[q];
          faces->InsertNextId(numNodes);
          if (mustReverse)
          {
            for (vtkIdType nn = numNodes - 1; nn >= 0; --nn)
            {
              vtkIdType nodeID = faceElements[q + nn + 1] - 1; // AGAIN subtract 1 from node ID
              faces->InsertNextId(nodeID);
            }
          }
          else
          {
            for (vtkIdType nn = 0; nn < numNodes; ++nn)
            {
              vtkIdType nodeID = faceElements[q + nn + 1] - 1; // AGAIN subtract 1 from node ID
              faces->InsertNextId(nodeID);
            }
          }
        }
        ugrid->InsertNextCell(VTK_POLYHEDRON, faces.GetPointer());
        p += 1 + numCellFaces; // p now points to the index of the next cell
      }
    }
    else if (elemType == CGNS_ENUMV(NGON_n))
    {
      buildGrid = false;
      vtkNew<vtkIdTypeArray> ngonFaceArray;
      cgsize_t eDataSize = 0;
      eDataSize = sectionInfoList[sec].eDataSize;

      ngonFaceArray->SetNumberOfValues(eDataSize);
      vtkIdType* localElements = ngonFaceArray->GetPointer(0);
      cgsize_t memDim[2];

      srcStart[0] = 1;
      srcEnd[0] = eDataSize;
      srcStride[0] = 1;

      memStart[0] = 1;
      memStart[1] = 1;
      memEnd[0] = eDataSize;
      memEnd[1] = 1;
      memStride[0] = 1;
      memStride[1] = 1;
      memDim[0] = eDataSize;
      memDim[1] = 1;

      if (0 != CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
                 srcStride, memStart, memEnd, memStride, memDim, localElements))
      {
        vtkErrorMacro(<< "FAILED to read NGON_n cells\n");
        return 1;
      }

      vtkIdType numFaces = 1 + sectionInfoList[sec].range[1] - sectionInfoList[sec].range[0];
      int p(0);
      for (vtkIdType nf = 0; nf < numFaces; ++nf)
      {
        vtkIdType numNodes = localElements[p];
        vtkNew<vtkIdList> nodes;
        // nodes->InsertNextId(numNodes);
        for (vtkIdType nn = 0; nn < numNodes; ++nn)
        {
          vtkIdType nodeId = localElements[p + nn + 1];
          nodeId -= 1; // FORTRAN to C-style indexing
          nodes->InsertNextId(nodeId);
        }
        ugrid->InsertNextCell(VTK_POLYGON, nodes.GetPointer());
        p += 1 + numNodes;
      }
    }

    cgio_release_id(this->cgioNum, cgioSectionId);
  }

  if (buildGrid)
    cells->SetCells(numCoreCells, cellLocations.GetPointer());

  //
  bool requiredPatch = (this->LoadBndPatch != 0);
  // SetUp zone Blocks
  vtkMultiBlockDataSet* mzone = vtkMultiBlockDataSet::New();
  if (bndSec.size() > 0 && requiredPatch)
  {
    mzone->SetNumberOfBlocks(2);
  }
  else
  {
    mzone->SetNumberOfBlocks(1);
  }
  mzone->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), "Internal");

  if (buildGrid)
    ugrid->SetCells(cellsTypes, cells.GetPointer());

  delete[] cellsTypes;

  //----------------------------------------------------------------------------
  // Handle solutions
  //----------------------------------------------------------------------------
  int nsols = 0;

  nsols = solIdList.size();

  if (nsols > 0)
  {
    CGNS_ENUMT(GridLocation_t) varCentering = CGNS_ENUMV(Vertex);

    double cgioSolId;
    double cgioVarId;

    if (readSolutionName == true)
    {
      int requiredsol = 1;
      cgioSolId = solIdList[requiredsol - 1];
    }

    if (readSolutionName != true)
    {
      cgio_get_node_id(this->cgioNum, this->currentId, SolutionName, &cgioSolId);
    }

    std::vector<double> solChildId;
    std::size_t nVarArray = 0;

    vtkPrivate::getVarsIdAndFillRind(cgioSolId, nVarArray, varCentering, solChildId, rind, this);

    if ((varCentering != CGNS_ENUMV(Vertex)) && (varCentering != CGNS_ENUMV(CellCenter)))
    {
      vtkWarningMacro(<< "Solution " << SolutionName << " centering is not supported\n");
    }
    vtkDebugMacro(<< "Reading solution :" << SolutionName << "\n");
    std::vector<CGNSRead::CGNSVariable> cgnsVars(nVarArray);
    std::vector<CGNSRead::CGNSVector> cgnsVectors;

    vtkPrivate::fillArrayInformation(solChildId, physicalDim, cgnsVars, cgnsVectors, this);

    // Source layout
    cgsize_t fieldSrcStart[3] = { 1, 1, 1 };
    cgsize_t fieldSrcStride[3] = { 1, 1, 1 };
    cgsize_t fieldSrcEnd[3];
    //
    const int m_num_dims = 1;
    // Destination memory layout
    cgsize_t fieldMemStart[3] = { 1, 1, 1 };
    cgsize_t fieldMemStride[3] = { 1, 1, 1 };
    cgsize_t fieldMemEnd[3] = { 1, 1, 1 };
    cgsize_t fieldMemDims[3] = { 1, 1, 1 };

    vtkIdType nVals = 0;

    // Get solution data range
    int nsc = varCentering == CGNS_ENUMV(Vertex) ? 0 : 1;
    fieldSrcStart[0] = rind[0] + 1;
    fieldSrcEnd[0] = rind[0] + zsize[nsc];
    fieldMemEnd[0] = zsize[nsc];
    fieldMemDims[0] = zsize[nsc];
    // compute number of elements
    nVals = static_cast<vtkIdType>(fieldMemEnd[0]);
    //
    // VECTORS aliasing ...
    // destination
    cgsize_t fieldVectMemStart[3] = { 1, 1, 1 };
    cgsize_t fieldVectMemStride[3] = { 3, 1, 1 };
    cgsize_t fieldVectMemEnd[3] = { 1, 1, 1 };
    cgsize_t fieldVectMemDims[3] = { 1, 1, 1 };

    fieldVectMemStride[0] = static_cast<cgsize_t>(physicalDim);

    fieldVectMemDims[0] = fieldMemDims[0] * fieldVectMemStride[0];
    fieldVectMemDims[1] = fieldMemDims[1];
    fieldVectMemDims[2] = fieldMemDims[2];
    fieldVectMemEnd[0] = fieldMemEnd[0] * fieldVectMemStride[0];
    fieldVectMemEnd[1] = fieldMemEnd[1];
    fieldVectMemEnd[2] = fieldMemEnd[2];

    //
    //
    // Count number of vars and vectors
    // Assign vars and vectors to a vtkvars array
    std::vector<vtkDataArray*> vtkVars(nVarArray);
    vtkPrivate::AllocateVtkArray(
      physicalDim, nVals, varCentering, cgnsVars, cgnsVectors, vtkVars, this);

    // Load Data
    for (std::size_t ff = 0; ff < nVarArray; ++ff)
    {
      // only read allocated fields
      if (vtkVars[ff] == 0)
      {
        continue;
      }
      cgioVarId = solChildId[ff];

      // quick transfer of data because data types is given by cgns database
      if (cgnsVars[ff].isComponent == false)
      {
        if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
              m_num_dims, fieldMemDims, fieldMemStart, fieldMemEnd, fieldMemStride,
              (void*)vtkVars[ff]->GetVoidPointer(0)) != CG_OK)
        {
          char message[81];
          cgio_error_message(message);
          vtkErrorMacro(<< "cgio_read_data :" << message);
        }
      }
      else
      {
        if (cgio_read_data(this->cgioNum, cgioVarId, fieldSrcStart, fieldSrcEnd, fieldSrcStride,
              m_num_dims, fieldVectMemDims, fieldVectMemStart, fieldVectMemEnd, fieldVectMemStride,
              (void*)vtkVars[ff]->GetVoidPointer(cgnsVars[ff].xyzIndex - 1)) != CG_OK)
        {
          char message[81];
          cgio_error_message(message);
          vtkErrorMacro(<< "cgio_read_data :" << message);
        }
      }
      cgio_release_id(this->cgioNum, cgioVarId);
    }
    // Append data to StructuredGrid
    vtkDataSetAttributes* dsa = 0;
    if (varCentering == CGNS_ENUMV(Vertex)) // ON_NODES
    {
      dsa = ugrid->GetPointData();
    }
    if (varCentering == CGNS_ENUMV(CellCenter)) // ON_CELL
    {
      dsa = ugrid->GetCellData();
    }
    // SetData in vtk Structured Zone + Clean Pointers
    for (std::size_t nv = 0; nv < nVarArray; ++nv)
    {
      // only transfer allocated fields
      if (vtkVars[nv] == 0)
      {
        continue;
      }

      int arrIndex(-1);
      if ((cgnsVars[nv].isComponent == false) || (cgnsVars[nv].xyzIndex == 1))
      {
        arrIndex = dsa->AddArray(vtkVars[nv]);
        vtkVars[nv]->Delete();
      }
      if (arrIndex >= 0 && cgnsVars[nv].isComponent && cgnsVars[nv].xyzIndex == 1)
      {
        vtkDataArray* arr = dsa->GetArray(arrIndex);
        dsa->SetVectors(arr);
      }
      vtkVars[nv] = 0;
    }
  }

  // Handle Reference Values (Mach Number, ...)
  vtkPrivate::AttachReferenceValue(base, ugrid, this);

  //--------------------------------------------------
  // Read patch boundary Sections
  //--------------------------------------------------
  // Iterate over bnd sections.
  vtkIntArray* ugrid_id_arr = vtkIntArray::New();
  ugrid_id_arr->SetNumberOfTuples(1);
  ugrid_id_arr->SetValue(0, 0);
  ugrid_id_arr->SetName("ispatch");
  ugrid->GetFieldData()->AddArray(ugrid_id_arr);
  ugrid_id_arr->Delete();

  if (bndSec.size() > 0 && requiredPatch)
  {
    // mzone Set Blocks
    mzone->SetBlock(0, ugrid);
    ugrid->Delete();
    vtkMultiBlockDataSet* mpatch = vtkMultiBlockDataSet::New();
    mpatch->SetNumberOfBlocks(bndSec.size());

    int bndNum = 0;
    for (std::vector<int>::iterator iter = bndSec.begin(); iter != bndSec.end(); ++iter)
    {
      int sec = *iter;
      CGNS_ENUMT(ElementType_t) elemType = CGNS_ENUMV(ElementTypeNull);
      cgsize_t start = 1;
      cgsize_t end = 1;
      cgsize_t elementSize = 0;

      start = sectionInfoList[sec].range[0];
      end = sectionInfoList[sec].range[1];
      elemType = sectionInfoList[sec].elemType;

      mpatch->GetMetaData(static_cast<unsigned int>(bndNum))
        ->Set(vtkCompositeDataSet::NAME(), sectionInfoList[sec].name);
      elementSize = end - start + 1; // Bnd Volume + Bnd
      if (start < zsize[1])
      {
        vtkErrorMacro(<< "ERROR:: Internal Section\n");
      }

      int* bndCellsTypes = new int[elementSize];
      if (bndCellsTypes == 0)
      {
        vtkErrorMacro(<< "Could not allocate memory for connectivity\n");
        return 1;
      }

      cgsize_t eDataSize = 0;
      cgsize_t EltsEnd = elementSize + start - 1;
      eDataSize = sectionInfoList[sec].eDataSize;

      vtkDebugMacro(<< "Element data size for sec " << sec << " is: " << eDataSize << "\n");
      //
      cgsize_t elementBndSize = 0;
      elementBndSize = eDataSize;
      vtkIdTypeArray* IdBndArray_ptr = vtkIdTypeArray::New();
      vtkIdType* bndElements = NULL;

      double cgioSectionId;
      cgioSectionId = elemIdList[sec];

      if (elemType != CGNS_ENUMV(MIXED))
      {
        // All cells are of the same type.
        int numPointsPerCell = 0;
        int cellType;
        bool higherOrderWarning;
        bool reOrderElements;
        //
        if (cg_npe(elemType, &numPointsPerCell) || numPointsPerCell == 0)
        {
          vtkErrorMacro(<< "Invalid numPointsPerCell\n");
        }

        cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
        //
        for (vtkIdType i = 0; i < elementSize; ++i)
        {
          bndCellsTypes[i] = cellType;
        }
        //
        elementBndSize = (numPointsPerCell + 1) * elementSize;
        IdBndArray_ptr->SetNumberOfValues(elementBndSize);
        bndElements = IdBndArray_ptr->GetPointer(0);
        if (bndElements == 0)
        {
          vtkErrorMacro(<< "Could not allocate memory for bnd connectivity\n");
          return 1;
        }

        if (eDataSize != numPointsPerCell * elementSize)
        {
          vtkErrorMacro(<< "Wrong elements dimensions\n");
        }

        // pointer on start !!
        vtkIdType* locElements = &(bndElements[0]);

        cgsize_t memDim[2];
        cgsize_t npe = numPointsPerCell;

        srcStart[0] = 1;
        srcStart[1] = 1;

        srcEnd[0] = (EltsEnd - start + 1) * npe;
        srcStride[0] = 1;

        memStart[0] = 2;
        memStart[1] = 1;
        memEnd[0] = npe + 1;
        memEnd[1] = EltsEnd - start + 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = npe + 1;
        memDim[1] = EltsEnd - start + 1;

        CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 2, srcStart, srcEnd,
          srcStride, memStart, memEnd, memStride, memDim, locElements);

        // Add numptspercell and do -1 on indexes
        for (vtkIdType icell = 0; icell < elementSize; ++icell)
        {
          vtkIdType pos = icell * (numPointsPerCell + 1);
          locElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
          for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
          {
            pos++;
            locElements[pos] = locElements[pos] - 1;
          }
        }
      }
      else if (elemType == CGNS_ENUMV(MIXED))
      {
        //
        // all cells are of the same type.
        int numPointsPerCell = 0;
        int cellType;
        bool higherOrderWarning;
        bool reOrderElements;
        // pointer on start !!

        IdBndArray_ptr->SetNumberOfValues(elementBndSize);
        bndElements = IdBndArray_ptr->GetPointer(0);

        if (bndElements == 0)
        {
          vtkErrorMacro(<< "Could not allocate memory for bnd connectivity\n");
          return 1;
        }
        //
        vtkIdType* localElements = &(bndElements[0]);

        cgsize_t memDim[2];

        srcStart[0] = 1;
        srcEnd[0] = eDataSize;
        srcStride[0] = 1;

        memStart[0] = 1;
        memStart[1] = 1;
        memEnd[0] = eDataSize;
        memEnd[1] = 1;
        memStride[0] = 1;
        memStride[1] = 1;
        memDim[0] = eDataSize;
        memDim[1] = 1;

        CGNSRead::get_section_connectivity(this->cgioNum, cgioSectionId, 1, srcStart, srcEnd,
          srcStride, memStart, memEnd, memStride, memDim, localElements);
        vtkIdType pos = 0;
        for (vtkIdType icell = 0; icell < elementSize; ++icell)
        {
          elemType = static_cast<CGNS_ENUMT(ElementType_t)>(localElements[pos]);
          cg_npe(elemType, &numPointsPerCell);
          cellType = CGNSRead::GetVTKElemType(elemType, higherOrderWarning, reOrderElements);
          bndCellsTypes[icell] = cellType;
          localElements[pos] = static_cast<vtkIdType>(numPointsPerCell);
          pos++;
          for (vtkIdType ip = 0; ip < numPointsPerCell; ip++)
          {
            localElements[ip + pos] = localElements[ip + pos] - 1;
          }
          pos += numPointsPerCell;
        }
      }

      // Create Cell Array
      vtkCellArray* bndCells = vtkCellArray::New();
      bndCells->SetCells(elementSize, IdBndArray_ptr);
      IdBndArray_ptr->Delete();
      // Set up ugrid
      // Create an unstructured grid to contain the points.
      vtkUnstructuredGrid* bndugrid = vtkUnstructuredGrid::New();
      bndugrid->SetPoints(points);
      bndugrid->SetCells(bndCellsTypes, bndCells);
      bndCells->Delete();
      delete[] bndCellsTypes;

      //
      // Add ispatch 0=false/1=true as field data
      //
      vtkIntArray* bnd_id_arr = vtkIntArray::New();
      bnd_id_arr->SetNumberOfTuples(1);
      bnd_id_arr->SetValue(0, 1);
      bnd_id_arr->SetName("ispatch");
      bndugrid->GetFieldData()->AddArray(bnd_id_arr);
      bnd_id_arr->Delete();

      // Handle Ref Values
      vtkPrivate::AttachReferenceValue(base, bndugrid, this);

      // Copy PointData if exists
      vtkPointData* temp = ugrid->GetPointData();
      if (temp != NULL)
      {
        int NumArray = temp->GetNumberOfArrays();
        for (int i = 0; i < NumArray; ++i)
        {
          vtkDataArray* dataTmp = temp->GetArray(i);
          bndugrid->GetPointData()->AddArray(dataTmp);
        }
      }
      mpatch->SetBlock(bndNum, bndugrid);
      bndugrid->Delete();
      bndNum++;
    }
    mzone->SetBlock(1, mpatch);
    mpatch->Delete();
    mzone->GetMetaData((unsigned int)1)->Set(vtkCompositeDataSet::NAME(), "Patches");
  }
  //
  points->Delete();
  if (bndSec.size() > 0 && requiredPatch)
  {
    mbase->SetBlock(zone, mzone);
  }
  else
  {
    mbase->SetBlock(zone, ugrid);
    ugrid->Delete();
  }
  mzone->Delete();
  return 0;
}

//----------------------------------------------------------------------------
class WithinTolerance : public std::binary_function<double, double, bool>
{
public:
  result_type operator()(first_argument_type a, second_argument_type b) const
  {
    bool result = (std::fabs(a - b) <= (a * 1E-6));
    return (result_type)result;
  }
};

//----------------------------------------------------------------------------
int vtkCGNSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  int ier;
  int nzones;
  int nSelectedBases = 0;
  unsigned int blockIndex = 0;

  int processNumber;
  int numProcessors;
  int startRange, endRange;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the output
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // The whole notion of pieces for this reader is really
  // just a division of zones between processors
  processNumber = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numProcessors = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int numBases = this->Internal->GetNumberOfBaseNodes();
  int numZones = 0;
  for (int bb = 0; bb < numBases; bb++)
  {
    numZones += this->Internal->GetBase(bb).nzones;
  }

  // Divide the files evenly between processors
  int num_zones_per_process = numZones / numProcessors;

  // This if/else logic is for when you don't have a nice even division of files
  // Each process computes which sequence of files it needs to read in
  int left_over_zones = numZones - (num_zones_per_process * numProcessors);
  // base --> startZone,endZone
  std::map<int, duo_t> baseToZoneRange;

  // REDO this part !!!!
  if (processNumber < left_over_zones)
  {
    int accumulated = 0;
    startRange = (num_zones_per_process + 1) * processNumber;
    endRange = startRange + (num_zones_per_process + 1);
    for (int bb = 0; bb < numBases; bb++)
    {
      duo_t zoneRange;
      startRange = startRange - accumulated;
      endRange = endRange - accumulated;
      int startInterZone = std::max(startRange, 0);
      int endInterZone = std::min(endRange, this->Internal->GetBase(bb).nzones);

      if ((endInterZone - startInterZone) > 0)
      {
        zoneRange[0] = startInterZone;
        zoneRange[1] = endInterZone;
      }
      accumulated = this->Internal->GetBase(bb).nzones;
      baseToZoneRange[bb] = zoneRange;
    }
  }
  else
  {
    int accumulated = 0;
    startRange = num_zones_per_process * processNumber + left_over_zones;
    endRange = startRange + num_zones_per_process;
    for (int bb = 0; bb < numBases; bb++)
    {
      duo_t zoneRange;
      startRange = startRange - accumulated;
      endRange = endRange - accumulated;
      int startInterZone = std::max(startRange, 0);
      int endInterZone = std::min(endRange, this->Internal->GetBase(bb).nzones);
      if ((endInterZone - startInterZone) > 0)
      {
        zoneRange[0] = startInterZone;
        zoneRange[1] = endInterZone;
      }
      accumulated = this->Internal->GetBase(bb).nzones;
      baseToZoneRange[bb] = zoneRange;
    }
  }

  // Bnd Sections Not implemented yet for parallel
  if (numProcessors > 1)
  {
    this->LoadBndPatch = 0;
    this->CreateEachSolutionAsBlock = 0;
  }

  if (!this->Internal->Parse(this->FileName))
  {
    return 0;
  }

  vtkMultiBlockDataSet* rootNode = output;

  vtkDebugMacro(<< "Start Loading CGNS data");

  this->UpdateProgress(0.0);
  // Setup Global Time Information
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {

    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // Snapping and clamping of the requested time step to be in bounds
    // can be done by FileSeriesReader class or not ?
    vtkDebugMacro(<< "RequestData: requested time value: " << requestedTimeValue);

    // Clamp requestedTimeValue to available time range.
    if (requestedTimeValue < this->Internal->GetTimes().front())
    {
      requestedTimeValue = this->Internal->GetTimes().front();
    }

    if (requestedTimeValue > this->Internal->GetTimes().back())
    {
      requestedTimeValue = this->Internal->GetTimes().back();
    }

    std::vector<double>::iterator timeIte = std::find_if(this->Internal->GetTimes().begin(),
      this->Internal->GetTimes().end(), std::bind2nd(WithinTolerance(), requestedTimeValue));
    //
    if (timeIte == this->Internal->GetTimes().end())
    {
      return 0;
    }
    requestedTimeValue = *timeIte;
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
  }

  vtkDebugMacro(<< "CGNSReader::RequestData: Reading from file <" << this->FileName << ">...");

  // Openning with cgio layer
  ier = cgio_open_file(this->FileName, CGIO_MODE_READ, 0, &(this->cgioNum));
  if (ier != CG_OK)
  {
    vtkErrorMacro(<< "Error Reading file with cgio");
    return 0;
  }
  cgio_get_root_id(this->cgioNum, &(this->rootId));

  // Get base id list :
  std::vector<double> baseIds;
  ier = CGNSRead::readBaseIds(this->cgioNum, this->rootId, baseIds);
  if (ier != 0)
  {
    vtkErrorMacro(<< "Error Reading Base Ids");
    goto errorData;
  }

  nSelectedBases = this->BaseSelection->GetNumberOfArraysEnabled();
  rootNode->SetNumberOfBlocks(nSelectedBases);
  blockIndex = 0;
  for (int numBase = 0; numBase < baseIds.size(); numBase++)
  {
    int cellDim = 0;
    int physicalDim = 0;

    const CGNSRead::BaseInformation& curBaseInfo = this->Internal->GetBase(numBase);

    // skip unselected base
    if (this->BaseSelection->ArrayIsEnabled(curBaseInfo.name) == 0)
    {
      continue;
    }
    cellDim = curBaseInfo.cellDim;
    physicalDim = curBaseInfo.physicalDim;

    // Get timesteps here !!
    // Operate on Global time scale :
    // clamp requestedTimeValue to available time range
    // if < timemin --> timemin
    // if > timemax --> timemax
    // Then for each base get Index for TimeStep
    // if useFlowSolution read flowSolution and take name with index
    // same for use
    // Setup Global Time Information
    this->ActualTimeStep = 0;
    bool skipBase = false;

    if (outInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {

      // Get the requested time step. We only support requests of a single time
      // step in this reader right now
      double requestedTimeValue = outInfo->Get(vtkDataObject::DATA_TIME_STEP());

      vtkDebugMacro(<< "RequestData: requested time value: " << requestedTimeValue);

      // Check if requestedTimeValue is available in base time range.
      if ((requestedTimeValue < curBaseInfo.times.front()) ||
        (requestedTimeValue > curBaseInfo.times.back()))
      {
        skipBase = true;
        requestedTimeValue = this->Internal->GetTimes().front();
      }

      std::vector<double>::const_iterator iter;
      iter =
        std::upper_bound(curBaseInfo.times.begin(), curBaseInfo.times.end(), requestedTimeValue);

      if (iter == curBaseInfo.times.begin())
      {
        // The requested time step is before any time
        this->ActualTimeStep = 0;
      }
      else
      {
        iter--;
        this->ActualTimeStep = static_cast<int>(iter - curBaseInfo.times.begin());
      }
    }
    if (skipBase == true)
    {
      continue;
    }
    vtkMultiBlockDataSet* mbase = vtkMultiBlockDataSet::New();
    rootNode->GetMetaData(blockIndex)->Set(vtkCompositeDataSet::NAME(), curBaseInfo.name);

    nzones = curBaseInfo.nzones;
    if (nzones == 0)
    {
      vtkWarningMacro(<< "No zones in base " << curBaseInfo.name);
    }
    else
    {
      mbase->SetNumberOfBlocks(nzones);
    }

    std::vector<double> baseChildId;
    CGNSRead::getNodeChildrenId(this->cgioNum, baseIds[numBase], baseChildId);

    std::size_t nz;
    std::size_t nn;
    CGNSRead::char_33 nodeLabel;
    for (nz = 0, nn = 0; nn < baseChildId.size(); ++nn)
    {
      if (cgio_get_label(this->cgioNum, baseChildId[nn], nodeLabel) != CG_OK)
      {
        return false;
      }

      if (strcmp(nodeLabel, "Zone_t") == 0)
      {
        if (nz < nn)
        {
          baseChildId[nz] = baseChildId[nn];
        }
        nz++;
      }
      else
      {
        cgio_release_id(this->cgioNum, baseChildId[nn]);
      }
    }

    int zonemin = baseToZoneRange[numBase][0];
    int zonemax = baseToZoneRange[numBase][1];
    for (int zone = zonemin; zone < zonemax; ++zone)
    {
      CGNSRead::char_33 zoneName;
      cgsize_t zsize[9];
      CGNS_ENUMT(ZoneType_t) zt = CGNS_ENUMV(ZoneTypeNull);
      memset(zoneName, 0, 33);
      memset(zsize, 0, 9 * sizeof(cgsize_t));

      if (cgio_get_name(this->cgioNum, baseChildId[zone], zoneName) != CG_OK)
      {
        char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
        cgio_error_message(errmsg);
        vtkErrorMacro(<< "Problem while reading name of zone number " << zone
                      << ", error : " << errmsg);
        return 1;
      }

      CGNSRead::char_33 dataType;
      if (cgio_get_data_type(this->cgioNum, baseChildId[zone], dataType) != CG_OK)
      {
        char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
        cgio_error_message(errmsg);
        vtkErrorMacro(<< "Problem while reading data_type of zone number " << zone << " "
                      << errmsg);
        return 1;
      }

      if (strcmp(dataType, "I4") == 0)
      {
        std::vector<int> mdata;
        CGNSRead::readNodeData<int>(this->cgioNum, baseChildId[zone], mdata);
        for (std::size_t index = 0; index < mdata.size(); index++)
        {
          zsize[index] = static_cast<cgsize_t>(mdata[index]);
        }
      }
      else if (strcmp(dataType, "I8") == 0)
      {
        std::vector<cglong_t> mdata;
        CGNSRead::readNodeData<cglong_t>(this->cgioNum, baseChildId[zone], mdata);
        for (std::size_t index = 0; index < mdata.size(); index++)
        {
          zsize[index] = static_cast<cgsize_t>(mdata[index]);
        }
      }
      else
      {
        vtkErrorMacro(<< "Problem while reading dimension in zone number " << zone);
        return 1;
      }

      mbase->GetMetaData(zone)->Set(vtkCompositeDataSet::NAME(), zoneName);

      double famId;
      if (CGNSRead::getFirstNodeId(this->cgioNum, baseChildId[zone], "FamilyName_t", &famId) ==
        CG_OK)
      {
        std::string familyName;
        CGNSRead::readNodeStringData(this->cgioNum, famId, familyName);

        vtkInformationStringKey* zonefamily =
          new vtkInformationStringKey("FAMILY", "vtkCompositeDataSet");
        mbase->GetMetaData(zone)->Set(zonefamily, familyName.c_str());
      }

      this->currentId = baseChildId[zone];

      double zoneTypeId;
      zt = CGNS_ENUMV(Structured);
      if (CGNSRead::getFirstNodeId(this->cgioNum, baseChildId[zone], "ZoneType_t", &zoneTypeId) ==
        CG_OK)
      {
        std::string zoneType;
        CGNSRead::readNodeStringData(this->cgioNum, zoneTypeId, zoneType);

        if (zoneType == "Structured")
        {
          zt = CGNS_ENUMV(Structured);
        }
        else if (zoneType == "Unstructured")
        {
          zt = CGNS_ENUMV(Unstructured);
        }
        else if (zoneType == "Null")
        {
          zt = CGNS_ENUMV(ZoneTypeNull);
        }
        else if (zoneType == "UserDefined")
        {
          zt = CGNS_ENUMV(ZoneTypeUserDefined);
        }
      }

      switch (zt)
      {
        case CGNS_ENUMV(ZoneTypeNull):
          break;
        case CGNS_ENUMV(ZoneTypeUserDefined):
          break;
        case CGNS_ENUMV(Structured):
        {
          ier = GetCurvilinearZone(numBase, zone, cellDim, physicalDim, zsize, mbase);
          if (ier != CG_OK)
          {
            vtkErrorMacro(<< "Error Reading file");
            return 0;
          }

          break;
        }
        case CGNS_ENUMV(Unstructured):
          ier = GetUnstructuredZone(numBase, zone, cellDim, physicalDim, zsize, mbase);
          if (ier != CG_OK)
          {
            vtkErrorMacro(<< "Error Reading file");
            return 0;
          }
          break;
      }
      this->UpdateProgress(0.5);
    }
    rootNode->SetBlock(blockIndex, mbase);
    mbase->Delete();
    blockIndex++;
  }

errorData:
  cgio_close_file(this->cgioNum);

  this->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::RequestInformation(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{

  // Setting CAN_HANDLE_PIECE_REQUEST to 1 indicates to the
  // upstream consumer that I can provide the same number of pieces
  // as there are number of processors
  // get the info object
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }

  if (this->ProcRank == 0)
  {
    if (!this->FileName)
    {
      vtkErrorMacro(<< "File name not set\n");
      return 0;
    }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!vtksys::SystemTools::FileExists(this->FileName))
    {
      vtkErrorMacro(<< "Error opening file " << this->FileName);
      return false;
    }

    vtkDebugMacro(<< "CGNSReader::RequestInformation: Parsing file " << this->FileName
                  << " for fields and time steps");

    // Parse the file...
    if (!this->Internal->Parse(this->FileName))
    {
      vtkErrorMacro(<< "Failed to parse cgns file: " << this->FileName);
      return false;
    }
  } // End_ProcRank_0

  if (this->ProcSize > 1)
  {
    this->Broadcast(this->Controller);
  }

  this->NumberOfBases = this->Internal->GetNumberOfBaseNodes();

  // Set up time information
  if (this->Internal->GetTimes().size() != 0)
  {
    std::vector<double> timeSteps(
      this->Internal->GetTimes().begin(), this->Internal->GetTimes().end());

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps.front(),
      static_cast<int>(timeSteps.size()));
    double timeRange[2];
    timeRange[0] = timeSteps.front();
    timeRange[1] = timeSteps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  for (int base = 0; base < this->Internal->GetNumberOfBaseNodes(); ++base)
  {
    const CGNSRead::BaseInformation& curBase = this->Internal->GetBase(base);
    // Fill base names
    if (base == 0 && (!this->BaseSelection->ArrayExists(curBase.name)))
    {
      this->BaseSelection->EnableArray(curBase.name);
    }
    else if (!this->BaseSelection->ArrayExists(curBase.name))
    {
      this->BaseSelection->DisableArray(curBase.name);
    }

    // Fill Variable Vertex/Cell names ... perhaps should be improved
    CGNSRead::vtkCGNSArraySelection::const_iterator iter;
    for (iter = curBase.PointDataArraySelection.begin();
         iter != curBase.PointDataArraySelection.end(); ++iter)
    {
      if (!this->PointDataArraySelection->ArrayExists(iter->first.c_str()))
      {
        this->PointDataArraySelection->DisableArray(iter->first.c_str());
      }
    }
    for (iter = curBase.CellDataArraySelection.begin();
         iter != curBase.CellDataArraySelection.end(); ++iter)
    {
      if (!this->CellDataArraySelection->ArrayExists(iter->first.c_str()))
      {
        this->CellDataArraySelection->DisableArray(iter->first.c_str());
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCGNSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//------------------------------------------------------------------------------
int vtkCGNSReader::CanReadFile(const char* name)
{
  // return value 0: can not read
  // return value 1: can read
  int cgioNum;
  int ierr = 1;
  double rootId;
  double childId;
  float FileVersion = 0.0;
  int intFileVersion = 0;
  char dataType[CGIO_MAX_DATATYPE_LENGTH + 1];
  char errmsg[CGIO_MAX_ERROR_LENGTH + 1];
  int ndim = 0;
  cgsize_t dimVals[12];
  int fileType = CG_FILE_NONE;

  if (cgio_open_file(name, CG_MODE_READ, CG_FILE_NONE, &cgioNum) != CG_OK)
  {
    cgio_error_message(errmsg);
    vtkErrorMacro(<< "vtkCGNSReader::CanReadFile : " << errmsg);
    return 0;
  }

  cgio_get_root_id(cgioNum, &rootId);
  cgio_get_file_type(cgioNum, &fileType);

  if (cgio_get_node_id(cgioNum, rootId, "CGNSLibraryVersion", &childId))
  {
    cgio_error_message(errmsg);
    vtkErrorMacro(<< "vtkCGNSReader::CanReadFile : " << errmsg);
    ierr = 0;
    goto CanReadError;
  }

  if (cgio_get_data_type(cgioNum, childId, dataType))
  {
    vtkErrorMacro(<< "CGNS Version data type");
    ierr = 0;
    goto CanReadError;
  }

  if (cgio_get_dimensions(cgioNum, childId, &ndim, dimVals))
  {
    vtkErrorMacro(<< "cgio_get_dimensions");
    ierr = 0;
    goto CanReadError;
  }

  // check data type
  if (strcmp(dataType, "R4") != 0)
  {
    vtkErrorMacro(<< "Unexpected data type for CGNS-Library-Version=" << dataType);
    ierr = 0;
    goto CanReadError;
  }

  // check data dim
  if ((ndim != 1) || (dimVals[0] != 1))
  {
    vtkDebugMacro(<< "Wrong data dimension for CGNS-Library-Version");
    ierr = 0;
    goto CanReadError;
  }

  // read data
  if (cgio_read_all_data(cgioNum, childId, &FileVersion))
  {
    vtkErrorMacro(<< "read CGNS version number");
    ierr = 0;
    goto CanReadError;
  }

  // Check that the library version is at least as recent as the one used
  //   to create the file being read

  intFileVersion = static_cast<int>(FileVersion * 1000 + 0.5);

  if (intFileVersion > CGNS_VERSION)
  {
    // This code allows reading version newer than the lib,
    // as long as the 1st digit of the versions are equal
    if ((intFileVersion / 1000) > (CGNS_VERSION / 1000))
    {
      vtkErrorMacro(<< "The file " << name << " was written with a more recent version"
                                              "of the CGNS library.  You must update your CGNS"
                                              "library before trying to read this file.");
      ierr = 0;
    }
    // warn only if different in second digit
    if ((intFileVersion / 100) > (CGNS_VERSION / 100))
    {
      vtkWarningMacro(<< "The file being read is more recent"
                         "than the CGNS library used");
    }
  }
  if ((intFileVersion / 10) < 255)
  {
    vtkWarningMacro(<< "The file being read was written with an old version"
                       "of the CGNS library. Please update your file"
                       "to a more recent version.");
  }
  vtkDebugMacro(<< "FileVersion=" << FileVersion << "\n");

CanReadError:
  cgio_close_file(cgioNum);
  return ierr ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkCGNSReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllBases()
{
  this->BaseSelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllBases()
{
  this->BaseSelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfBaseArrays()
{
  return this->BaseSelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetBaseArrayStatus(const char* name)
{
  return this->BaseSelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetBaseArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->BaseSelection->EnableArray(name);
  }
  else
  {
    this->BaseSelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetBaseArrayName(int index)
{
  if (index >= (int)this->NumberOfBases || index < 0)
  {
    return NULL;
  }
  else
  {
    return this->BaseSelection->GetArrayName(index);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetPointArrayName(int index)
{
  if (index >= (int)this->GetNumberOfPointArrays() || index < 0)
  {
    return NULL;
  }
  else
  {
    return this->PointDataArraySelection->GetArrayName(index);
  }
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCGNSReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkCGNSReader::GetCellArrayName(int index)
{
  if (index >= (int)this->GetNumberOfCellArrays() || index < 0)
  {
    return NULL;
  }
  else
  {
    return this->CellDataArraySelection->GetArrayName(index);
  }
}

//----------------------------------------------------------------------------
int vtkCGNSReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSReader::SelectionModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkCGNSReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
void vtkCGNSReader::Broadcast(vtkMultiProcessController* ctrl)
{
  if (ctrl)
  {
    int rank = ctrl->GetLocalProcessId();
    this->Internal->Broadcast(ctrl, rank);
  }
}
