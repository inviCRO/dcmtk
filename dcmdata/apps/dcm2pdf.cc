/*
 *
 *  Copyright (C) 2005-2007, OFFIS
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
 *  Author:  Marco Eichelberg
 *
 *  Purpose: Exctract PDF file from DICOM encapsulated PDF storage object
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2007-07-11 09:10:29 $
 *  CVS/RCS Revision: $Revision: 1.1 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

BEGIN_EXTERN_C
#ifdef HAVE_FCNTL_H
#include <fcntl.h>       /* for O_RDONLY */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>   /* required for sys/stat.h */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>    /* for stat, fstat */
#endif
END_EXTERN_C

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/dcistrmz.h"    /* for dcmZlibExpectRFC1950Encoding */

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcm2pdf"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define FILENAME_PLACEHOLDER "#f"

static OFString replaceChars( const OFString &srcstr, const OFString &pattern, const OFString &substitute )
    /*
     * This function replaces all occurrences of pattern in srcstr with substitute and returns
     * the result as a new OFString variable. Note that srcstr itself will not be changed,
     *
     * Parameters:
     *   srcstr     - [in] The source string.
     *   pattern    - [in] The pattern string which shall be substituted.
     *   substitute - [in] The substitute for pattern in srcstr.
     */
{
  OFString result = srcstr;
  size_t pos = 0;

  while( pos != OFString_npos )
  {
    pos = result.find( pattern, pos );

    if( pos != OFString_npos )
    {
      result.replace( pos, pattern.size(), substitute );
      pos += substitute.size();
    }
  }

  return( result );
}


#define SHORTCOL 3
#define LONGCOL 19

