/*
 *
 *  Copyright (C) 1994-2003, OFFIS
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
 *  Module:  dcmnet
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: 
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2003-06-10 14:30:15 $
 *  Source File:      $Source: /export/gitmirror/dcmtk-git/../dcmtk-cvs/dcmtk/dcmnet/libsrc/dcasccfg.cc,v $
 *  CVS/RCS Revision: $Revision: 1.1 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "osconfig.h"
#include "dcasccfg.h"
#include "dcerror.h"  /* for EC_IllegalCall */

DcmAssociationConfiguration::DcmAssociationConfiguration()
: xferSyntaxes_()
, contexts_()
, roleselection_()
, extneg_()
, profiles_()
{
}

DcmAssociationConfiguration::~DcmAssociationConfiguration()
{
}

OFCondition DcmAssociationConfiguration::addTransferSyntax(
  const char *key,
  const char *transferSyntaxUID)
{
  return xferSyntaxes_.add(key, transferSyntaxUID);
}

OFCondition DcmAssociationConfiguration::addPresentationContext(
  const char *key,
  const char *abstractSyntaxUID,
  const char *transferSyntaxKey)
{
  if ((!key)||(!abstractSyntaxUID)||(!transferSyntaxKey)) return EC_IllegalCall;

  if (! xferSyntaxes_.isKnownKey(transferSyntaxKey))
  {
    // error: key undefined
    OFString s("transfer syntax key undefined: ");
    s += transferSyntaxKey;
    return makeOFCondition(OFM_dcmnet, 1031, OF_error, s.c_str());
  }

  return contexts_.add(key, abstractSyntaxUID, transferSyntaxKey);
}

OFCondition DcmAssociationConfiguration::addRole(
  const char *key,
  const char *abstractSyntaxUID,
  T_ASC_SC_ROLE role)
{
  return roleselection_.add(key, abstractSyntaxUID, role);
}

OFCondition DcmAssociationConfiguration::addExtendedNegotiation(
  const char *key,
  const char *abstractSyntaxUID,
  const unsigned char *rawData,
  Uint32 length)
{
  return extneg_.add(key, abstractSyntaxUID, rawData, length);
}

OFCondition DcmAssociationConfiguration::addProfile(
  const char *key,
  const char *presentationContextKey,
  const char *roleSelectionKey,
  const char *extendedNegotiationKey)
{
  if ((!key)||(!presentationContextKey)) return EC_IllegalCall;

  if (!contexts_.isKnownKey(presentationContextKey))
  {
    // error: key undefined
    OFString s("presentation context key undefined: ");
    s += presentationContextKey;
    return makeOFCondition(OFM_dcmnet, 1033, OF_error, s.c_str());
  }

  OFCondition status = EC_Normal;
  if (roleSelectionKey)
  {
    // check that role selection list exists and matches presentation context list
    status = roleselection_.checkConsistency(roleSelectionKey, contexts_, presentationContextKey);
    if (status.bad()) return status;
  }

  if (extendedNegotiationKey)
  {
    // check that extended negotiation list exists and matches presentation context list
    status = extneg_.checkConsistency(extendedNegotiationKey, contexts_, presentationContextKey);
    if (status.bad()) return status;
  }

  return profiles_.add(key, presentationContextKey, roleSelectionKey, extendedNegotiationKey);
}

OFBool DcmAssociationConfiguration::isKnownProfile(const char *key) const
{
  return profiles_.isKnownKey(key);
}

