/*
 *
 *  Copyright (C) 2001-2007, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  dcmdata
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Class for conversion of image file into DICOM SC Image Storage
 *
 *  Last Update:      $Author: onken $
 *  Update Date:      $Date: 2008-01-11 14:17:53 $
 *  CVS/RCS Revision: $Revision: 1.2 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef I2DPLVLP_H
#define I2DPLVLP_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/i2dlib/i2doutpl.h"

class I2DOutputPlugVLP : public I2DOutputPlug
{

public:

  /** Constructor, initializes member variables with standard values
   *  @param argc none
   *  @return none
   */
  I2DOutputPlugVLP();

  /** Virtual function that returns a short name of the plugin.
   *  @param none
   *  @return The name of the plugin
   */
  virtual OFString ident();

  /** Overwrites function from base class. Returns the Storage SOP class
   *  written by this plugin
   *  @param none
   *  @return A string holding the Storage SOP class written by this plugin
   */
  virtual void supportedSOPClassUIDs(OFList<OFString> suppSOPs);

  /** Outputs SOP class specific information into dataset
   * @param dset - [in/out] Dataset to write to
   * @return EC_Normal if successful, error otherwise
   */
  virtual OFCondition convert(DcmDataset &dataset) const;

  /** Do some completeness / validity checks. Should be called when
   *  dataset is completed and is about to be saved.
   *  @param dataset - [in] The dataset to check
   *  @param inventMissingType2Attribs - [in] If set, missing type attributes
   *         are added automatically.
   *  @return Error string if error occurs, empty string else
   */
  virtual OFString isValid(DcmDataset& dataset) const;

  /** Virtual Destructor, clean up memory
   *  @param none
   *  @return none
   */
  virtual ~I2DOutputPlugVLP();

};

#endif // I2DPLVLP_H

/*
 * CVS/RCS Log:
 * $Log: i2dplvlp.h,v $
 * Revision 1.2  2008-01-11 14:17:53  onken
 * Added various options to i2dlib. Changed logging to use a configurable
 * logstream. Added output plugin for the new Multiframe Secondary Capture SOP
 * Classes. Added mode for JPEG plugin to copy exsiting APPn markers (except
 * JFIF). Changed img2dcm default behaviour to invent type1/type2 attributes (no
 * need for templates any more). Added some bug fixes.
 *
 * Revision 1.1  2007/11/08 15:58:56  onken
 * Initial checkin of img2dcm application and corresponding library i2dlib.
 *
 *
 */