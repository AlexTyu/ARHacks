/*=========================================================================

   Program: ParaView
   Module:    pqNamedObjectPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqNamedObjectPanel.h"

#include "vtkCollection.h"
#include "vtkPVXMLElement.h"

//#include "pq3DWidget.h"
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"

pqNamedObjectPanel::pqNamedObjectPanel(pqProxy* object_proxy, QWidget* p)
  : pqObjectPanel(object_proxy, p)
{
}

pqNamedObjectPanel::~pqNamedObjectPanel()
{
  pqNamedWidgets::unlink(this, this->proxy(), this->propertyManager());
}

void pqNamedObjectPanel::linkServerManagerProperties()
{
  // Don't link properties that form a property group (Look at BUG #7175).
  QStringList exceptions;
  //  QList<pq3DWidget*> widgets = this->findChildren<pq3DWidget*>();
  //  foreach (pq3DWidget* widget, widgets)
  //    {
  //    vtkCollection* elements = vtkCollection::New();
  //    vtkPVXMLElement* widgetHints = widget->getHints();
  //    widgetHints->GetElementsByName("Property", elements);
  //    for (int cc=0; cc < elements->GetNumberOfItems(); ++cc)
  //      {
  //      vtkPVXMLElement* child = vtkPVXMLElement::SafeDownCast(
  //        elements->GetItemAsObject(cc));
  //      if (!child)
  //        {
  //        continue;
  //        }
  //      exceptions.push_back(child->GetAttribute("name"));
  //      }
  //    elements->Delete();
  //    }
  pqNamedWidgets::link(this, this->proxy(), this->propertyManager(), &exceptions);
}
