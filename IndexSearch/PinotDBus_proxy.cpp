/*
 * Generated by gdbus-codegen-glibmm 2.42.0. DO NOT EDIT.
 *
 * The license of this code is the same as for the source it was derived from.
 */

#include "PinotDBus_proxy.h"

#include <utility>

template<class T>
inline T specialGetter(Glib::Variant<T> variant)
{
    return variant.get();
}

template<>
inline std::string specialGetter(Glib::Variant<std::string> variant)
{
    // String is not guaranteed to be null-terminated, so don't use ::get()
    gsize n_elem;
    gsize elem_size = sizeof(char);
    char* data = (char*)g_variant_get_fixed_array(variant.gobj(), &n_elem, elem_size);

    return std::string(data, n_elem);
}

void org::freedesktop::DBus::IntrospectableProxy::Introspect(
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    m_proxy->call("Introspect", callback, cancellable, base, timeout_msec);
}

void org::freedesktop::DBus::IntrospectableProxy::Introspect_finish(
    Glib::ustring &out_data,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<Glib::ustring> out_data_v;
    wrapped.get_child(out_data_v, 0);
    out_data = out_data_v.get();
}

Glib::ustring
org::freedesktop::DBus::IntrospectableProxy::Introspect_sync(
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("Introspect", cancellable, base, timeout_msec);

    Glib::ustring out_data;
    Glib::Variant<Glib::ustring> out_data_v;
    wrapped.get_child(out_data_v, 0);
    out_data = out_data_v.get();
    return out_data;
}

void org::freedesktop::DBus::IntrospectableProxy::handle_signal(const Glib::ustring&/* sender_name */,
    const Glib::ustring& signal_name,
    const Glib::VariantContainerBase& parameters)
{
    static_cast<void>(signal_name); // maybe unused
    static_cast<void>(parameters); // maybe unused

}

void org::freedesktop::DBus::IntrospectableProxy::handle_properties_changed(
    const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
    const std::vector<Glib::ustring> &/* invalidated_properties */)
{
    static_cast<void>(changed_properties); // maybe unused

    // Only check changed_properties since value will already be cached. Glib can be setup to get
    // values of invalidated properties in which case property will be in changed_properties when
    // value is actually received. See Gio::DBus::ProxyFlags::PROXY_FLAGS_GET_INVALIDATED_PROPERTIES .

}

org::freedesktop::DBus::IntrospectableProxy::IntrospectableProxy(const Glib::RefPtr<Gio::DBus::Proxy> &proxy) : m_proxy(proxy)
{
    m_proxy->signal_signal().connect(sigc::mem_fun(this, &IntrospectableProxy::handle_signal));
    m_proxy->signal_properties_changed().
        connect(sigc::mem_fun(this, &IntrospectableProxy::handle_properties_changed));
}

void org::freedesktop::DBus::IntrospectableProxy::createForBus(
    Gio::DBus::BusType busType,
    Gio::DBus::ProxyFlags proxyFlags,
    const std::string &name,
    const std::string &objectPath,
    const Gio::SlotAsyncReady &slot,
    const Glib::RefPtr<Gio::Cancellable> &cancellable)
{
    Gio::DBus::Proxy::create_for_bus(busType,
        name,
        objectPath,
        "org.freedesktop.DBus.Introspectable",
        slot,
        cancellable,
        Glib::RefPtr<Gio::DBus::InterfaceInfo>(),
        proxyFlags);
}

Glib::RefPtr<org::freedesktop::DBus::IntrospectableProxy> org::freedesktop::DBus::IntrospectableProxy::createForBusFinish(const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::RefPtr<Gio::DBus::Proxy> proxy =
        Gio::DBus::Proxy::create_for_bus_finish(result);
    org::freedesktop::DBus::IntrospectableProxy *p =
        new org::freedesktop::DBus::IntrospectableProxy(proxy);
    return Glib::RefPtr<org::freedesktop::DBus::IntrospectableProxy>(p);
}