OFCondition DcmAssociationConfiguration::setAssociationParameters(
  const char *profile,
  T_ASC_Parameters& params) const
{
  if (!profile) return EC_IllegalCall;

  const char *contextKey = profiles_.getPresentationContextKey(profile);
  if (! contextKey)
  {
    // error: key undefined
    OFString s("profile key undefined: ");
    s += profile;
    return makeOFCondition(OFM_dcmnet, 1062, OF_error, s.c_str());
  }

  // these two keys may be NULL
  const char *roleKey = profiles_.getRoleSelectionKey(profile);
  const char *extnegKey = profiles_.getExtendedNegotiationKey(profile);

  // pointer to list of presentation contexts
  const DcmPresentationContextList *contextList = contexts_.getPresentationContextList(contextKey);
  if (! contextList)
  {
    // error: key undefined
    OFString s("presentation context key undefined: ");
    s += contextKey;
    return makeOFCondition(OFM_dcmnet, 1063, OF_error, s.c_str());
  }

  // pointer to list of SCP/SCU role selection items
  const DcmRoleSelectionList *roleSelectionList = NULL;
  if (roleKey)
  {
    roleSelectionList = roleselection_.getRoleSelectionList(roleKey);
    if (! roleSelectionList)
    {
      // error: key undefined
      OFString s("role selection key undefined: ");
      s += roleKey;
      return makeOFCondition(OFM_dcmnet, 1064, OF_error, s.c_str());
    }
  }

  // pointer to list of SCP/SCU role selection items
  const DcmExtendedNegotiationList *extendedNegotiationList = NULL;
  if (extnegKey)
  {
    extendedNegotiationList = extneg_.getExtendedNegotiationList(extnegKey);
    if (! extendedNegotiationList)
    {
      // error: key undefined
      OFString s("extended negotiation key undefined: ");
      s += extnegKey;
      return makeOFCondition(OFM_dcmnet, 1065, OF_error, s.c_str());
    }
  }

  // initialize presentation context ID counter
  T_ASC_PresentationContextID cid = 1;
  OFCondition result = EC_Normal;
  const char *abstractSyntax = NULL;
  const char *transferSyntaxKey = NULL;
  const DcmTransferSyntaxList *transferSyntaxList = NULL;
  const char *xferSyntaxes[128];
  OFListIterator(DcmUIDHandler) xffirst;
  OFListIterator(DcmUIDHandler) xflast;
  int numXferSyntaxes = 0;
  T_ASC_SC_ROLE proposedRole;
  OFListIterator(DcmRoleSelectionItem) rsfirst;
  OFListIterator(DcmRoleSelectionItem) rslast;
  DcmUIDHandler uid;

  // now loop through all presentation contexts  
  OFListIterator(DcmPresentationContextItem) first = contextList->begin();
  OFListIterator(DcmPresentationContextItem) last = contextList->end();
  while (first != last)
  {
    // determine abstract syntax
    abstractSyntax = (*first).getAbstractSyntax();
    transferSyntaxKey = (*first).getTransferSyntaxKey();
    transferSyntaxList = xferSyntaxes_.getTransferSyntaxList(transferSyntaxKey);

    if (! transferSyntaxList)
    {
      // error: key undefined
      OFString s("transfer syntax key undefined: ");
      s += transferSyntaxKey;
      return makeOFCondition(OFM_dcmnet, 1066, OF_error, s.c_str());
    }

    if (transferSyntaxList->size() > 128)
    {
      // error: key undefined
      OFString s("transfer syntax list too long, cannot handle more than 128 entries: ");
      s += transferSyntaxKey;
      return makeOFCondition(OFM_dcmnet, 1067, OF_error, s.c_str());
    }

    if (transferSyntaxList->size() < 1)
    {
      // error: key undefined
      OFString s("transfer syntax list empty: ");
      s += transferSyntaxKey;
      return makeOFCondition(OFM_dcmnet, 1068, OF_error, s.c_str());
    }

    // copy list of transfer syntaxes into array
    numXferSyntaxes = 0;
    xffirst = transferSyntaxList->begin();
    xflast  = transferSyntaxList->end();
    while (xffirst != xflast)
    {
      xferSyntaxes[numXferSyntaxes++] = (*xffirst).c_str();
      ++xffirst;
    }

    // look up SCP/SCU role for this abstract syntax
    proposedRole = ASC_SC_ROLE_DEFAULT;
    if (roleSelectionList)
    {
      // look-up abstract syntax in role selection list
      rsfirst = roleSelectionList->begin();
      rslast = roleSelectionList->end();
      uid = abstractSyntax;
      while (rsfirst != rslast)
      {        
        if ((*rsfirst).matches(uid))
        {
          // found abstract syntax, set role accordingly
          proposedRole = (*rsfirst).getRole();
          break; /* out of while loop */
        }
        ++rsfirst;
      }
    }

    // finally add presentation context to params
    result = ASC_addPresentationContext(&params, cid, abstractSyntax, xferSyntaxes, numXferSyntaxes, proposedRole);
    if (result.bad()) return result;

    // increase index by 2
    cid += 2;
    ++first;
  }

  // create extended negotiation items if present
  if (extendedNegotiationList)
  {
    // create extended negotiation item list as required by dcmnet code
    SOPClassExtendedNegotiationSubItemList *enlist = new SOPClassExtendedNegotiationSubItemList();
    ASC_setRequestedExtNegList(&params, enlist);
    SOPClassExtendedNegotiationSubItem *currentItem = NULL;

    // for each entry in our list
    OFListIterator(DcmExtendedNegotiationItem) enfirst = extendedNegotiationList->begin();
    OFListIterator(DcmExtendedNegotiationItem) enlast = extendedNegotiationList->end();
    while (enfirst != enlast)
    {
      // create and populate SOPClassExtendedNegotiationSubItem object
      currentItem = new SOPClassExtendedNegotiationSubItem();
      currentItem->sopClassUID = (*enfirst).getAbstractSyntaxC();
      currentItem->serviceClassAppInfoLength = (unsigned short)((*enfirst).getLength());
      currentItem->serviceClassAppInfo = new unsigned char[currentItem->serviceClassAppInfoLength];
      memcpy(currentItem->serviceClassAppInfo, (*enfirst).getRaw() , currentItem->serviceClassAppInfoLength);
      enlist->push_back(currentItem);
      ++enfirst;
    }
  }

  return result;
}

