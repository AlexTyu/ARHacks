/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVFileInformation
 * @brief   Information object that can
 * be used to obtain information about a file/directory.
 *
 * vtkPVFileInformation can be used to collect information about file
 * or directory. vtkPVFileInformation can collect information
 * from a vtkPVFileInformationHelper object alone.
 * @sa
 * vtkPVFileInformationHelper
*/

#ifndef vtkPVFileInformation_h
#define vtkPVFileInformation_h

#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkCollection;
class vtkPVFileInformationSet;
class vtkFileSequenceParser;

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkPVFileInformation : public vtkPVInformation
{
public:
  static vtkPVFileInformation* New();
  vtkTypeMacro(vtkPVFileInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Transfer information about a single object into this object.
   * The object must be a vtkPVFileInformationHelper.
   */
  virtual void CopyFromObject(vtkObject* object);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //@}

  enum FileTypes
  {
    INVALID = 0,
    SINGLE_FILE,
    SINGLE_FILE_LINK,
    DIRECTORY,
    DIRECTORY_LINK,
    FILE_GROUP,
    DRIVE,
    NETWORK_ROOT,
    NETWORK_DOMAIN,
    NETWORK_SERVER,
    NETWORK_SHARE
  };

  /**
   * Helper that returns whether a FileType is a
   * directory (DIRECTORY, DRIVE, NETWORK_ROOT, etc...)
   * Or in other words, a type that we can do a DirectoryListing on.
   */
  static bool IsDirectory(int t);

  /**
   * Initializes the information object.
   */
  void Initialize();

  //@{
  /**
   * Get the name of the file/directory whose information is
   * represented by this object.
   */
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Get the full path of the file/directory whose information is
   * represented by this object.
   */
  vtkGetStringMacro(FullPath);
  //@}

  //@{
  /**
   * Get the type of this file object.
   */
  vtkGetMacro(Type, int);
  //@}

  //@{
  /**
   * Get the state of the hidden flag for the file/directory.
   */
  vtkGetMacro(Hidden, bool);
  //@}

  //@{
  /**
   * Get the Contents for this directory.
   * Returns a collection with vtkPVFileInformation objects
   * for the contents of this directory if Type = DIRECTORY
   * or the contents of this file group if Type ==FILE_GROUP.
   */
  vtkGetObjectMacro(Contents, vtkCollection);
  vtkGetStringMacro(Extension);
  vtkGetMacro(Size, long long);
  vtkGetMacro(ModificationTime, time_t);
  //@}

protected:
  vtkPVFileInformation();
  ~vtkPVFileInformation();

  vtkCollection* Contents;
  vtkFileSequenceParser* SequenceParser;

  char* Name;              // Name of this file/directory.
  char* FullPath;          // Full path for this file/directory.
  int Type;                // Type i.e. File/Directory/FileGroup.
  bool Hidden;             // If file/directory is hidden
  char* Extension;         // File extension
  long long Size;          // File size
  time_t ModificationTime; // File modification time

  vtkSetStringMacro(Extension);
  vtkSetStringMacro(Name);
  vtkSetStringMacro(FullPath);

  void GetWindowsDirectoryListing();
  void GetDirectoryListing();

  // Goes thru the collection of vtkPVFileInformation objects
  // are creates file groups, if possible.
  void OrganizeCollection(vtkPVFileInformationSet& vector);

  bool DetectType();
  void GetSpecialDirectories();
  void SetHiddenFlag();
  int FastFileTypeDetection;

private:
  vtkPVFileInformation(const vtkPVFileInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVFileInformation&) VTK_DELETE_FUNCTION;

  struct vtkInfo;
};

#endif