Glib::RefPtr<org::freedesktop::DBus::IntrospectableProxy> org::freedesktop::DBus::IntrospectableProxy::createForBus_sync(
    Gio::DBus::BusType busType,
    Gio::DBus::ProxyFlags proxyFlags,
    const std::string &name,
    const std::string &objectPath,
    const Glib::RefPtr<Gio::Cancellable> &cancellable)
{
    Glib::RefPtr<Gio::DBus::Proxy> proxy =
        Gio::DBus::Proxy::create_for_bus_sync(busType,
            name,
            objectPath,
            "org.freedesktop.DBus.Introspectable",
            cancellable,
            Glib::RefPtr<Gio::DBus::InterfaceInfo>(),
            proxyFlags);
    org::freedesktop::DBus::IntrospectableProxy *p =
        new org::freedesktop::DBus::IntrospectableProxy(proxy);
    return Glib::RefPtr<org::freedesktop::DBus::IntrospectableProxy>(p);
}/**
 * Retrieves statistics.
 *  crawledCount: the number of documents crawled
 *  docsCount: the number of documents in the index
 */
void com::github::fabricecolin::PinotProxy::GetStatistics(
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    m_proxy->call("GetStatistics", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::GetStatistics_finish(
    guint32 &out_crawledCount,
    guint32 &out_docsCount,
    bool &out_lowDiskSpace,
    bool &out_onBattery,
    bool &out_crawling,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_crawledCount_v;
    wrapped.get_child(out_crawledCount_v, 0);
    out_crawledCount = out_crawledCount_v.get();

    Glib::Variant<guint32> out_docsCount_v;
    wrapped.get_child(out_docsCount_v, 1);
    out_docsCount = out_docsCount_v.get();

    Glib::Variant<bool> out_lowDiskSpace_v;
    wrapped.get_child(out_lowDiskSpace_v, 2);
    out_lowDiskSpace = out_lowDiskSpace_v.get();

    Glib::Variant<bool> out_onBattery_v;
    wrapped.get_child(out_onBattery_v, 3);
    out_onBattery = out_onBattery_v.get();

    Glib::Variant<bool> out_crawling_v;
    wrapped.get_child(out_crawling_v, 4);
    out_crawling = out_crawling_v.get();
}

std::tuple<guint32, guint32, bool, bool, bool>
com::github::fabricecolin::PinotProxy::GetStatistics_sync(
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("GetStatistics", cancellable, base, timeout_msec);

    guint32 out_crawledCount;
    Glib::Variant<guint32> out_crawledCount_v;
    wrapped.get_child(out_crawledCount_v, 0);
    out_crawledCount = out_crawledCount_v.get();

    guint32 out_docsCount;
    Glib::Variant<guint32> out_docsCount_v;
    wrapped.get_child(out_docsCount_v, 1);
    out_docsCount = out_docsCount_v.get();

    bool out_lowDiskSpace;
    Glib::Variant<bool> out_lowDiskSpace_v;
    wrapped.get_child(out_lowDiskSpace_v, 2);
    out_lowDiskSpace = out_lowDiskSpace_v.get();

    bool out_onBattery;
    Glib::Variant<bool> out_onBattery_v;
    wrapped.get_child(out_onBattery_v, 3);
    out_onBattery = out_onBattery_v.get();

    bool out_crawling;
    Glib::Variant<bool> out_crawling_v;
    wrapped.get_child(out_crawling_v, 4);
    out_crawling = out_crawling_v.get();

    return std::make_tuple(
        std::move(out_crawledCount),
        std::move(out_docsCount),
        std::move(out_lowDiskSpace),
        std::move(out_onBattery),
        std::move(out_crawling)
    );
}

/**
 * Instructs the daemon program to reload the configuration file.
 *  reloading: TRUE if the configuration is being reloaded
 */
void com::github::fabricecolin::PinotProxy::Reload(
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    m_proxy->call("Reload", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::Reload_finish(
    bool &out_reloading,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<bool> out_reloading_v;
    wrapped.get_child(out_reloading_v, 0);
    out_reloading = out_reloading_v.get();
}

bool
com::github::fabricecolin::PinotProxy::Reload_sync(
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("Reload", cancellable, base, timeout_msec);

    bool out_reloading;
    Glib::Variant<bool> out_reloading_v;
    wrapped.get_child(out_reloading_v, 0);
    out_reloading = out_reloading_v.get();
    return out_reloading;
}

/**
 * Stops the daemon program.
 *  exitStatus: the daemon's exit status
 */
void com::github::fabricecolin::PinotProxy::Stop(
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    m_proxy->call("Stop", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::Stop_finish(
    gint32 &out_exitStatus,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<gint32> out_exitStatus_v;
    wrapped.get_child(out_exitStatus_v, 0);
    out_exitStatus = out_exitStatus_v.get();
}

gint32
com::github::fabricecolin::PinotProxy::Stop_sync(
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("Stop", cancellable, base, timeout_msec);

    gint32 out_exitStatus;
    Glib::Variant<gint32> out_exitStatus_v;
    wrapped.get_child(out_exitStatus_v, 0);
    out_exitStatus = out_exitStatus_v.get();
    return out_exitStatus;
}

/**
 * Checks if a URL is indexed.
 *  docId: the document's ID
 */
void com::github::fabricecolin::PinotProxy::HasDocument(
    const Glib::ustring & arg_url,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::HasDocument_pack(
        arg_url);

    m_proxy->call("HasDocument", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::HasDocument_finish(
    guint32 &out_docId,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
}

guint32
com::github::fabricecolin::PinotProxy::HasDocument_sync(
    const Glib::ustring & arg_url,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::HasDocument_pack(
        arg_url);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("HasDocument", cancellable, base, timeout_msec);

    guint32 out_docId;
    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
    return out_docId;
}

/**
 * Gets the list of known labels.
 *  labels: array of labels
 */
void com::github::fabricecolin::PinotProxy::GetLabels(
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    m_proxy->call("GetLabels", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::GetLabels_finish(
    std::vector<Glib::ustring> &out_labels,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<std::vector<Glib::ustring>> out_labels_v;
    wrapped.get_child(out_labels_v, 0);
    out_labels = out_labels_v.get();
}

std::vector<Glib::ustring>
com::github::fabricecolin::PinotProxy::GetLabels_sync(
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("GetLabels", cancellable, base, timeout_msec);

    std::vector<Glib::ustring> out_labels;
    Glib::Variant<std::vector<Glib::ustring>> out_labels_v;
    wrapped.get_child(out_labels_v, 0);
    out_labels = out_labels_v.get();
    return out_labels;
}

/**
 * Adds a label.
 *  label: the name of the new label
 *        This method shouldn't be used by applications other than Pinot.
 */
void com::github::fabricecolin::PinotProxy::AddLabel(
    const Glib::ustring & arg_label,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::AddLabel_pack(
        arg_label);

    m_proxy->call("AddLabel", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::AddLabel_finish(
    Glib::ustring &out_label,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<Glib::ustring> out_label_v;
    wrapped.get_child(out_label_v, 0);
    out_label = out_label_v.get();
}

Glib::ustring
com::github::fabricecolin::PinotProxy::AddLabel_sync(
    const Glib::ustring & arg_label,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::AddLabel_pack(
        arg_label);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("AddLabel", cancellable, base, timeout_msec);

    Glib::ustring out_label;
    Glib::Variant<Glib::ustring> out_label_v;
    wrapped.get_child(out_label_v, 0);
    out_label = out_label_v.get();
    return out_label;
}

/**
 * Deletes a label.
 *  label: the name of the label to delete
 *        This method shouldn't be used by applications other than Pinot.
 */
void com::github::fabricecolin::PinotProxy::DeleteLabel(
    const Glib::ustring & arg_label,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::DeleteLabel_pack(
        arg_label);

    m_proxy->call("DeleteLabel", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::DeleteLabel_finish(
    Glib::ustring &out_label,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<Glib::ustring> out_label_v;
    wrapped.get_child(out_label_v, 0);
    out_label = out_label_v.get();
}

Glib::ustring
com::github::fabricecolin::PinotProxy::DeleteLabel_sync(
    const Glib::ustring & arg_label,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::DeleteLabel_pack(
        arg_label);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("DeleteLabel", cancellable, base, timeout_msec);

    Glib::ustring out_label;
    Glib::Variant<Glib::ustring> out_label_v;
    wrapped.get_child(out_label_v, 0);
    out_label = out_label_v.get();
    return out_label;
}

/**
 * Retrieves a document's labels.
 *  docId: the document's ID
 *  labels: array of labels applied to the document
 */
void com::github::fabricecolin::PinotProxy::GetDocumentLabels(
    guint32 arg_docId,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::GetDocumentLabels_pack(
        arg_docId);

    m_proxy->call("GetDocumentLabels", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::GetDocumentLabels_finish(
    std::vector<Glib::ustring> &out_labels,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<std::vector<Glib::ustring>> out_labels_v;
    wrapped.get_child(out_labels_v, 0);
    out_labels = out_labels_v.get();
}

std::vector<Glib::ustring>
com::github::fabricecolin::PinotProxy::GetDocumentLabels_sync(
    guint32 arg_docId,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::GetDocumentLabels_pack(
        arg_docId);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("GetDocumentLabels", cancellable, base, timeout_msec);

    std::vector<Glib::ustring> out_labels;
    Glib::Variant<std::vector<Glib::ustring>> out_labels_v;
    wrapped.get_child(out_labels_v, 0);
    out_labels = out_labels_v.get();
    return out_labels;
}

/**
 * Sets a document's labels.
 *  docId: the document's ID
 *  labels: array of labels to apply to the document
 *  resetLabels: TRUE if existing labels should be unset
 */
void com::github::fabricecolin::PinotProxy::SetDocumentLabels(
    guint32 arg_docId,
    const std::vector<Glib::ustring> & arg_labels,
    bool arg_resetLabels,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentLabels_pack(
        arg_docId,
        arg_labels,
        arg_resetLabels);

    m_proxy->call("SetDocumentLabels", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::SetDocumentLabels_finish(
    guint32 &out_docId,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
}

guint32
com::github::fabricecolin::PinotProxy::SetDocumentLabels_sync(
    guint32 arg_docId,
    const std::vector<Glib::ustring> & arg_labels,
    bool arg_resetLabels,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentLabels_pack(
        arg_docId,
        arg_labels,
        arg_resetLabels);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("SetDocumentLabels", cancellable, base, timeout_msec);

    guint32 out_docId;
    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
    return out_docId;
}

/**
 * Sets labels on a group of documents.
 *  docIds: array of document IDs
 *  labels: array of labels to apply to the documents
 *  resetLabels: TRUE if existing labels should be unset
 */
void com::github::fabricecolin::PinotProxy::SetDocumentsLabels(
    const std::vector<Glib::ustring> & arg_docIds,
    const std::vector<Glib::ustring> & arg_labels,
    bool arg_resetLabels,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentsLabels_pack(
        arg_docIds,
        arg_labels,
        arg_resetLabels);

    m_proxy->call("SetDocumentsLabels", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::SetDocumentsLabels_finish(
    bool &out_status,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<bool> out_status_v;
    wrapped.get_child(out_status_v, 0);
    out_status = out_status_v.get();
}

bool
com::github::fabricecolin::PinotProxy::SetDocumentsLabels_sync(
    const std::vector<Glib::ustring> & arg_docIds,
    const std::vector<Glib::ustring> & arg_labels,
    bool arg_resetLabels,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentsLabels_pack(
        arg_docIds,
        arg_labels,
        arg_resetLabels);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("SetDocumentsLabels", cancellable, base, timeout_msec);

    bool out_status;
    Glib::Variant<bool> out_status_v;
    wrapped.get_child(out_status_v, 0);
    out_status = out_status_v.get();
    return out_status;
}

/**
 * Retrieves information about a document.
 *  docId: the document's ID
 *  fields : array of (s name, s value) structures with name one of
 *  "caption", "url", "type", "language", "modtime", "size", "extract"
 */
void com::github::fabricecolin::PinotProxy::GetDocumentInfo(
    guint32 arg_docId,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::GetDocumentInfo_pack(
        arg_docId);

    m_proxy->call("GetDocumentInfo", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::GetDocumentInfo_finish(
    std::vector<std::tuple<Glib::ustring,Glib::ustring>> &out_fields,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> out_fields_v;
    wrapped.get_child(out_fields_v, 0);
    out_fields = out_fields_v.get();
}

std::vector<std::tuple<Glib::ustring,Glib::ustring>>
com::github::fabricecolin::PinotProxy::GetDocumentInfo_sync(
    guint32 arg_docId,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::GetDocumentInfo_pack(
        arg_docId);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("GetDocumentInfo", cancellable, base, timeout_msec);

    std::vector<std::tuple<Glib::ustring,Glib::ustring>> out_fields;
    Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> out_fields_v;
    wrapped.get_child(out_fields_v, 0);
    out_fields = out_fields_v.get();
    return out_fields;
}

/**
 * Sets information about a document.
 *  docId: the document's ID
 *  fields : array of (s name, s value) structures with name one of
 *  "caption", "url", "type", "language", "modtime", "size", "extract"
 */
void com::github::fabricecolin::PinotProxy::SetDocumentInfo(
    guint32 arg_docId,
    const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & arg_fields,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentInfo_pack(
        arg_docId,
        arg_fields);

    m_proxy->call("SetDocumentInfo", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::SetDocumentInfo_finish(
    guint32 &out_docId,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
}

guint32
com::github::fabricecolin::PinotProxy::SetDocumentInfo_sync(
    guint32 arg_docId,
    const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & arg_fields,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SetDocumentInfo_pack(
        arg_docId,
        arg_fields);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("SetDocumentInfo", cancellable, base, timeout_msec);

    guint32 out_docId;
    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
    return out_docId;
}

/**
 * Queries the index.
 *  engineType : engine type (defaults to "xapian"). See pinot-search(1) for a list of supported types
 *  engineName : engine name (defaults to "~/.pinot/daemon"). See pinot-search(1) for examples
 *  searchText : search text, as would be entered in Pinot's live query field
 *  startDoc: the first result to return, starting from 0
 *  maxHits: the maximum number of hits desired
 *  estimatedHits: an estimate of the total number of hits
 *  hitsList: hit properties
 */
void com::github::fabricecolin::PinotProxy::Query(
    const Glib::ustring & arg_engineType,
    const Glib::ustring & arg_engineName,
    const Glib::ustring & arg_searchText,
    guint32 arg_startDoc,
    guint32 arg_maxHits,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::Query_pack(
        arg_engineType,
        arg_engineName,
        arg_searchText,
        arg_startDoc,
        arg_maxHits);

    m_proxy->call("Query", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::Query_finish(
    guint32 &out_estimatedHits,
    std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> &out_hitsList,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_estimatedHits_v;
    wrapped.get_child(out_estimatedHits_v, 0);
    out_estimatedHits = out_estimatedHits_v.get();

    Glib::Variant<std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>> out_hitsList_v;
    wrapped.get_child(out_hitsList_v, 1);
    out_hitsList = out_hitsList_v.get();
}

std::tuple<guint32, std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>>
com::github::fabricecolin::PinotProxy::Query_sync(
    const Glib::ustring & arg_engineType,
    const Glib::ustring & arg_engineName,
    const Glib::ustring & arg_searchText,
    guint32 arg_startDoc,
    guint32 arg_maxHits,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::Query_pack(
        arg_engineType,
        arg_engineName,
        arg_searchText,
        arg_startDoc,
        arg_maxHits);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("Query", cancellable, base, timeout_msec);

    guint32 out_estimatedHits;
    Glib::Variant<guint32> out_estimatedHits_v;
    wrapped.get_child(out_estimatedHits_v, 0);
    out_estimatedHits = out_estimatedHits_v.get();

    std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> out_hitsList;
    Glib::Variant<std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>> out_hitsList_v;
    wrapped.get_child(out_hitsList_v, 1);
    out_hitsList = out_hitsList_v.get();

    return std::make_tuple(
        std::move(out_estimatedHits),
        std::move(out_hitsList)
    );
}

/**
 * Queries the index.
 *  searchText : search text, as would be entered in Pinot's live query field
 *  maxHits: the maximum number of hits desired
 *  docIds: array of document IDs
 *  docIdsCount: the number of document IDs in the array
 *        WARNING: this method is obsolete
 */
void com::github::fabricecolin::PinotProxy::SimpleQuery(
    const Glib::ustring & arg_searchText,
    guint32 arg_maxHits,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SimpleQuery_pack(
        arg_searchText,
        arg_maxHits);

    m_proxy->call("SimpleQuery", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::SimpleQuery_finish(
    std::vector<Glib::ustring> &out_docIds,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<std::vector<Glib::ustring>> out_docIds_v;
    wrapped.get_child(out_docIds_v, 0);
    out_docIds = out_docIds_v.get();
}

std::vector<Glib::ustring>
com::github::fabricecolin::PinotProxy::SimpleQuery_sync(
    const Glib::ustring & arg_searchText,
    guint32 arg_maxHits,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::SimpleQuery_pack(
        arg_searchText,
        arg_maxHits);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("SimpleQuery", cancellable, base, timeout_msec);

    std::vector<Glib::ustring> out_docIds;
    Glib::Variant<std::vector<Glib::ustring>> out_docIds_v;
    wrapped.get_child(out_docIds_v, 0);
    out_docIds = out_docIds_v.get();
    return out_docIds;
}

/**
 * Updates a document.
 *  docId: the document's ID
 */
void com::github::fabricecolin::PinotProxy::UpdateDocument(
    guint32 arg_docId,
    const Gio::SlotAsyncReady &callback,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::UpdateDocument_pack(
        arg_docId);

    m_proxy->call("UpdateDocument", callback, cancellable, base, timeout_msec);
}

void com::github::fabricecolin::PinotProxy::UpdateDocument_finish(
    guint32 &out_docId,
    const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_finish(result);

    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
}

guint32
com::github::fabricecolin::PinotProxy::UpdateDocument_sync(
    guint32 arg_docId,
    const Glib::RefPtr<Gio::Cancellable> &cancellable,
    int timeout_msec)
{
    Glib::VariantContainerBase base;
    base = PinotTypeWrap::UpdateDocument_pack(
        arg_docId);

    Glib::VariantContainerBase wrapped;
    wrapped = m_proxy->call_sync("UpdateDocument", cancellable, base, timeout_msec);

    guint32 out_docId;
    Glib::Variant<guint32> out_docId_v;
    wrapped.get_child(out_docId_v, 0);
    out_docId = out_docId_v.get();
    return out_docId;
}

void com::github::fabricecolin::PinotProxy::handle_signal(const Glib::ustring&/* sender_name */,
    const Glib::ustring& signal_name,
    const Glib::VariantContainerBase& parameters)
{
    static_cast<void>(signal_name); // maybe unused
    static_cast<void>(parameters); // maybe unused

    if (signal_name == "IndexFlushed") {
        if (parameters.get_n_children() != 1) return;
        Glib::Variant<guint32> base_docsCount;
        parameters.get_child(base_docsCount, 0);
        guint32 p_docsCount;
        p_docsCount = base_docsCount.get();

        IndexFlushed_signal.emit((p_docsCount));
    }
}

void com::github::fabricecolin::PinotProxy::handle_properties_changed(
    const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
    const std::vector<Glib::ustring> &/* invalidated_properties */)
{
    static_cast<void>(changed_properties); // maybe unused

    // Only check changed_properties since value will already be cached. Glib can be setup to get
    // values of invalidated properties in which case property will be in changed_properties when
    // value is actually received. See Gio::DBus::ProxyFlags::PROXY_FLAGS_GET_INVALIDATED_PROPERTIES .

}

com::github::fabricecolin::PinotProxy::PinotProxy(const Glib::RefPtr<Gio::DBus::Proxy> &proxy) : m_proxy(proxy)
{
    m_proxy->signal_signal().connect(sigc::mem_fun(this, &PinotProxy::handle_signal));
    m_proxy->signal_properties_changed().
        connect(sigc::mem_fun(this, &PinotProxy::handle_properties_changed));
}

void com::github::fabricecolin::PinotProxy::createForBus(
    Gio::DBus::BusType busType,
    Gio::DBus::ProxyFlags proxyFlags,
    const std::string &name,
    const std::string &objectPath,
    const Gio::SlotAsyncReady &slot,
    const Glib::RefPtr<Gio::Cancellable> &cancellable)
{
    Gio::DBus::Proxy::create_for_bus(busType,
        name,
        objectPath,
        "com.github.fabricecolin.Pinot",
        slot,
        cancellable,
        Glib::RefPtr<Gio::DBus::InterfaceInfo>(),
        proxyFlags);
}

Glib::RefPtr<com::github::fabricecolin::PinotProxy> com::github::fabricecolin::PinotProxy::createForBusFinish(const Glib::RefPtr<Gio::AsyncResult> &result)
{
    Glib::RefPtr<Gio::DBus::Proxy> proxy =
        Gio::DBus::Proxy::create_for_bus_finish(result);
    com::github::fabricecolin::PinotProxy *p =
        new com::github::fabricecolin::PinotProxy(proxy);
    return Glib::RefPtr<com::github::fabricecolin::PinotProxy>(p);
}

Glib::RefPtr<com::github::fabricecolin::PinotProxy> com::github::fabricecolin::PinotProxy::createForBus_sync(
    Gio::DBus::BusType busType,
    Gio::DBus::ProxyFlags proxyFlags,
    const std::string &name,
    const std::string &objectPath,
    const Glib::RefPtr<Gio::Cancellable> &cancellable)
{
    Glib::RefPtr<Gio::DBus::Proxy> proxy =
        Gio::DBus::Proxy::create_for_bus_sync(busType,
            name,
            objectPath,
            "com.github.fabricecolin.Pinot",
            cancellable,
            Glib::RefPtr<Gio::DBus::InterfaceInfo>(),
            proxyFlags);
    com::github::fabricecolin::PinotProxy *p =
        new com::github::fabricecolin::PinotProxy(proxy);
    return Glib::RefPtr<com::github::fabricecolin::PinotProxy>(p);
}
