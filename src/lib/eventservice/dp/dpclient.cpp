/*
* vzes
* Copyright 2013 - 2018, Vzenith Inc.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*  3. The name of the author may not be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpserver.h"

namespace vzes {

class DpClientManager;

DpClientManager *GetDpServiceInstance();

bool InitDpSystem() {
  GetDpServiceInstance();
  return true;
}

class DpClientImpl : public DpClient,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public MessageHandler,
  public boost::enable_shared_from_this<DpClientImpl> {
 public:
  typedef boost::shared_ptr<DpClientImpl> Ptr;

  DpClientImpl(EventService::Ptr es)
    : event_service_(es) {
    client_manager_ = GetDpServiceInstance();
    signal_event_   = vzes::SignalEvent::CreateSignalEvent();
  }

  virtual ~DpClientImpl() {
  }

  bool InitDpClient() {
    RegisterDpClient();
    return true;
  }

  void RegisterDpClient();
  void UnregisterDpClient();

  virtual bool SendDpMessage(const std::string method,
                             uint32 session_id,
                             const char *data,
                             uint32 data_size);

  virtual bool SendDpRequest(const std::string method,
                             uint32 session_id,
                             const char *data,
                             uint32 data_size,
                             DpBuffer *res_buffer,
                             uint32 timeout_millisecond);

  virtual bool SendDpReply(DpMessage::Ptr dp_msg) {
    vzes::CritScope cr(&crit_);
    dp_msg->type = TYPE_REPLY;
    dp_msg->signal_event->TriggerSignal();
    return true;
  }

  virtual void ListenMessage(const std::string method) {
    vzes::CritScope cr(&crit_);
    method_set_.insert(method);
  }

  virtual void RemoveMessage(const std::string method) {
    vzes::CritScope cr(&crit_);
    method_set_.erase(method);
  }

  virtual void OnMessage(Message *msg) {
    DpMessage::Ptr dp_msg = boost::dynamic_pointer_cast<DpMessage>(msg->pdata);
    SignalDpMessage(shared_from_this(), dp_msg);
  }

  bool HandleDpMessage(DpClientImpl::Ptr dp_client,
                       DpMessage::Ptr dp_msg) {
    vzes::CritScope cr(&crit_);
    std::set<std::string>::iterator pos = method_set_.find(dp_msg->method);
    if (pos != method_set_.end()) {
      event_service_->Post(this, 0, dp_msg);
      return true;
    }
    return false;
  }
 private:
  std::set<std::string> method_set_;
  EventService::Ptr     event_service_;
  vzes::CriticalSection crit_;
  DpClientManager       *client_manager_;
  vzes::SignalEvent::Ptr  signal_event_;
};

class DpClientManager {
 public:
  DpClientManager() {
  }
  virtual ~DpClientManager() {
  }

  void AddDpClient(DpClientImpl::Ptr dp_client) {
    DpClients::iterator pos = std::find(dpclients_.begin(),
                                        dpclients_.end(),
                                        dp_client);
    if (pos != dpclients_.end()) {
      LOG(L_ERROR) << "Can't add dp client";
      return ;
    }
    dpclients_.push_back(dp_client);
  }

  void RemoveDpClient(DpClientImpl::Ptr dp_client) {
    DpClients::iterator pos = std::find(dpclients_.begin(),
                                        dpclients_.end(),
                                        dp_client);
    if (pos == dpclients_.end()) {
      LOG(L_ERROR) << "the dpclient is not find";
      return ;
    }
    dpclients_.erase(pos);
  }

  bool ProcessDpMessage(DpClientImpl::Ptr dp_client,
                        DpMessage::Ptr dp_msg) {
    for (DpClients::iterator iter = dpclients_.begin();
         iter != dpclients_.end(); iter++) {
      if ((*iter) == dp_client) {
        continue;
      }
      (*iter)->HandleDpMessage(dp_client, dp_msg);
    }
    return true;
  }

  bool ProcessDpRequest(DpClientImpl::Ptr dp_client,
                        DpMessage::Ptr dp_msg) {
    for (DpClients::iterator iter = dpclients_.begin();
         iter != dpclients_.end(); iter++) {
      if ((*iter) == dp_client) {
        continue;
      }
      if ((*iter)->HandleDpMessage(dp_client, dp_msg)) {
        return true;
      }
    }
    return false;
  }

  bool ProcessDpReply(DpClientImpl::Ptr dp_client,
                      DpMessage::Ptr dp_msg) {
    dp_msg->signal_event->TriggerSignal();
  }
 private:
  typedef std::list<DpClientImpl::Ptr> DpClients;
  DpClients               dpclients_;
  vzes::CriticalSection   crit_;
 public:
  static DpClientManager  *instance_;
};

DpClientManager *DpClientManager::instance_ = NULL;

DpClientManager *GetDpServiceInstance() {
  if (DpClientManager::instance_ == NULL) {
    DpClientManager::instance_ = new DpClientManager();
  }
  return DpClientManager::instance_;
}


////////////////////////////////////////////////////////////////////////////////

DpClient::Ptr DpClient::CreateDpClient(EventService::Ptr es) {
  ASSERT_RETURN_FAILURE(!es, DpClient::Ptr());
  DpClientImpl::Ptr client(new DpClientImpl(es));
  client->InitDpClient();
  return client;
}

void DpClient::DestoryDpClient(DpClient::Ptr dp_client) {
  ASSERT_RETURN_VOID(!dp_client);
  DpClientImpl::Ptr client =
    boost::dynamic_pointer_cast<DpClientImpl>(dp_client);
  client->UnregisterDpClient();
}

void DpClientImpl::RegisterDpClient() {
  vzes::CritScope cr(&crit_);
  client_manager_->AddDpClient(shared_from_this());
}

void DpClientImpl::UnregisterDpClient() {
  vzes::CritScope cr(&crit_);
  client_manager_->RemoveDpClient(shared_from_this());

}

bool DpClientImpl::SendDpMessage(const std::string method,
                                 uint32 session_id,
                                 const char *data,
                                 uint32 data_size) {
  // vzes::CritScope cr(&crit_);
  DpMessage::Ptr dp_msg(new DpMessage);
  dp_msg->method      = method;
  dp_msg->session_id  = session_id;
  dp_msg->type        = TYPE_MESSAGE;
  dp_msg->req_buffer.append(data, data_size);
  dp_msg->signal_event = signal_event_;
  return client_manager_->ProcessDpMessage(shared_from_this(), dp_msg);
}

bool DpClientImpl::SendDpRequest(const std::string method,
                                 uint32 session_id,
                                 const char *data,
                                 uint32 data_size,
                                 DpBuffer *res_buffer,
                                 uint32 timeout_millisecond) {
  // vzes::CritScope cr(&crit_);
  DpMessage::Ptr dp_msg(new DpMessage);
  dp_msg->method      = method;
  dp_msg->session_id  = session_id;
  dp_msg->type = TYPE_REQUEST;
  dp_msg->req_buffer.append(data, data_size);
  dp_msg->res_buffer  = res_buffer;
  dp_msg->signal_event      = signal_event_;
  if (!client_manager_->ProcessDpRequest(shared_from_this(), dp_msg)) {
    LOG(L_ERROR) << "No model service for this method";
    return false;
  }
  int res = dp_msg->signal_event->WaitSignal(timeout_millisecond);
  if (res != SIGNAL_EVENT_DONE) {
    return false;
  }
  return true;
}

}

