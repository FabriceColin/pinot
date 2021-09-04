#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <glibmm.h>
#include <giomm.h>
#include "PinotDBus_common.h"

namespace org {
namespace freedesktop {
namespace DBus {

class IntrospectableProxy : public Glib::ObjectBase {
public:
    static void createForBus(Gio::DBus::BusType busType,
                             Gio::DBus::ProxyFlags proxyFlags,
                             const std::string &name,
                             const std::string &objectPath,
                             const Gio::SlotAsyncReady &slot,
                             const Glib::RefPtr<Gio::Cancellable> &cancellable = {});

    static Glib::RefPtr<IntrospectableProxy> createForBusFinish (const Glib::RefPtr<Gio::AsyncResult> &result);

    static Glib::RefPtr<IntrospectableProxy> createForBus_sync(
        Gio::DBus::BusType busType,
        Gio::DBus::ProxyFlags proxyFlags,
        const std::string &name,
        const std::string &objectPath,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {});

    Glib::RefPtr<Gio::DBus::Proxy> dbusProxy() const { return m_proxy; }

    void Introspect(
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Introspect_finish (
        Glib::ustring &data,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    Glib::ustring
    Introspect_sync(
const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);


    void reference() const override {}
    void unreference() const override {}

protected:
    Glib::RefPtr<Gio::DBus::Proxy> m_proxy;

private:
    IntrospectableProxy(const Glib::RefPtr<Gio::DBus::Proxy> &proxy);

    void handle_signal(const Glib::ustring &sender_name,
                       const Glib::ustring &signal_name,
                       const Glib::VariantContainerBase &parameters);

    void handle_properties_changed(const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
                                   const std::vector<Glib::ustring> &invalidated_properties);

};

}// DBus
}// freedesktop
}// org
namespace com {
namespace github {
namespace fabricecolin {

class PinotProxy : public Glib::ObjectBase {
public:
    static void createForBus(Gio::DBus::BusType busType,
                             Gio::DBus::ProxyFlags proxyFlags,
                             const std::string &name,
                             const std::string &objectPath,
                             const Gio::SlotAsyncReady &slot,
                             const Glib::RefPtr<Gio::Cancellable> &cancellable = {});

    static Glib::RefPtr<PinotProxy> createForBusFinish (const Glib::RefPtr<Gio::AsyncResult> &result);

    static Glib::RefPtr<PinotProxy> createForBus_sync(
        Gio::DBus::BusType busType,
        Gio::DBus::ProxyFlags proxyFlags,
        const std::string &name,
        const std::string &objectPath,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {});

    Glib::RefPtr<Gio::DBus::Proxy> dbusProxy() const { return m_proxy; }

    void GetStatistics(
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetStatistics_finish (
        guint32 &crawledCount,
        guint32 &docsCount,
        bool &lowDiskSpace,
        bool &onBattery,
        bool &crawling,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::tuple<guint32, guint32, bool, bool, bool>
    GetStatistics_sync(
const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Reload(
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Reload_finish (
        bool &reloading,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    bool
    Reload_sync(
const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Stop(
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Stop_finish (
        gint32 &exitStatus,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    gint32
    Stop_sync(
const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void HasDocument(
        const Glib::ustring & url,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void HasDocument_finish (
        guint32 &docId,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    guint32
    HasDocument_sync(
        const Glib::ustring & url,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetLabels(
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetLabels_finish (
        std::vector<Glib::ustring> &labels,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::vector<Glib::ustring>
    GetLabels_sync(
const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void AddLabel(
        const Glib::ustring & label,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void AddLabel_finish (
        Glib::ustring &label,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    Glib::ustring
    AddLabel_sync(
        const Glib::ustring & label,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void DeleteLabel(
        const Glib::ustring & label,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void DeleteLabel_finish (
        Glib::ustring &label,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    Glib::ustring
    DeleteLabel_sync(
        const Glib::ustring & label,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetDocumentLabels(
        guint32 docId,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetDocumentLabels_finish (
        std::vector<Glib::ustring> &labels,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::vector<Glib::ustring>
    GetDocumentLabels_sync(
        guint32 docId,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentLabels(
        guint32 docId,
        const std::vector<Glib::ustring> & labels,
        bool resetLabels,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentLabels_finish (
        guint32 &docId,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    guint32
    SetDocumentLabels_sync(
        guint32 docId,        const std::vector<Glib::ustring> & labels,        bool resetLabels,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentsLabels(
        const std::vector<Glib::ustring> & docIds,
        const std::vector<Glib::ustring> & labels,
        bool resetLabels,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentsLabels_finish (
        bool &status,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    bool
    SetDocumentsLabels_sync(
        const std::vector<Glib::ustring> & docIds,        const std::vector<Glib::ustring> & labels,        bool resetLabels,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetDocumentInfo(
        guint32 docId,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void GetDocumentInfo_finish (
        std::vector<std::tuple<Glib::ustring,Glib::ustring>> &fields,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::vector<std::tuple<Glib::ustring,Glib::ustring>>
    GetDocumentInfo_sync(
        guint32 docId,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentInfo(
        guint32 docId,
        const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & fields,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SetDocumentInfo_finish (
        guint32 &docId,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    guint32
    SetDocumentInfo_sync(
        guint32 docId,        const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & fields,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Query(
        const Glib::ustring & engineType,
        const Glib::ustring & engineName,
        const Glib::ustring & searchText,
        guint32 startDoc,
        guint32 maxHits,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void Query_finish (
        guint32 &estimatedHits,
        std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> &hitsList,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::tuple<guint32, std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>>
    Query_sync(
        const Glib::ustring & engineType,        const Glib::ustring & engineName,        const Glib::ustring & searchText,        guint32 startDoc,        guint32 maxHits,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SimpleQuery(
        const Glib::ustring & searchText,
        guint32 maxHits,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void SimpleQuery_finish (
        std::vector<Glib::ustring> &docIds,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    std::vector<Glib::ustring>
    SimpleQuery_sync(
        const Glib::ustring & searchText,        guint32 maxHits,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void UpdateDocument(
        guint32 docId,
        const Gio::SlotAsyncReady &slot,
        const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    void UpdateDocument_finish (
        guint32 &docId,
        const Glib::RefPtr<Gio::AsyncResult> &res);

    guint32
    UpdateDocument_sync(
        guint32 docId,const Glib::RefPtr<Gio::Cancellable> &cancellable = {},
        int timeout_msec = -1);

    sigc::signal<void, guint32 > IndexFlushed_signal;

    void reference() const override {}
    void unreference() const override {}

protected:
    Glib::RefPtr<Gio::DBus::Proxy> m_proxy;

private:
    PinotProxy(const Glib::RefPtr<Gio::DBus::Proxy> &proxy);

    void handle_signal(const Glib::ustring &sender_name,
                       const Glib::ustring &signal_name,
                       const Glib::VariantContainerBase &parameters);

    void handle_properties_changed(const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
                                   const std::vector<Glib::ustring> &invalidated_properties);

};

}// fabricecolin
}// github
}// com

