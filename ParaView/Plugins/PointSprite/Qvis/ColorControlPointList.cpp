/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

#include <ColorControlPoint.h>
#include <ColorControlPointList.h>
#include <DataNode.h>

namespace pointsprite
{
// Type map format string
const char* ColorControlPointList::TypeMapFormatString = "a*bbbb";

// ****************************************************************************
// Method: ColorControlPointList::ColorControlPointList
//
// Purpose:
//   Constructor for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPointList::ColorControlPointList()
  : AttributeSubject(ColorControlPointList::TypeMapFormatString)
{
  smoothingFlag = true;
  equalSpacingFlag = false;
  discreteFlag = false;
  externalFlag = false;
}

// ****************************************************************************
// Method: ColorControlPointList::ColorControlPointList
//
// Purpose:
//   Copy constructor for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPointList::ColorControlPointList(const ColorControlPointList& obj)
  : AttributeSubject(ColorControlPointList::TypeMapFormatString)
{
  AttributeGroupVector::const_iterator pos;

  // *** Copy the controlPoints field ***
  // Delete the AttributeGroup objects and clear the vector.
  for (pos = controlPoints.begin(); pos != controlPoints.end(); ++pos)
    delete *pos;
  controlPoints.clear();
  if (obj.controlPoints.size() > 0)
    controlPoints.reserve(obj.controlPoints.size());
  // Duplicate the controlPoints from obj.
  for (pos = obj.controlPoints.begin(); pos != obj.controlPoints.end(); ++pos)
  {
    ColorControlPoint* oldColorControlPoint = (ColorControlPoint*)(*pos);
    ColorControlPoint* newColorControlPoint = new ColorControlPoint(*oldColorControlPoint);
    controlPoints.push_back(newColorControlPoint);
  }

  smoothingFlag = obj.smoothingFlag;
  equalSpacingFlag = obj.equalSpacingFlag;
  discreteFlag = obj.discreteFlag;
  externalFlag = obj.externalFlag;

  SelectAll();
}

// ****************************************************************************
// Method: ColorControlPointList::~ColorControlPointList
//
// Purpose:
//   Destructor for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPointList::~ColorControlPointList()
{
  AttributeGroupVector::iterator pos;

  // Destroy the controlPoints field.
  for (pos = controlPoints.begin(); pos != controlPoints.end(); ++pos)
    delete *pos;
}

// ****************************************************************************
// Method: ColorControlPointList::operator =
//
// Purpose:
//   Assignment operator for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPointList& ColorControlPointList::operator=(const ColorControlPointList& obj)
{
  if (this == &obj)
    return *this;
  AttributeGroupVector::const_iterator pos;

  // *** Copy the controlPoints field ***
  // Delete the AttributeGroup objects and clear the vector.
  for (pos = controlPoints.begin(); pos != controlPoints.end(); ++pos)
    delete *pos;
  controlPoints.clear();
  if (obj.controlPoints.size() > 0)
    controlPoints.reserve(obj.controlPoints.size());
  // Duplicate the controlPoints from obj.
  for (pos = obj.controlPoints.begin(); pos != obj.controlPoints.end(); ++pos)
  {
    ColorControlPoint* oldColorControlPoint = (ColorControlPoint*)(*pos);
    ColorControlPoint* newColorControlPoint = new ColorControlPoint(*oldColorControlPoint);
    controlPoints.push_back(newColorControlPoint);
  }

  smoothingFlag = obj.smoothingFlag;
  equalSpacingFlag = obj.equalSpacingFlag;
  discreteFlag = obj.discreteFlag;
  externalFlag = obj.externalFlag;

  SelectAll();
  return *this;
}

// ****************************************************************************
// Method: ColorControlPointList::operator ==
//
// Purpose:
//   Comparison operator == for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPointList::operator==(const ColorControlPointList& obj) const
{
  unsigned int i;

  bool controlPoints_equal = (obj.controlPoints.size() == controlPoints.size());
  for (i = 0; (i < controlPoints.size()) && controlPoints_equal; ++i)
  {
    // Make references to ColorControlPoint from AttributeGroup *.
    const ColorControlPoint& controlPoints1 = *((const ColorControlPoint*)(controlPoints[i]));
    const ColorControlPoint& controlPoints2 = *((const ColorControlPoint*)(obj.controlPoints[i]));
    controlPoints_equal = (controlPoints1 == controlPoints2);
  }

  // Create the return value
  return (controlPoints_equal && (smoothingFlag == obj.smoothingFlag) &&
    (equalSpacingFlag == obj.equalSpacingFlag) && (discreteFlag == obj.discreteFlag) &&
    (externalFlag == obj.externalFlag));
}

// ****************************************************************************
// Method: ColorControlPointList::operator !=
//
// Purpose:
//   Comparison operator != for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPointList::operator!=(const ColorControlPointList& obj) const
{
  return !(this->operator==(obj));
}

// ****************************************************************************
// Method: ColorControlPointList::TypeName
//
// Purpose:
//   Type name method for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

const std::string ColorControlPointList::TypeName() const
{
  return "ColorControlPointList";
}

// ****************************************************************************
// Method: ColorControlPointList::CopyAttributes
//
// Purpose:
//   CopyAttributes method for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPointList::CopyAttributes(const AttributeGroup* atts)
{
  if (TypeName() != atts->TypeName())
    return false;

  // Call assignment operator.
  const ColorControlPointList* tmp = (const ColorControlPointList*)atts;
  *this = *tmp;

  return true;
}

// ****************************************************************************
// Method: ColorControlPointList::CreateCompatible
//
// Purpose:
//   CreateCompatible method for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

AttributeSubject* ColorControlPointList::CreateCompatible(const std::string& tname) const
{
  AttributeSubject* retval = 0;
  if (TypeName() == tname)
    retval = new ColorControlPointList(*this);
  // Other cases could go here too.

  return retval;
}

// ****************************************************************************
// Method: ColorControlPointList::NewInstance
//
// Purpose:
//   NewInstance method for the ColorControlPointList class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

AttributeSubject* ColorControlPointList::NewInstance(bool copy) const
{
  AttributeSubject* retval = 0;
  if (copy)
    retval = new ColorControlPointList(*this);
  else
    retval = new ColorControlPointList;

  return retval;
}

// ****************************************************************************
// Method: ColorControlPointList::SelectAll
//
// Purpose:
//   Selects all attributes.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

void ColorControlPointList::SelectAll()
{
  Select(0, (void*)&controlPoints);
  Select(1, (void*)&smoothingFlag);
  Select(2, (void*)&equalSpacingFlag);
  Select(3, (void*)&discreteFlag);
  Select(4, (void*)&externalFlag);
}

// ****************************************************************************
// Method: ColorControlPointList::CreateSubAttributeGroup
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

AttributeGroup* ColorControlPointList::CreateSubAttributeGroup(int)
{
  return new ColorControlPoint;
}

///////////////////////////////////////////////////////////////////////////////
// Persistence methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPointList::CreateNode
//
// Purpose:
//   This method creates a DataNode representation of the object so it can be saved to a config
//   file.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPointList::CreateNode(DataNode* parentNode, bool completeSave, bool forceAdd)
{
  if (parentNode == 0)
    return false;

  ColorControlPointList defaultObject;
  bool addToParent = false;
  // Create a node for ColorControlPointList.
  DataNode* node = new DataNode("ColorControlPointList");

  if (completeSave || !FieldsEqual(0, &defaultObject))
  {
    addToParent = true;
    for (unsigned int i = 0; i < controlPoints.size(); ++i)
      controlPoints[i]->CreateNode(node, completeSave, true);
  }

  if (completeSave || !FieldsEqual(1, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("smoothingFlag", smoothingFlag));
  }

  if (completeSave || !FieldsEqual(2, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("equalSpacingFlag", equalSpacingFlag));
  }

  if (completeSave || !FieldsEqual(3, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("discreteFlag", discreteFlag));
  }

  if (completeSave || !FieldsEqual(4, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("externalFlag", externalFlag));
  }

  // Add the node to the parent node.
  if (addToParent || forceAdd)
    parentNode->AddNode(node);
  else
    delete node;

  return (addToParent || forceAdd);
}

// ****************************************************************************
// Method: ColorControlPointList::SetFromNode
//
// Purpose:
//   This method sets attributes in this object from values in a DataNode representation of the
//   object.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Wed Jul 23 11:29:47 PDT 2003
//
// Modifications:
//   Brad Whitlock, Tue Oct 21 16:22:31 PST 2003
//   I added support for reading in colors and positions in their compact form.
//
// ****************************************************************************

void ColorControlPointList::SetFromNode(DataNode* parentNode)
{
  if (parentNode == 0)
    return;

  DataNode* searchNode = parentNode->GetNode("ColorControlPointList");
  if (searchNode == 0)
    return;

  DataNode* node;
  DataNode** children;
  // Clear all the ColorControlPoints.
  ClearControlPoints();

  //
  // Try setting the colors from the compact color and position vectors.
  //
  bool colorsAreSet = false;
  DataNode* compactColorNode = searchNode->GetNode("compactColors");
  DataNode* compactPositionNode = searchNode->GetNode("compactPositions");
  if (compactColorNode != 0 && compactPositionNode != 0)
  {
    const unsignedCharVector& colors = compactColorNode->AsUnsignedCharVector();
    const floatVector& positions = compactPositionNode->AsFloatVector();
    unsigned int npts = static_cast<unsigned int>(colors.size() / 4);
    if (npts > static_cast<unsigned int>(positions.size()))
      npts = static_cast<unsigned int>(positions.size());
    if (npts > 0)
    {
      for (unsigned int i = 0; i < npts; ++i)
      {
        int index = i << 2;
        AddControlPoints(ColorControlPoint(
          positions[i], colors[index], colors[index + 1], colors[index + 2], colors[index + 3]));
      }

      colorsAreSet = true;
    }
  }

  if (!colorsAreSet)
  {
    // Go through all of the children and construct a new
    // ColorControlPoint for each one of them.
    children = searchNode->GetChildren();
    for (int i = 0; i < searchNode->GetNumChildren(); ++i)
    {
      if (children[i]->GetKey() == std::string("ColorControlPoint"))
      {
        ColorControlPoint temp;
        temp.SetFromNode(children[i]);
        AddControlPoints(temp);
      }
    }
  }

  if ((node = searchNode->GetNode("smoothingFlag")) != 0)
    SetSmoothingFlag(node->AsBool());
  if ((node = searchNode->GetNode("equalSpacingFlag")) != 0)
    SetEqualSpacingFlag(node->AsBool());
  if ((node = searchNode->GetNode("discreteFlag")) != 0)
    SetDiscreteFlag(node->AsBool());
  if ((node = searchNode->GetNode("externalFlag")) != 0)
    SetExternalFlag(node->AsBool());
}
///////////////////////////////////////////////////////////////////////////////
// Set property methods
///////////////////////////////////////////////////////////////////////////////

void ColorControlPointList::SetSmoothingFlag(bool smoothingFlag_)
{
  smoothingFlag = smoothingFlag_;
  Select(1, (void*)&smoothingFlag);
}

void ColorControlPointList::SetEqualSpacingFlag(bool equalSpacingFlag_)
{
  equalSpacingFlag = equalSpacingFlag_;
  Select(2, (void*)&equalSpacingFlag);
}

void ColorControlPointList::SetDiscreteFlag(bool discreteFlag_)
{
  discreteFlag = discreteFlag_;
  Select(3, (void*)&discreteFlag);
}

void ColorControlPointList::SetExternalFlag(bool externalFlag_)
{
  externalFlag = externalFlag_;
  Select(4, (void*)&externalFlag);
}

///////////////////////////////////////////////////////////////////////////////
// Get property methods
///////////////////////////////////////////////////////////////////////////////

const AttributeGroupVector& ColorControlPointList::GetControlPoints() const
{
  return controlPoints;
}

AttributeGroupVector& ColorControlPointList::GetControlPoints()
{
  return controlPoints;
}

bool ColorControlPointList::GetSmoothingFlag() const
{
  return smoothingFlag;
}

bool ColorControlPointList::GetEqualSpacingFlag() const
{
  return equalSpacingFlag;
}

bool ColorControlPointList::GetDiscreteFlag() const
{
  return discreteFlag;
}

bool ColorControlPointList::GetExternalFlag() const
{
  return externalFlag;
}

///////////////////////////////////////////////////////////////////////////////
// Select property methods
///////////////////////////////////////////////////////////////////////////////

void ColorControlPointList::SelectControlPoints()
{
  Select(0, (void*)&controlPoints);
}

///////////////////////////////////////////////////////////////////////////////
// AttributeGroupVector convenience methods.
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPointList::AddControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

void ColorControlPointList::AddControlPoints(const ColorControlPoint& obj)
{
  ColorControlPoint* newColorControlPoint = new ColorControlPoint(obj);
  controlPoints.push_back(newColorControlPoint);

  // Indicate that things have changed by selecting it.
  Select(0, (void*)&controlPoints);
}

// ****************************************************************************
// Method: ColorControlPointList::ClearControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

void ColorControlPointList::ClearControlPoints()
{
  AttributeGroupVector::iterator pos;

  for (pos = controlPoints.begin(); pos != controlPoints.end(); ++pos)
    delete *pos;
  controlPoints.clear();

  // Indicate that things have changed by selecting the list.
  Select(0, (void*)&controlPoints);
}

// ****************************************************************************
// Method: ColorControlPointList::RemoveControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

void ColorControlPointList::RemoveControlPoints(int index)
{
  AttributeGroupVector::iterator pos = controlPoints.begin();

  // Iterate through the vector "index" times.
  for (int i = 0; i < index; ++i)
    ++pos;

  // If pos is still a valid iterator, remove that element.
  if (pos != controlPoints.end())
  {
    delete *pos;
    controlPoints.erase(pos);
  }

  // Indicate that things have changed by selecting the list.
  Select(0, (void*)&controlPoints);
}

// ****************************************************************************
// Method: ColorControlPointList::GetNumControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

int ColorControlPointList::GetNumControlPoints() const
{
  return static_cast<int>(controlPoints.size());
}

// ****************************************************************************
// Method: ColorControlPointList::GetControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint& ColorControlPointList::GetControlPoints(int i)
{
  return *((ColorControlPoint*)controlPoints[i]);
}

// ****************************************************************************
// Method: ColorControlPointList::GetControlPoints
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

const ColorControlPoint& ColorControlPointList::GetControlPoints(int i) const
{
  return *((ColorControlPoint*)controlPoints[i]);
}

// ****************************************************************************
// Method: ColorControlPointList::operator []
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint& ColorControlPointList::operator[](int i)
{
  return *((ColorControlPoint*)controlPoints[i]);
}

// ****************************************************************************
// Method: ColorControlPointList::operator []
//
// Purpose:
//   This class contains a list of ColorControlPoint objects.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

const ColorControlPoint& ColorControlPointList::operator[](int i) const
{
  return *((ColorControlPoint*)controlPoints[i]);
}

///////////////////////////////////////////////////////////////////////////////
// Keyframing methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPointList::GetFieldName
//
// Purpose:
//   This method returns the name of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

std::string ColorControlPointList::GetFieldName(int index) const
{
  switch (index)
  {
    case 0:
      return "controlPoints";
    case 1:
      return "smoothingFlag";
    case 2:
      return "equalSpacingFlag";
    case 3:
      return "discreteFlag";
    case 4:
      return "externalFlag";
    default:
      return "invalid index";
  }
}

// ****************************************************************************
// Method: ColorControlPointList::GetFieldType
//
// Purpose:
//   This method returns the type of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

AttributeGroup::FieldType ColorControlPointList::GetFieldType(int index) const
{
  switch (index)
  {
    case 0:
      return FieldType_attVector;
    case 1:
      return FieldType_bool;
    case 2:
      return FieldType_bool;
    case 3:
      return FieldType_bool;
    case 4:
      return FieldType_bool;
    default:
      return FieldType_unknown;
  }
}

// ****************************************************************************
// Method: ColorControlPointList::GetFieldTypeName
//
// Purpose:
//   This method returns the name of a field type given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

std::string ColorControlPointList::GetFieldTypeName(int index) const
{
  switch (index)
  {
    case 0:
      return "attVector";
    case 1:
      return "bool";
    case 2:
      return "bool";
    case 3:
      return "bool";
    case 4:
      return "bool";
    default:
      return "invalid index";
  }
}

// ****************************************************************************
// Method: ColorControlPointList::FieldsEqual
//
// Purpose:
//   This method compares two fields and return true if they are equal.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 10:49:52 PDT 2007
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPointList::FieldsEqual(int index_, const AttributeGroup* rhs) const
{
  unsigned int i;

  const ColorControlPointList& obj = *((const ColorControlPointList*)rhs);
  bool retval = false;
  switch (index_)
  {
    case 0:
    { // new scope
      bool controlPoints_equal = (obj.controlPoints.size() == controlPoints.size());
      for (i = 0; (i < controlPoints.size()) && controlPoints_equal; ++i)
      {
        // Make references to ColorControlPoint from AttributeGroup *.
        const ColorControlPoint& controlPoints1 = *((const ColorControlPoint*)(controlPoints[i]));
        const ColorControlPoint& controlPoints2 =
          *((const ColorControlPoint*)(obj.controlPoints[i]));
        controlPoints_equal = (controlPoints1 == controlPoints2);
      }

      retval = controlPoints_equal;
    }
    break;
    case 1:
    { // new scope
      retval = (smoothingFlag == obj.smoothingFlag);
    }
    break;
    case 2:
    { // new scope
      retval = (equalSpacingFlag == obj.equalSpacingFlag);
    }
    break;
    case 3:
    { // new scope
      retval = (discreteFlag == obj.discreteFlag);
    }
    break;
    case 4:
    { // new scope
      retval = (externalFlag == obj.externalFlag);
    }
    break;
    default:
      retval = false;
  }

  return retval;
}

///////////////////////////////////////////////////////////////////////////////
// User-defined methods.
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPointList::GetColors
//
// Purpose:
//   This method samples the color control points and fills an array of
//   colors.
//
// Arguments:
//   rgb     : The array in which to return the sampled colors.
//   ncolors : The number of colors we expect to return in the array.
//
// Programmer: Brad Whitlock
// Creation:   Thu Nov 21 14:58:48 PST 2002
//
// Modifications:
//
//   Hank Childs, Sat Jan 27 12:03:56 PST 2007
//   Fix indexing bug that can come up with huge numbers of colors.  This
//   bug ultimately led to memory overwrites and a crash.
//
// ****************************************************************************

void ColorControlPointList::GetColors(unsigned char* rgb, int ncolors) const
{
  int i, ci, c = 0;
  float* newPts_pos = NULL;
  float* newPts_r = NULL;
  float* newPts_g = NULL;
  float* newPts_b = NULL;

  float* oldPts_pos = NULL;
  float* oldPts_r = NULL;
  float* oldPts_g = NULL;
  float* oldPts_b = NULL;

  float* c1_pos = NULL;
  float* c1_r = NULL;
  float* c1_g = NULL;
  float* c1_b = NULL;

  float* c2_pos = NULL;
  float* c2_r = NULL;
  float* c2_g = NULL;
  float* c2_b = NULL;

  /*******************************************
   * Phase I -- Get some values from the color
   *            control point list and put into
   *            local vars.
   *******************************************/
  int npoints = GetNumControlPoints();
  bool equal = GetEqualSpacingFlag();
  bool smooth = GetSmoothingFlag();
  bool discrete = GetDiscreteFlag();

  /*******************************************
   * Phase II -- Determine the number of
   *             control points needed and
   *             allocate storage.
   ******************************************/
  if (equal || !smooth || discrete)
  {
    oldPts_pos = new float[npoints + 1];
    oldPts_r = new float[npoints + 1];
    oldPts_g = new float[npoints + 1];
    oldPts_b = new float[npoints + 1];
  }
  else
  {
    oldPts_pos = new float[npoints];
    oldPts_r = new float[npoints];
    oldPts_g = new float[npoints];
    oldPts_b = new float[npoints];
  }

  for (i = 0; i < npoints; ++i)
  {
    const ColorControlPoint& cpt = this->operator[](i);
    oldPts_pos[i] = cpt.GetPosition();
    oldPts_r[i] = float(cpt.GetColors()[0]) / 255.;
    oldPts_g[i] = float(cpt.GetColors()[1]) / 255.;
    oldPts_b[i] = float(cpt.GetColors()[2]) / 255.;
  }

  /*******************************************
   * Phase III -- Process the control points.
   ******************************************/
  if (equal || !smooth || discrete)
  {
    ++npoints;
    newPts_pos = new float[npoints];
    newPts_r = new float[npoints];
    newPts_g = new float[npoints];
    newPts_b = new float[npoints];

    if (equal || discrete)
    {
      // Do the equal spacing case.
      for (i = 0; i < npoints; ++i)
      {
        ci = (i < (npoints - 2)) ? i : (npoints - 2);
        newPts_pos[i] = (float)i / (float)(npoints - 1);

        if (!smooth)
        {
          newPts_r[i] = oldPts_r[ci];
          newPts_g[i] = oldPts_g[ci];
          newPts_b[i] = oldPts_b[ci];
        }
        else
        {
          if (i < 1 || i >= npoints - 1)
          {
            newPts_r[i] = oldPts_r[ci];
            newPts_g[i] = oldPts_g[ci];
            newPts_b[i] = oldPts_b[ci];
          }
          else
          {
            newPts_r[i] = (oldPts_r[i] + oldPts_r[i - 1]) * 0.5;
            newPts_g[i] = (oldPts_g[i] + oldPts_g[i - 1]) * 0.5;
            newPts_b[i] = (oldPts_b[i] + oldPts_b[i - 1]) * 0.5;
          }
        }
      } // end for
    }   // end if equal spacing
    else
    {
      // Do non-equal, non-smooth case.
      newPts_pos[0] = oldPts_pos[0];
      newPts_r[0] = oldPts_r[0];
      newPts_g[0] = oldPts_g[0];
      newPts_b[0] = oldPts_b[0];
      for (i = 1; i < npoints - 1; i++)
      {
        newPts_pos[i] = oldPts_pos[i - 1] + ((oldPts_pos[i] - oldPts_pos[i - 1]) * 0.5);
        newPts_r[i] = oldPts_r[i];
        newPts_g[i] = oldPts_g[i];
        newPts_b[i] = oldPts_b[i];
      }
      newPts_pos[npoints - 1] = oldPts_pos[npoints - 2];
      newPts_r[npoints - 1] = oldPts_r[npoints - 2];
      newPts_g[npoints - 1] = oldPts_g[npoints - 2];
      newPts_b[npoints - 1] = oldPts_b[npoints - 2];
    }
    c1_pos = newPts_pos;
    c1_r = newPts_r;
    c1_g = newPts_g;
    c1_b = newPts_b;
  }
  else
  {
    c1_pos = oldPts_pos;
    c1_r = oldPts_r;
    c1_g = oldPts_g;
    c1_b = oldPts_b;
  }

  /********************************************
   * Phase IV -- Determine if we need to post sample
   ********************************************/
  const int sampleMultiple = 5;
  bool postSample = (ncolors < (npoints * sampleMultiple));
  int oldNColors = ncolors;
  unsigned char* dest = rgb;
  if (postSample)
  {
    ncolors = npoints * sampleMultiple;
    dest = new unsigned char[3 * ncolors];
  }

  /********************************************
   * Phase V -- Figure the colors for a row.
   ********************************************/
  c2_pos = c1_pos;
  c2_r = c1_r;
  c2_g = c1_g;
  c2_b = c1_b;

  for (ci = 0; ci < npoints - 1; ci++)
  {
    float delta_r, delta_g, delta_b;
    float r_sum, g_sum, b_sum;
    int color_start_i, color_end_i, color_range;

    // Initialize some variables.
    c2_pos++;
    c2_r++;
    c2_g++;
    c2_b++;
    color_start_i = int(c1_pos[0] * float(ncolors));
    color_end_i = int(c2_pos[0] * float(ncolors));
    color_range = color_end_i - color_start_i;

    if (color_range > 1)
    {
      if (ci == 0 && color_start_i != 0)
      {
        for (i = 0; i < color_start_i; i++)
        {
          dest[3 * i + 0] = (unsigned char)(c1_r[0] * 255);
          dest[3 * i + 1] = (unsigned char)(c1_g[0] * 255);
          dest[3 * i + 2] = (unsigned char)(c1_b[0] * 255);
        }
      }

      // Figure out some deltas.
      if (smooth)
      {
        delta_r = (float)(c2_r[0] - c1_r[0]) / (float)(color_range - 1);
        delta_g = (float)(c2_g[0] - c1_g[0]) / (float)(color_range - 1);
        delta_b = (float)(c2_b[0] - c1_b[0]) / (float)(color_range - 1);
      }
      else
        delta_r = delta_g = delta_b = 0.;

      // Initialize sums.
      r_sum = c1_r[0];
      g_sum = c1_g[0];
      b_sum = c1_b[0];

      // Interpolate color1 to color2.
      for (i = color_start_i; i < color_end_i; i++)
      {
        // Store the colors as 24 bit rgb.
        dest[3 * i + 0] = (unsigned char)(r_sum * 255);
        dest[3 * i + 1] = (unsigned char)(g_sum * 255);
        dest[3 * i + 2] = (unsigned char)(b_sum * 255);

        // Add the color deltas.
        r_sum += delta_r;
        g_sum += delta_g;
        b_sum += delta_b;
      }

      if (ci == npoints - 2 && color_end_i != ncolors)
      {
        for (i = color_end_i; i < ncolors; i++)
        {
          dest[3 * i + 0] = (unsigned char)(c2_r[0] * 255);
          dest[3 * i + 1] = (unsigned char)(c2_g[0] * 255);
          dest[3 * i + 2] = (unsigned char)(c2_b[0] * 255);
        }
      }
    }
    else
    {
      dest[3 * color_start_i + 0] = (unsigned char)(c1_r[0] * 255);
      dest[3 * color_start_i + 1] = (unsigned char)(c1_g[0] * 255);
      dest[3 * color_start_i + 2] = (unsigned char)(c1_b[0] * 255);
    }

    c1_pos++;
    c1_r++;
    c1_g++;
    c1_b++;
  }

  /********************************************
   * Phase V -- Postsample colors if needed.
   ********************************************/
  if (postSample)
  {
    c = 0;
    for (i = 0; i < oldNColors; ++i)
    {
      float t;
      if (oldNColors > 1)
        t = float(i) / float(oldNColors - 1);
      else
        t = 0.f;
      int index = int(t * (ncolors - 1));
      rgb[c++] = dest[index * 3];
      rgb[c++] = dest[index * 3 + 1];
      rgb[c++] = dest[index * 3 + 2];
    }

    delete[] dest;
  }

  // Free unneeded memory.
  delete[] oldPts_pos;
  delete[] oldPts_r;
  delete[] oldPts_g;
  delete[] oldPts_b;

  delete[] newPts_pos;
  delete[] newPts_r;
  delete[] newPts_g;
  delete[] newPts_b;
}

// ****************************************************************************
// Method: ColorControlPointList::CompactCreateNode
//
// Purpose:
//   This method creates a compact DataNode representation of the object so
//   it can be saved to a config file.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: Brad Whitlock
// Creation:   Tue Oct 21 16:13:44 PST 2003
//
// Modifications:
//   Brad Whitlock, Wed Dec 17 12:23:35 PDT 2003
//   I added the completeSave argument.
//
// ****************************************************************************

bool ColorControlPointList::CompactCreateNode(
  DataNode* parentNode, bool completeSave, bool forceAdd)
{
  if (parentNode == 0)
    return false;

  ColorControlPointList defaultObject;
  bool addToParent = false;
  // Create a node for ColorControlPointList.
  DataNode* node = new DataNode("ColorControlPointList");

  //
  // Save out the color control points as vectors of uchar and float.
  //
  int npts = GetNumControlPoints();
  if (npts > 0)
  {
    unsignedCharVector compactColors;
    floatVector compactPositions;
    compactColors.reserve(npts << 2);
    compactPositions.reserve(npts);
    for (int i = 0; i < npts; ++i)
    {
      const ColorControlPoint& cpt = GetControlPoints(i);
      const unsigned char* c = cpt.GetColors();
      compactColors.push_back(c[0]);
      compactColors.push_back(c[1]);
      compactColors.push_back(c[2]);
      compactColors.push_back(c[3]);
      compactPositions.push_back(cpt.GetPosition());
    }

    node->AddNode(new DataNode("compactColors", compactColors));
    node->AddNode(new DataNode("compactPositions", compactPositions));
    addToParent = true;
  }

  if (completeSave || !FieldsEqual(1, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("smoothingFlag", smoothingFlag));
  }

  if (completeSave || !FieldsEqual(2, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("equalSpacingFlag", equalSpacingFlag));
  }

  if (completeSave || !FieldsEqual(3, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("discreteFlag", discreteFlag));
  }

  if (completeSave || !FieldsEqual(4, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("externalFlag", externalFlag));
  }

  // Add the node to the parent node.
  if (addToParent || forceAdd)
    parentNode->AddNode(node);
  else
    delete node;

  return (addToParent || forceAdd);
}
}
