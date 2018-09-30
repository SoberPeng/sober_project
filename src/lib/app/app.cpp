/*
* vzsdk
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

#include "app/app.h"
#include "filecache/server/cacheserver.h"
#include "eventservice/dp/dpserver.h"

namespace app {

#define ON_MSG_PRE_INIT   1
#define ON_MSG_INIT       2
#define ON_MSG_RUN_APP    3
#define ON_MSG_EXIT       4

struct AppInstance {
  AppInterface::Ptr app;
  vzes::EventService::Ptr es;
};

struct AppData : public vzes::MessageData {
  typedef boost::shared_ptr<AppData> Ptr;
  AppInterface::Ptr       app;
  vzes::EventService::Ptr es;
  vzes::SignalEvent::Ptr  signal_event;
};

class AppImpl : public App,
  public boost::noncopyable,
  public boost::enable_shared_from_this<AppImpl>,
  public sigslot::has_slots<>,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<AppImpl> Ptr;

 public:
  AppImpl() {
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
  }
  virtual ~AppImpl() {
  }

 public:
  bool InitPublicSystem() {
    // 初始化日志库
    vzes::LogMessage::LogTimestamps(true);
    vzes::LogMessage::LogContext(vzes::LS_SENSITIVE);
    vzes::LogMessage::LogThreads(true);
    // 初始化主线程
    main_es_ = vzes::EventService::CreateCurrentEventService("vz_main_thread");
    // 初始化Filecache和KVDB，只需要调用一次就可以了
    ASSERT_RETURN_FAILURE(cache::CacheServer::Instance() == NULL, false);
    // 初始化星形结构
    ASSERT_RETURN_FAILURE(vzes::InitDpSystem() == false, false);

    // 监听事件客户端
    dp_client_ = vzes::DpClient::CreateDpClient(main_es_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);

    dp_client_->SignalDpMessage.connect(this, &AppImpl::OnDpMessage);

    dp_client_->ListenMessage(APP_RESET_SUBSYSTEM);

    return true;
  }

 public:
  virtual bool RegisterApp(AppInterface::Ptr app) {
    ASSERT_RETURN_FAILURE(!app, false);
    ASSERT_RETURN_FAILURE(!main_es_, false);
    AppInstance app_instance;
    app_instance.app = app;
    app_instance.es  = vzes::EventService::CreateEventService(NULL, app->app_name());
    apps_.push_back(app_instance);
    return true;
  }

  virtual void AppRun() {
    ASSERT_RETURN_VOID(!main_es_);
    // Run pre init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_PRE_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      LOG(L_INFO) << "PreInit message Done " << iter->app->app_name();
    }
    // run init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      LOG(L_INFO) << "Init message Done " << iter->app->app_name();
    }
    // run appwe
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_RUN_APP, app_data);
      LOG(L_INFO) << "Run message Done " << iter->app->app_name();
    }
    //
    main_es_->Run();
  }

  virtual void ExitApp() {
    vzes::SignalEvent::Ptr exit_signal = vzes::SignalEvent::CreateSignalEvent();
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      iter->es = vzes::EventService::CreateEventService();
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = exit_signal;
      iter->es->Post(this, ON_MSG_EXIT, app_data);
      exit_signal->WaitSignal(100);
      LOG(L_INFO) << "Exit Message Done " << iter->app->app_name();
    }

    LOG(L_INFO) << "App exited!!!";
    exit(0);
  }

 public:
  virtual void OnMessage(vzes::Message *msg) {
    AppData::Ptr app_data = boost::dynamic_pointer_cast<AppData>(msg->pdata);
    if (msg->message_id == ON_MSG_PRE_INIT) {
      LOG(L_INFO) << "ON_MSG_PRE_INIT";
      OnPreInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_INIT) {
      LOG(L_INFO) << "ON_MSG_INIT";
      OnInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_RUN_APP) {
      LOG(L_INFO) << "ON_MSG_RUN_APP";
      OnRunApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_EXIT) {
      LOG(L_INFO) << "ON_MSG_EXIT";
      OnExitApp(app_data->es, app_data->app, app_data->signal_event);
    } else {
      LOG(L_ERROR) << "Unkown message";
    }
  }

  void OnPreInitApp(vzes::EventService::Ptr es,
                    AppInterface::Ptr app,
                    vzes::SignalEvent::Ptr signal_event) {
    if (!app->PreInit(es)) {
      LOG(L_ERROR) << "App PreInit failed " << app->app_name();
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnInitApp(vzes::EventService::Ptr es,
                 AppInterface::Ptr app,
                 vzes::SignalEvent::Ptr signal_event) {
    if (!app->InitApp(es)) {
      LOG(L_ERROR) << "App Init failed " << app->app_name();
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnRunApp(vzes::EventService::Ptr es,
                AppInterface::Ptr app,
                vzes::SignalEvent::Ptr signal_event) {
    if (!app->RunAPP(es)) {
      LOG(L_ERROR) << "App Run failed " << app->app_name();
      ExitApp();
    }
  }

  void OnExitApp(vzes::EventService::Ptr es,
                 AppInterface::Ptr app,
                 vzes::SignalEvent::Ptr signal_event) {
    app->OnExitApp(es);
    signal_event->TriggerSignal();
  }

 protected:
  void OnDpMessage(vzes::DpClient::Ptr dp_client,
                   vzes::DpMessage::Ptr dp_msg) {
    if (0 == dp_msg->method.compare(APP_RESET_SUBSYSTEM)) {
      // 销毁所有子系统
      ExitApp();

      // 重启所有子系统
      AppRun();
    }
  }

 private:
  typedef std::list<AppInstance> Apps;

  Apps                    apps_;
  vzes::CriticalSection   crit_;
  vzes::EventService::Ptr main_es_;
  vzes::SignalEvent::Ptr  signal_event_;

 private:
  vzes::DpClient::Ptr     dp_client_;
};

App::Ptr App::CreateApp() {
  AppImpl::Ptr app(new AppImpl());
  if (app->InitPublicSystem()) {
    return app;
  }
  return App::Ptr();
}

}