int main(int argc, char *argv[])
{
  SetDebugLevel(( 0 ));

  const char *opt_ifname = NULL;
  const char *opt_ofname = NULL;
  int opt_debugMode = 0;
  OFBool opt_verbose = OFFalse;
  const char *   opt_execString = NULL;
  E_FileReadMode opt_readMode = ERM_autoDetect;
  E_TransferSyntax opt_ixfer = EXS_Unknown;

  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Exctract PDF file from DICOM encapsulated PDF", rcsid);
  OFCommandLine cmd;
  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

  cmd.addParam("dcmfile-in",  "DICOM input filename");
  cmd.addParam("pdffile-out", "PDF output filename");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
   cmd.addOption("--help",                 "-h",     "print this help text and exit", OFCommandLine::AF_Exclusive);
   cmd.addOption("--version",                        "print version information and exit", OFCommandLine::AF_Exclusive);
   cmd.addOption("--verbose",              "-v",     "verbose mode, print processing details");
   cmd.addOption("--debug",                "-d",     "debug mode, print debug information");

  cmd.addGroup("input options:");
    cmd.addSubGroup("input file format:");
     cmd.addOption("--read-file",            "+f",     "read file format or data set (default)");
     cmd.addOption("--read-file-only",       "+fo",    "read file format only");
     cmd.addOption("--read-dataset",         "-f",     "read data set without file meta information");
    cmd.addSubGroup("input transfer syntax:", LONGCOL, SHORTCOL);
     cmd.addOption("--read-xfer-auto",       "-t=",    "use TS recognition (default)");
     cmd.addOption("--read-xfer-detect",     "-td",    "ignore TS specified in the file meta header");
     cmd.addOption("--read-xfer-little",     "-te",    "read with explicit VR little endian TS");
     cmd.addOption("--read-xfer-big",        "-tb",    "read with explicit VR big endian TS");
     cmd.addOption("--read-xfer-implicit",   "-ti",    "read with implicit VR little endian TS");
    cmd.addSubGroup("parsing of odd-length attributes:");
     cmd.addOption("--accept-odd-length",    "+ao",    "accept odd length attributes (default)");
     cmd.addOption("--assume-even-length",   "+ae",    "assume real length is one byte larger");
    cmd.addSubGroup("handling of undefined length UN elements:");
     cmd.addOption("--enable-cp246",         "+ui",    "read undefined len UN as implicit VR (default)");
     cmd.addOption("--disable-cp246",        "-ui",    "read undefined len UN as explicit VR");
    cmd.addSubGroup("handling of defined length UN elements:");
     cmd.addOption("--retain-un",            "-uc",    "retain elements as UN (default)");
     cmd.addOption("--convert-un",           "+uc",    "convert to real VR if known");
    cmd.addSubGroup("automatic data correction:");
     cmd.addOption("--enable-correction",    "+dc",    "enable automatic data correction (default)");
     cmd.addOption("--disable-correction",   "-dc",    "disable automatic data correction");
#ifdef WITH_ZLIB
    cmd.addSubGroup("bitstream format of deflated input:");
     cmd.addOption("--bitstream-deflated",   "+bd",    "expect deflated bitstream (default)");
     cmd.addOption("--bitstream-zlib",       "+bz",    "expect deflated zlib bitstream");
#endif

   cmd.addGroup("execution options:", LONGCOL, SHORTCOL+2);
    cmd.addOption(  "--exec",              "-x",   1, "[c]ommand: string",
                                                      "execute command c after PDF extraction" );
    /* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::PF_ExpandWildcards))
    {
      /* check exclusive options first */
      if (cmd.hasExclusiveOption())
      {
          if (cmd.findOption("--version"))
          {
              app.printHeader(OFTrue /*print host identifier*/);          // uses ofConsole.lockCerr()
              ofConsole.lockCerr() << OFendl << "External libraries used:";
              ofConsole.unlockCerr();
#ifdef WITH_ZLIB
              ofConsole.lockCerr() << OFendl << "- ZLIB, Version " << zlibVersion() << OFendl;
              ofConsole.unlockCerr();
#else
              ofConsole.lockCerr() << " none" << OFendl;
              ofConsole.unlockCerr();
#endif
              return 0;
          }
      }

      /* command line parameters and options */
      cmd.getParam(1, opt_ifname);
      cmd.getParam(2, opt_ofname);

      if (cmd.findOption("--verbose")) opt_verbose = OFTrue;
      if (cmd.findOption("--debug")) opt_debugMode = 5;

      cmd.beginOptionBlock();
      if (cmd.findOption("--read-file")) opt_readMode = ERM_autoDetect;
      if (cmd.findOption("--read-file-only")) opt_readMode = ERM_fileOnly;
      if (cmd.findOption("--read-dataset")) opt_readMode = ERM_dataset;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--read-xfer-auto"))
          opt_ixfer = EXS_Unknown;
      if (cmd.findOption("--read-xfer-detect"))
          dcmAutoDetectDatasetXfer.set(OFTrue);
      if (cmd.findOption("--read-xfer-little"))
      {
          app.checkDependence("--read-xfer-little", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_LittleEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-big"))
      {
          app.checkDependence("--read-xfer-big", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_BigEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-implicit"))
      {
          app.checkDependence("--read-xfer-implicit", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_LittleEndianImplicit;
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--accept-odd-length"))
      {
        dcmAcceptOddAttributeLength.set(OFTrue);
      }
      if (cmd.findOption("--assume-even-length"))
      {
        dcmAcceptOddAttributeLength.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-cp246"))
      {
        dcmEnableCP246Support.set(OFTrue);
      }
      if (cmd.findOption("--disable-cp246"))
      {
        dcmEnableCP246Support.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--retain-un"))
      {
        dcmEnableUnknownVRConversion.set(OFFalse);
      }
      if (cmd.findOption("--convert-un"))
      {
        dcmEnableUnknownVRConversion.set(OFTrue);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-correction"))
      {
        dcmEnableAutomaticInputDataCorrection.set(OFTrue);
      }
      if (cmd.findOption("--disable-correction"))
      {
        dcmEnableAutomaticInputDataCorrection.set(OFFalse);
      }
      cmd.endOptionBlock();

#ifdef WITH_ZLIB
      cmd.beginOptionBlock();
      if (cmd.findOption("--bitstream-deflated"))
      {
        dcmZlibExpectRFC1950Encoding.set(OFFalse);
      }
      if (cmd.findOption("--bitstream-zlib"))
      {
        dcmZlibExpectRFC1950Encoding.set(OFTrue);
      }
      cmd.endOptionBlock();
#endif

      if (cmd.findOption("--exec")) app.checkValue(cmd.getValue(opt_execString));

   }

    SetDebugLevel((opt_debugMode));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        ofConsole.lockCerr() << "Warning: no data dictionary loaded, "
             << "check environment variable: "
             << DCM_DICT_ENVIRONMENT_VARIABLE << OFendl;
        ofConsole.unlockCerr();
    }


    // open inputfile
    if ((opt_ifname == NULL) || (strlen(opt_ifname) == 0))
    {
        CERR << "Error: invalid filename: <empty string>" << OFendl;
        return 1;
    }

    DcmFileFormat fileformat;
    DcmDataset * dataset = fileformat.getDataset();

    if (opt_verbose)
        COUT << "open input file " << opt_ifname << OFendl;

    OFCondition error = fileformat.loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);

    if (error.bad())
    {
        CERR << "Error: "
             << error.text()
             << ": reading file: " <<  opt_ifname << OFendl;
        return 1;
    }

    OFString sopClass;
    error = dataset->findAndGetOFString(DCM_SOPClassUID, sopClass);
    if (error.bad() || sopClass != UID_EncapsulatedPDFStorage)
    {
        CERR << "Error: not an Encapsulated PDF Storage object: " <<  opt_ifname << OFendl;
        return 1;
    }

    DcmElement *delem = NULL;
    error = dataset->findAndGetElement(DCM_EncapsulatedDocument, delem);
    if (error.bad() || delem == NULL)
    {
        CERR << "Error: attribute (0042,0011) Encapsulated Document missing." << OFendl;
        return 1;
    }

    Uint32 len = delem->getLength();
    Uint8 *pdfDocument = NULL;
    error = delem->getUint8Array(pdfDocument);
    if (error.bad() || pdfDocument == NULL || len == 0)
    {
        CERR << "Error: attribute (0042,0011) Encapsulated Document empty or wrong VR." << OFendl;
        return 1;
    }

    /* strip pad byte at end of file, if there is one. The PDF format expects
     * files to end with %%EOF followed by CR/LF. If the last character of the
     * file is not a CR or LF, we assume it is a pad byte and remove it.
     */
    if (pdfDocument[len-1] != 10 && pdfDocument[len-1] != 13) 
    {
      --len;
    }

    FILE *pdffile = fopen(opt_ofname, "wb");
    if (pdffile == NULL)
    {
      ofConsole.lockCerr() << "Error: unable to create file " << opt_ofname << OFendl;
      ofConsole.unlockCerr();
      return 1;
    }

    if (len != fwrite(pdfDocument, 1, len, pdffile))
    {
      ofConsole.lockCerr() << "Error: write error in file " << opt_ofname << OFendl;
      ofConsole.unlockCerr();
      fclose(pdffile);
      return 1;
    }

    fclose(pdffile);
    
    if (opt_verbose)
    {
        ofConsole.lockCout() << "conversion successful\n";
        ofConsole.unlockCout();
    }

    if (opt_execString) 
    {
      OFString cmd = opt_execString;
      cmd = replaceChars( cmd, OFString(FILENAME_PLACEHOLDER), opt_ofname );

      // Execute command and return result
      return system(cmd.c_str());
    }

    return 0;
}


/*
 * CVS/RCS Log:
 * $Log: dcm2pdf.cc,v $
 * Revision 1.1  2007-07-11 09:10:29  meichel
 * Added new tool dcm2pdf that extracts a PDF document from a DICOM
 *   Encapsulated PDF file, i.e. is the counterpart to pdf2dcm.
 *
 *
 */