/*=========================================================================

   Program: ParaView
   Module:    pqSaveScreenshotReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqSaveScreenshotReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqImageUtil.h"
#include "pqPVApplicationCore.h"
#include "pqSaveSnapshotDialog.h"
#include "pqSettings.h"
#include "pqStereoModeHelper.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkImageData.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <QDebug>
#include <QFileInfo>

//-----------------------------------------------------------------------------
pqSaveScreenshotReaction::pqSaveScreenshotReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  bool is_enabled = (activeObjects->activeView() && activeObjects->activeServer());
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::saveScreenshot()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannnot save image. No active view.";
    return;
  }

  pqSaveSnapshotDialog ssDialog(pqCoreUtilities::mainWidget());
  ssDialog.setViewSize(view->getSize());

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  ssDialog.setAllViewsSize(viewManager ? viewManager->clientSize() : view->getSize());
  ssDialog.setEnableSaveAllViews(viewManager != 0);

  if (ssDialog.exec() != QDialog::Accepted)
  {
    return;
  }

  QString lastUsedExt;
  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("extensions/ScreenshotExtension"))
  {
    lastUsedExt = settings->value("extensions/ScreenshotExtension").toString();
  }

  QString filters;
  filters += "PNG image (*.png)";
  filters += ";;BMP image (*.bmp)";
  filters += ";;TIFF image (*.tif)";
  filters += ";;PPM image (*.ppm)";
  filters += ";;JPG image (*.jpg)";
  pqFileDialog file_dialog(
    NULL, pqCoreUtilities::mainWidget(), tr("Save Screenshot:"), QString(), filters);
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName("FileSaveScreenshotDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo = QFileInfo(file);
  lastUsedExt = QString("*.") + fileInfo.suffix();
  settings->setValue("extensions/ScreenshotExtension", lastUsedExt);

  QSize size = ssDialog.viewSize();
  QString palette = ssDialog.palette();

  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().activeServer()->proxyManager();
  vtkSMProxy* colorPalette = pxm->GetProxy("global_properties", "ColorPalette");
  vtkSmartPointer<vtkSMProxy> clone;
  if (colorPalette && !palette.isEmpty())
  {
    // save current property values
    clone.TakeReference(pxm->NewProxy(colorPalette->GetXMLGroup(), colorPalette->GetXMLName()));
    clone->Copy(colorPalette);

    vtkSMProxy* chosenPalette = pxm->NewProxy("palettes", palette.toLatin1().data());
    colorPalette->Copy(chosenPalette);
    chosenPalette->Delete();
  }

  QScopedPointer<pqStereoModeHelper> helper(ssDialog.saveAllViews()
      ? new pqStereoModeHelper(ssDialog.getStereoMode(), view->getServer())
      : new pqStereoModeHelper(ssDialog.getStereoMode(), view));

  pqSaveScreenshotReaction::saveScreenshot(file, size, ssDialog.quality(), ssDialog.saveAllViews());

  // restore color palette.
  if (clone)
  {
    colorPalette->Copy(clone);
    pqApplicationCore::instance()->render();
  }
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::saveScreenshot(
  const QString& filename, const QSize& size, int quality, bool all_views)
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (all_views && viewManager)
  {
    if (!viewManager->writeImage(filename, size.width(), size.height(), quality))
    {
      qCritical() << "Save Image failed.";
    }
  }
  else
  {
    if (pqView* view = pqActiveObjects::instance().activeView())
    {
      if (!view->writeImage(filename, size, quality))
      {
        qCritical() << "Save Image failed.";
      }
    }
    else
    {
      qCritical() << "No active view present. Save screenshot failed.";
    }
  }
}