OFCondition DcmAssociationConfiguration::evaluateAssociationParameters(
  const char *profile,
  T_ASC_Association &assoc) const
{
  if (!profile) return EC_IllegalCall;

  const char *contextKey = profiles_.getPresentationContextKey(profile);
  if (! contextKey)
  {
    // error: key undefined
    OFString s("profile key undefined: ");
    s += profile;
    return makeOFCondition(OFM_dcmnet, 1069, OF_error, s.c_str());
  }

  // these two keys may be NULL
  const char *roleKey = profiles_.getRoleSelectionKey(profile);
  const char *extnegKey = profiles_.getExtendedNegotiationKey(profile);

  // pointer to list of presentation contexts
  const DcmPresentationContextList *contextList = contexts_.getPresentationContextList(contextKey);
  if (! contextList)
  {
    // error: key undefined
    OFString s("presentation context key undefined: ");
    s += contextKey;
    return makeOFCondition(OFM_dcmnet, 1070, OF_error, s.c_str());
  }

  // pointer to list of SCP/SCU role selection items
  const DcmRoleSelectionList *roleSelectionList = NULL;
  if (roleKey)
  {
    roleSelectionList = roleselection_.getRoleSelectionList(roleKey);
    if (! roleSelectionList)
    {
      // error: key undefined
      OFString s("role selection key undefined: ");
      s += roleKey;
      return makeOFCondition(OFM_dcmnet, 1071, OF_error, s.c_str());
    }
  }

  // pointer to list of SCP/SCU role selection items
  const DcmExtendedNegotiationList *extendedNegotiationList = NULL;
  if (extnegKey)
  {
    extendedNegotiationList = extneg_.getExtendedNegotiationList(extnegKey);
    if (! extendedNegotiationList)
    {
      // error: key undefined
      OFString s("extended negotiation key undefined: ");
      s += extnegKey;
      return makeOFCondition(OFM_dcmnet, 1072, OF_error, s.c_str());
    }
  }

  // declare a few variables we need later
  OFCondition result = EC_Normal;
  T_ASC_PresentationContext pc;
  const char *transferSyntaxKey = NULL;
  OFBool found;
  T_ASC_SC_ROLE acceptedRole;
  OFListIterator(DcmRoleSelectionItem) rsfirst;
  OFListIterator(DcmRoleSelectionItem) rslast;
  DcmUIDHandler uid;
  const DcmTransferSyntaxList *transferSyntaxList = NULL;
  OFListIterator(DcmPresentationContextItem) first;
  OFListIterator(DcmPresentationContextItem) last = contextList->end();
  OFListIterator(DcmUIDHandler) tsfirst;
  OFListIterator(DcmUIDHandler) tslast;
  int numContexts = ASC_countPresentationContexts(assoc.params);
  unsigned char j = 0;

  // traverse list of presentation contexts
  for (int i = 0; i < numContexts; ++i)
  {
    // retrieve presentation context
    result = ASC_getPresentationContext(assoc.params, i, &pc);
    if (result.bad()) return result;

    // check if the abstract syntax is in our list of abstract syntaxes
    first = contextList->begin();
    found = OFFalse;
    while (first != last)
    {
      if (0 == strcmp((*first).getAbstractSyntax(), pc.abstractSyntax))
      {
        found = OFTrue;
        break; /* out of while loop */
      }
      ++first;
    }

    if (!found)
    {
      // abstract syntax not supported, reject presentation context
      result = ASC_refusePresentationContext(assoc.params, pc.presentationContextID, 
        ASC_P_ABSTRACTSYNTAXNOTSUPPORTED);
      if (result.bad()) return result;
    }
    else
    {
      // abstract syntax is supported, get transfer syntax list and SCP/SCU role
      transferSyntaxKey = (*first).getTransferSyntaxKey();
      transferSyntaxList = xferSyntaxes_.getTransferSyntaxList(transferSyntaxKey);      
      if (! transferSyntaxList)
      {
        // error: key undefined
        OFString s("transfer syntax key undefined: ");
        s += transferSyntaxKey;
        return makeOFCondition(OFM_dcmnet, 1073, OF_error, s.c_str());
      }
    
      // look up SCP/SCU role for this abstract syntax
      acceptedRole = ASC_SC_ROLE_DEFAULT;
      if (roleSelectionList)
      {
        // look-up abstract syntax in role selection list
        rsfirst = roleSelectionList->begin();
        rslast = roleSelectionList->end();
        uid = pc.abstractSyntax;
        while (rsfirst != rslast)
        {        
          if ((*rsfirst).matches(uid))
          {
            // found abstract syntax, set role accordingly
            acceptedRole = (*rsfirst).getRole();
            break; /* out of while loop */
          }
          ++rsfirst;
        }
      }

      // try to accept the presentation context with the most preferred transfer syntax
      tsfirst = transferSyntaxList->begin();
      tslast = transferSyntaxList->end();
      found = OFFalse;

      // loop through our list of preferred transfer syntaxes for this abstract syntax,
      // starting with the most preferred one
      while ((tsfirst != tslast) && !found)
      {
        // loop through list of transfer syntaxes in presentation context
        for (j=0; (!found && (j < pc.transferSyntaxCount)); ++j)
        {
          if ((*tsfirst).str() == pc.proposedTransferSyntaxes[j])
          {
            // matching transfer syntax found, accept context
            found = OFTrue;
            result = ASC_acceptPresentationContext(
                assoc.params, pc.presentationContextID,
                pc.proposedTransferSyntaxes[j], acceptedRole);
            if (result.bad()) return result;
          }
        }
        ++tsfirst;
      }

      // no matching transfer syntax found, reject context     
      if (!found)
      {
        // transfer syntax not supported, reject presentation context
        result = ASC_refusePresentationContext(assoc.params, pc.presentationContextID, 
          ASC_P_TRANSFERSYNTAXESNOTSUPPORTED);
        if (result.bad()) return result;
      }
    }
  }

  // create extended negotiation items if present
  if (extendedNegotiationList)
  {
    // create extended negotiation item list as required by dcmnet code
    SOPClassExtendedNegotiationSubItemList *enlist = new SOPClassExtendedNegotiationSubItemList();
    SOPClassExtendedNegotiationSubItem *currentItem = NULL;

    // for each entry in our list
    OFListIterator(DcmExtendedNegotiationItem) enfirst = extendedNegotiationList->begin();
    OFListIterator(DcmExtendedNegotiationItem) enlast = extendedNegotiationList->end();
    while (enfirst != enlast)
    {
      // check if we have an accepted presentation context for this abstract syntax
      if (0 != ASC_findAcceptedPresentationContextID(&assoc, (*enfirst).getAbstractSyntaxC()))
      {
        // create and populate SOPClassExtendedNegotiationSubItem object
        currentItem = new SOPClassExtendedNegotiationSubItem();
        currentItem->sopClassUID = (*enfirst).getAbstractSyntaxC();
        currentItem->serviceClassAppInfoLength = (unsigned short)((*enfirst).getLength());
        currentItem->serviceClassAppInfo = new unsigned char[currentItem->serviceClassAppInfoLength];
        memcpy(currentItem->serviceClassAppInfo, (*enfirst).getRaw() , currentItem->serviceClassAppInfoLength);
        enlist->push_back(currentItem);
      }
      ++enfirst;
    }

    if (enlist->size() > 0)
      ASC_setAcceptedExtNegList(assoc.params, enlist);
      else delete enlist;
  }

  return result;
}


/*
 * CVS/RCS Log
 * $Log: dcasccfg.cc,v $
 * Revision 1.1  2003-06-10 14:30:15  meichel
 * Initial release of class DcmAssociationConfiguration and support
 *   classes. This class maintains a list of association negotiation
 *   profiles that can be addressed by symbolic keys. The profiles may
 *   be read from a configuration file.
 *
 *
 */