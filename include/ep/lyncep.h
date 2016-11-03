/*
 * lyncep.h
 *
 * Header file for Lync (UCMA) interface
 *
 * Copyright (C) 2016 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_EP_LYNCEP_H
#define OPAL_EP_LYNCEP_H

#include <opal_config.h>

#if OPAL_LYNC

#include <string>

/* Class to hide Lync UCMA Managed code
   This odd arrangement is so when compiling the shim and we include this header
   file, we do not get all the PTLib stuff which interferes with the managed
   code interfacing.
 */
class OpalLyncShim
{
public:
  OpalLyncShim();
  ~OpalLyncShim();

protected:
  bool StartPlatform(const char * userAgent);
  bool ShutdownPlatform();

  struct UserEndpoint;
  UserEndpoint * CreateUserEndpoint(const char * uri);
  void DestroyUserEndpoint(UserEndpoint * user);

  const std::string & GetLastError() const { return m_lastError; }

private:
  struct Platform;
  Platform * m_platform;

  std::string m_lastError;
};


#ifndef OPAL_LYNC_SHIM

#include <opal/endpoint.h>
#include <opal/connection.h>


//////////////////////////////////////////////////////////////////

class OpalLyncShim;
class OpalLyncConnection;


/**Endpoint for interfacing Microsoft Lync via UCMA.
 */
class OpalLyncEndPoint : public OpalEndPoint, public OpalLyncShim
{
  PCLASSINFO(OpalLyncEndPoint, OpalEndPoint);
public:
  /**@name Construction */
  //@{
  /**Create a new endpoint.
   */
  OpalLyncEndPoint(
    OpalManager & manager,
    const char *prefix = "lync"
  );

  /**Destroy endpoint.
   */
  virtual ~OpalLyncEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
  /** Shut down the endpoint, this is called by the OpalManager just before
      destroying the object and can be handy to make sure some things are
      stopped before the vtable gets clobbered.
      */
  virtual void ShutDown();

  /**Get the data formats this endpoint is capable of operating.
     This provides a list of media data format names that may be used by an
     OpalMediaStream may be created by a connection from this endpoint.

     Note that a specific connection may not actually support all of the
     media formats returned here, but should return no more.
     */
  virtual OpalMediaFormatList GetMediaFormats() const;

  /** Set up a connection to a remote party.
      This is called from the OpalManager::MakeConnection() function once
      it has determined that this is the endpoint for the protocol.

      The general form for this party parameter is:

      [proto:][alias@][transport$]address[:port]

      where the various fields will have meanings specific to the endpoint
      type. For example, with H.323 it could be "h323:Fred@site.com" which
      indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
      endpoint it could be "pstn:5551234" which is to call 5551234 on the
      first available PSTN line.

      The proto field is optional when passed to a specific endpoint. If it
      is present, however, it must agree with the endpoints protocol name or
      false is returned.

      This function usually returns almost immediately with the connection
      continuing to occur in a new background thread.

      If false is returned then the connection could not be established. For
      example if a PSTN endpoint is used and the assiciated line is engaged
      then it may return immediately. Returning a non-NULL value does not
      mean that the connection will succeed, only that an attempt is being
      made.
      */
  virtual PSafePtr<OpalConnection> MakeConnection(
    OpalCall & call,                         ///<  Owner of connection
    const PString & party,                   ///<  Remote party to call
    void * userData,                         ///<  Arbitrary data to pass to connection
    unsigned int options,                    ///<  options to pass to conneciton
    OpalConnection::StringOptions * stringOptions  ///<  complex string options
  );

  /** Execute garbage collection for endpoint.
      Returns true if all garbage has been collected.
      */
  virtual PBoolean GarbageCollection();
  //@}

  //@{
  bool Register(
    const PString & uri
  );
  bool Unregister(
    const PString & uri
  );
  //@}

protected:
  typedef std::map<PString, OpalLyncShim::UserEndpoint *> RegistrationMap;
  RegistrationMap m_registrations;
  PMutex          m_registrationMutex;
};


/**Connection for interfacing Microsoft Lync via UCMA.
  */
class OpalLyncConnection : public OpalConnection
{
    PCLASSINFO(OpalLyncConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalLyncConnection(
      OpalCall & call,
      OpalLyncEndPoint & ep,
      const PString & dialNumber,
      void * /*userData*/,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /** Get indication of connection being to a "network".
        This indicates the if the connection may be regarded as a "network"
        connection. The distinction is about if there is a concept of a "remote"
        party being connected to and is best described by example: sip, h323,
        iax and pstn are all "network" connections as they connect to something
        "remote". While pc, pots and ivr are not as the entity being connected
        to is intrinsically local.
    */
    virtual bool IsNetworkConnection() const { return true; }

    /** Start an outgoing connection.
        This function will initiate the connection to the remote entity, for
        example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
    */
    virtual PBoolean SetUpConnection();

    /** Clean up the termination of the connection.
        This function can do any internal cleaning up and waiting on background
        threads that may be using the connection object.

        Note that there is not a one to one relationship with the
        OnEstablishedConnection() function. This function may be called without
        that function being called. For example if SetUpConnection() was used
        but the call never completed.

        Classes that override this function should make sure they call the
        ancestor version for correct operation.

        An application will not typically call this function as it is used by
        the OpalManager during a release of the connection.
      */
    virtual void OnReleased();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.
      */
    virtual PBoolean SetConnected();

    /** Get the remote transport address
      */
    virtual OpalTransportAddress GetRemoteAddress() const;
  //@}
};


#endif // OPAL_LYNC_INTERNAL

#endif // OPAL_LYNC

#endif // OPAL_EP_LYNCEP_H


// End of File ///////////////////////////////////////////////////////////////