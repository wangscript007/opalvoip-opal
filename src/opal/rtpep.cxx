/*
 * rtpep.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "rtpep.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpep.h>
#include <opal/rtpconn.h>


OpalRTPEndPoint::OpalRTPEndPoint(OpalManager & manager,     ///<  Manager of all endpoints.
                       const PCaselessString & prefix,      ///<  Prefix for URL style address strings
                                      unsigned attributes)  ///<  Bit mask of attributes endpoint has
  : OpalEndPoint(manager, prefix, attributes)
#ifdef OPAL_ZRTP
    , zrtpEnabled(manager.GetZRTPEnabled())
#endif
{
}

OpalRTPEndPoint::~OpalRTPEndPoint()
{
}


PBoolean OpalRTPEndPoint::IsRTPNATEnabled(OpalConnection & conn, 
                                const PIPSocket::Address & localAddr, 
                                const PIPSocket::Address & peerAddr,
                                const PIPSocket::Address & sigAddr,
                                                PBoolean   incoming)
{
  return GetManager().IsRTPNATEnabled(conn, localAddr, peerAddr, sigAddr, incoming);
}


OpalMediaFormatList OpalRTPEndPoint::GetMediaFormats() const
{
  return manager.GetCommonMediaFormats(true, false);
}


#ifdef OPAL_ZRTP

bool OpalRTPEndPoint::GetZRTPEnabled() const
{ 
  return zrtpEnabled; 
}

#endif


static RTP_UDP * GetRTPFromStream(const OpalMediaStream & stream)
{
  const OpalRTPMediaStream * rtpStream = dynamic_cast<const OpalRTPMediaStream *>(&stream);
  if (rtpStream == NULL)
    return NULL;

  return dynamic_cast<RTP_UDP *>(&rtpStream->GetRtpSession());
}


void OpalRTPEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  CheckEndLocalRTP(stream.GetConnection(), GetRTPFromStream(stream));

  OpalEndPoint::OnClosedMediaStream(stream);
}


bool OpalRTPEndPoint::OnLocalRTP(OpalConnection & connection1,
                                 OpalConnection & connection2,
                                 unsigned         sessionID,
                                 bool             opened) const
{
  return manager.OnLocalRTP(connection1, connection2, sessionID, opened);
}


bool OpalRTPEndPoint::CheckForLocalRTP(const OpalRTPMediaStream & stream)
{
  RTP_UDP * rtp = GetRTPFromStream(stream);
  if (rtp == NULL)
    return false;

  if (!PIPSocket::IsLocalHost(rtp->GetRemoteAddress())) {
    PTRACE(5, "RTPEp\tRemote RTP address " << rtp->GetRemoteAddress() << " not local.");
    return false;
  }

  std::pair<LocalRtpInfoMap::iterator, bool> insertResult =
            m_connectionsByRtpLocalPort.insert(LocalRtpInfoMap::value_type(rtp->GetLocalDataPort(), stream.GetConnection()));
  PTRACE_IF(4, insertResult.second,
            "RTPEp\tRemembering local RTP port " << rtp->GetLocalDataPort()
            << " on connection " << stream.GetConnection());

  LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(rtp->GetRemoteDataPort());
  if (it == m_connectionsByRtpLocalPort.end()) {
    PTRACE(5, "RTPEp\tRemote RTP port " << rtp->GetRemoteDataPort() << " not peviously remembered, searching.");
    // Not already cached so search all RTP connections for if it is there
    PWaitAndSignal mutex(connectionsActive.GetMutex());
    for (PINDEX index = 0; index < connectionsActive.GetSize(); ++index) {
      PSafePtr<OpalRTPConnection> connection = PSafePtrCast<OpalConnection, OpalRTPConnection>(connectionsActive.GetAt(index, PSafeReadOnly));
      if (connection != NULL && connection->FindSessionByLocalPort(rtp->GetRemoteDataPort()) != NULL) {
        PTRACE(4, "RTPEp\tRemembering remote RTP port " << rtp->GetRemoteDataPort() << " on connection " << *connection);
        it = m_connectionsByRtpLocalPort.insert(LocalRtpInfoMap::value_type(rtp->GetRemoteDataPort(), *connection)).first;
        break;
      }
    }
    if (it == m_connectionsByRtpLocalPort.end()) {
      PTRACE(4, "RTPEp\tRemote RTP port " << rtp->GetRemoteDataPort() << " not this process.");
      return false;
    }
  }

  if (it->second.m_previousResult < 0) {
    it->second.m_previousResult = OnLocalRTP(stream.GetConnection(), it->second.m_connection, rtp->GetSessionID(), true);
    if (insertResult.second)
      insertResult.first->second.m_previousResult = it->second.m_previousResult;

    PTRACE(3, "RTPEp\tLocal RTP ports " << rtp->GetRemoteDataPort() << " and " << it->first
           << " flagged as " << (it->second.m_previousResult != 0 ? "bypassed" : "normal"));
  }

  return it->second.m_previousResult != 0;
}


void OpalRTPEndPoint::CheckEndLocalRTP(OpalConnection & connection, RTP_UDP * rtp)
{
  if (rtp == NULL)
    return;

  LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(rtp->GetLocalDataPort());
  if (it != m_connectionsByRtpLocalPort.end()) {
    m_connectionsByRtpLocalPort.erase(it);

    it = m_connectionsByRtpLocalPort.find(rtp->GetRemoteDataPort());
    if (it != m_connectionsByRtpLocalPort.end())
      OnLocalRTP(connection, it->second.m_connection, rtp->GetSessionID(), false);
  }
}
