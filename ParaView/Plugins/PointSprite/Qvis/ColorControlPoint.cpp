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
#include <DataNode.h>

namespace pointsprite
{

// ****************************************************************************
// Method: ColorControlPoint::ColorControlPoint
//
// Purpose:
//   Constructor for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint::ColorControlPoint()
  : AttributeSubject("Uf")
{
  colors[0] = 0;
  colors[1] = 0;
  colors[2] = 0;
  colors[3] = 255;
  position = 0;
}

// ****************************************************************************
// Method: ColorControlPoint::ColorControlPoint
//
// Purpose:
//   Copy constructor for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint::ColorControlPoint(const ColorControlPoint& obj)
  : AttributeSubject("Uf")
{
  int i;

  for (i = 0; i < 4; ++i)
    colors[i] = obj.colors[i];

  position = obj.position;

  SelectAll();
}

// ****************************************************************************
// Method: ColorControlPoint::~ColorControlPoint
//
// Purpose:
//   Destructor for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint::~ColorControlPoint()
{
  // nothing here
}

// ****************************************************************************
// Method: ColorControlPoint::operator =
//
// Purpose:
//   Assignment operator for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

void ColorControlPoint::operator=(const ColorControlPoint& obj)
{
  int i;

  for (i = 0; i < 4; ++i)
    colors[i] = obj.colors[i];

  position = obj.position;

  SelectAll();
}

// ****************************************************************************
// Method: ColorControlPoint::operator ==
//
// Purpose:
//   Comparison operator == for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPoint::operator==(const ColorControlPoint& obj) const
{
  int i;

  // Compare the colors arrays.
  bool colors_equal = true;
  for (i = 0; i < 4 && colors_equal; ++i)
    colors_equal = (colors[i] == obj.colors[i]);

  // Create the return value
  return (colors_equal && (position == obj.position));
}

// ****************************************************************************
// Method: ColorControlPoint::operator !=
//
// Purpose:
//   Comparison operator != for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPoint::operator!=(const ColorControlPoint& obj) const
{
  return !(this->operator==(obj));
}

// ****************************************************************************
// Method: ColorControlPoint::TypeName
//
// Purpose:
//   Type name method for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

const std::string ColorControlPoint::TypeName() const
{
  return "ColorControlPoint";
}

// ****************************************************************************
// Method: ColorControlPoint::CopyAttributes
//
// Purpose:
//   CopyAttributes method for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPoint::CopyAttributes(const AttributeGroup* atts)
{
  if (TypeName() != atts->TypeName())
    return false;

  // Call assignment operator.
  const ColorControlPoint* tmp = (const ColorControlPoint*)atts;
  *this = *tmp;

  return true;
}

// ****************************************************************************
// Method: ColorControlPoint::CreateCompatible
//
// Purpose:
//   CreateCompatible method for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

AttributeSubject* ColorControlPoint::CreateCompatible(const std::string& tname) const
{
  AttributeSubject* retval = 0;
  if (TypeName() == tname)
    retval = new ColorControlPoint(*this);
  // Other cases could go here too.

  return retval;
}

// ****************************************************************************
// Method: ColorControlPoint::NewInstance
//
// Purpose:
//   NewInstance method for the ColorControlPoint class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

AttributeSubject* ColorControlPoint::NewInstance(bool copy) const
{
  AttributeSubject* retval = 0;
  if (copy)
    retval = new ColorControlPoint(*this);
  else
    retval = new ColorControlPoint;

  return retval;
}

// ****************************************************************************
// Method: ColorControlPoint::SelectAll
//
// Purpose:
//   Selects all attributes.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

void ColorControlPoint::SelectAll()
{
  Select(0, (void*)colors, 4);
  Select(1, (void*)&position);
}

///////////////////////////////////////////////////////////////////////////////
// Persistence methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPoint::CreateNode
//
// Purpose:
//   This method creates a DataNode representation of the object so it can be saved to a config
//   file.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPoint::CreateNode(DataNode* parentNode, bool completeSave, bool forceAdd)
{
  if (parentNode == 0)
    return false;

  ColorControlPoint defaultObject;
  bool addToParent = false;
  // Create a node for ColorControlPoint.
  DataNode* node = new DataNode("ColorControlPoint");

  if (completeSave || !FieldsEqual(0, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("colors", colors, 4));
  }

  if (completeSave || !FieldsEqual(1, &defaultObject))
  {
    addToParent = true;
    node->AddNode(new DataNode("position", position));
  }

  // Add the node to the parent node.
  if (addToParent || forceAdd)
    parentNode->AddNode(node);
  else
    delete node;

  return (addToParent || forceAdd);
}

// ****************************************************************************
// Method: ColorControlPoint::SetFromNode
//
// Purpose:
//   This method sets attributes in this object from values in a DataNode representation of the
//   object.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

void ColorControlPoint::SetFromNode(DataNode* parentNode)
{
  // int i;
  if (parentNode == 0)
    return;

  DataNode* searchNode = parentNode->GetNode("ColorControlPoint");
  if (searchNode == 0)
    return;

  DataNode* node;
  if ((node = searchNode->GetNode("colors")) != 0)
    SetColors(node->AsUnsignedCharArray());
  if ((node = searchNode->GetNode("position")) != 0)
    SetPosition(node->AsFloat());
}

///////////////////////////////////////////////////////////////////////////////
// Set property methods
///////////////////////////////////////////////////////////////////////////////

void ColorControlPoint::SetColors(const unsigned char* colors_)
{
  colors[0] = colors_[0];
  colors[1] = colors_[1];
  colors[2] = colors_[2];
  colors[3] = colors_[3];
  Select(0, (void*)colors, 4);
}

void ColorControlPoint::SetPosition(float position_)
{
  position = position_;
  Select(1, (void*)&position);
}

///////////////////////////////////////////////////////////////////////////////
// Get property methods
///////////////////////////////////////////////////////////////////////////////

const unsigned char* ColorControlPoint::GetColors() const
{
  return colors;
}

unsigned char* ColorControlPoint::GetColors()
{
  return colors;
}

float ColorControlPoint::GetPosition() const
{
  return position;
}

///////////////////////////////////////////////////////////////////////////////
// Select property methods
///////////////////////////////////////////////////////////////////////////////

void ColorControlPoint::SelectColors()
{
  Select(0, (void*)colors, 4);
}

///////////////////////////////////////////////////////////////////////////////
// Keyframing methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: ColorControlPoint::GetFieldName
//
// Purpose:
//   This method returns the name of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

std::string ColorControlPoint::GetFieldName(int index) const
{
  switch (index)
  {
    case 0:
      return "colors";
    case 1:
      return "position";
    default:
      return "invalid index";
  }
}

// ****************************************************************************
// Method: ColorControlPoint::GetFieldType
//
// Purpose:
//   This method returns the type of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

AttributeGroup::FieldType ColorControlPoint::GetFieldType(int index) const
{
  switch (index)
  {
    case 0:
      return FieldType_ucharArray;
    case 1:
      return FieldType_float;
    default:
      return FieldType_unknown;
  }
}

// ****************************************************************************
// Method: ColorControlPoint::GetFieldTypeName
//
// Purpose:
//   This method returns the name of a field type given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

std::string ColorControlPoint::GetFieldTypeName(int index) const
{
  switch (index)
  {
    case 0:
      return "ucharArray";
    case 1:
      return "float";
    default:
      return "invalid index";
  }
}

// ****************************************************************************
// Method: ColorControlPoint::FieldsEqual
//
// Purpose:
//   This method compares two fields and return true if they are equal.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Thu Dec 18 11:24:01 PDT 2003
//
// Modifications:
//
// ****************************************************************************

bool ColorControlPoint::FieldsEqual(int index_, const AttributeGroup* rhs) const
{
  int i;

  const ColorControlPoint& obj = *((const ColorControlPoint*)rhs);
  bool retval = false;
  switch (index_)
  {
    case 0:
    { // new scope
      // Compare the colors arrays.
      bool colors_equal = true;
      for (i = 0; i < 4 && colors_equal; ++i)
        colors_equal = (colors[i] == obj.colors[i]);

      retval = colors_equal;
    }
    break;
    case 1:
    { // new scope
      retval = (position == obj.position);
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
// Method: ColorControlPoint::ColorControlPoint
//
// Purpose:
//   Constructor for the ColorControlPoint class.
//
// Programmer: Brad Whitlock
// Creation:   Tue Jun 12 16:51:24 PST 2001
//
// Modifications:
//
// ****************************************************************************

ColorControlPoint::ColorControlPoint(
  float pos, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
  : AttributeSubject("Uf")
{
  position = pos;
  colors[0] = r;
  colors[1] = g;
  colors[2] = b;
  colors[3] = a;
}
}