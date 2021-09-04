#pragma once
#include <string>
#include <vector>
#include <glibmm.h>
#include <giomm.h>
#include "PinotDBus_common.h"

namespace org {
namespace freedesktop {
namespace DBus {

class IntrospectableStub : public sigc::trackable {
public:
    IntrospectableStub();
    virtual ~IntrospectableStub();

    IntrospectableStub(const IntrospectableStub &other) = delete;
    IntrospectableStub(IntrospectableStub &&other) = delete;
    IntrospectableStub &operator=(const IntrospectableStub &other) = delete;
    IntrospectableStub &operator=(IntrospectableStub &&other) = delete;

    guint register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &object_path);
    void unregister_object();

    unsigned int usage_count() const {
        return static_cast<unsigned int>(m_registered_objects.size());
    }

    class MethodInvocation;

protected:
    virtual void Introspect(
        MethodInvocation &invocation) = 0;

    void on_method_call(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                        const Glib::ustring &sender,
                        const Glib::ustring &object_path,
                        const Glib::ustring &interface_name,
                        const Glib::ustring &method_name,
                        const Glib::VariantContainerBase &parameters,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

    void on_interface_get_property(Glib::VariantBase& property,
                                   const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                   const Glib::ustring &sender,
                                   const Glib::ustring &object_path,
                                   const Glib::ustring &interface_name,
                                   const Glib::ustring &property_name);

    bool on_interface_set_property(
        const Glib::RefPtr<Gio::DBus::Connection> &connection,
        const Glib::ustring &sender,
        const Glib::ustring &object_path,
        const Glib::ustring &interface_name,
        const Glib::ustring &property_name,
        const Glib::VariantBase &value);

private:
    bool emitSignal(const std::string &propName, Glib::VariantBase &value);

    struct RegisteredObject {
        guint id;
        Glib::RefPtr<Gio::DBus::Connection> connection;
        std::string object_path;
    };

    Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
    std::vector<RegisteredObject> m_registered_objects;
    std::string m_interfaceName;
};

class IntrospectableStub::MethodInvocation {
public:
    MethodInvocation(const Glib::RefPtr<Gio::DBus::MethodInvocation> &msg):
        m_message(msg) {}

    const Glib::RefPtr<Gio::DBus::MethodInvocation> getMessage() {
        return m_message;
    }

    void ret(Glib::Error error) {
        m_message->return_error(error);
    }

    void returnError(const Glib::ustring &domain, int code, const Glib::ustring &message) {
        m_message->return_error(domain, code, message);
    }

    void ret(const Glib::ustring & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<Glib::ustring> var0 =
            Glib::Variant<Glib::ustring>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

private:
    Glib::RefPtr<Gio::DBus::MethodInvocation> m_message;
};

} // DBus
} // freedesktop
} // org
namespace com {
namespace github {
namespace fabricecolin {

class PinotStub : public sigc::trackable {
public:
    PinotStub();
    virtual ~PinotStub();

    PinotStub(const PinotStub &other) = delete;
    PinotStub(PinotStub &&other) = delete;
    PinotStub &operator=(const PinotStub &other) = delete;
    PinotStub &operator=(PinotStub &&other) = delete;

    guint register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &object_path);
    void unregister_object();

    unsigned int usage_count() const {
        return static_cast<unsigned int>(m_registered_objects.size());
    }

    class MethodInvocation;

protected:
    virtual void GetStatistics(
        MethodInvocation &invocation) = 0;
    virtual void Reload(
        MethodInvocation &invocation) = 0;
    virtual void Stop(
        MethodInvocation &invocation) = 0;
    virtual void HasDocument(
        const Glib::ustring & url,
        MethodInvocation &invocation) = 0;
    virtual void GetLabels(
        MethodInvocation &invocation) = 0;
    virtual void AddLabel(
        const Glib::ustring & label,
        MethodInvocation &invocation) = 0;
    virtual void DeleteLabel(
        const Glib::ustring & label,
        MethodInvocation &invocation) = 0;
    virtual void GetDocumentLabels(
        guint32 docId,
        MethodInvocation &invocation) = 0;
    virtual void SetDocumentLabels(
        guint32 docId,
        const std::vector<Glib::ustring> & labels,
        bool resetLabels,
        MethodInvocation &invocation) = 0;
    virtual void SetDocumentsLabels(
        const std::vector<Glib::ustring> & docIds,
        const std::vector<Glib::ustring> & labels,
        bool resetLabels,
        MethodInvocation &invocation) = 0;
    virtual void GetDocumentInfo(
        guint32 docId,
        MethodInvocation &invocation) = 0;
    virtual void SetDocumentInfo(
        guint32 docId,
        const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & fields,
        MethodInvocation &invocation) = 0;
    virtual void Query(
        const Glib::ustring & engineType,
        const Glib::ustring & engineName,
        const Glib::ustring & searchText,
        guint32 startDoc,
        guint32 maxHits,
        MethodInvocation &invocation) = 0;
    virtual void SimpleQuery(
        const Glib::ustring & searchText,
        guint32 maxHits,
        MethodInvocation &invocation) = 0;
    virtual void UpdateDocument(
        guint32 docId,
        MethodInvocation &invocation) = 0;

    void IndexFlushed_emitter(const std::vector<Glib::ustring> &destination_bus_names, guint32);
    sigc::signal<void, guint32> IndexFlushed_signal;
    sigc::signal<void, const std::vector<Glib::ustring>&, guint32> IndexFlushed_selectiveSignal;

    void on_method_call(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                        const Glib::ustring &sender,
                        const Glib::ustring &object_path,
                        const Glib::ustring &interface_name,
                        const Glib::ustring &method_name,
                        const Glib::VariantContainerBase &parameters,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

    void on_interface_get_property(Glib::VariantBase& property,
                                   const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                   const Glib::ustring &sender,
                                   const Glib::ustring &object_path,
                                   const Glib::ustring &interface_name,
                                   const Glib::ustring &property_name);

    bool on_interface_set_property(
        const Glib::RefPtr<Gio::DBus::Connection> &connection,
        const Glib::ustring &sender,
        const Glib::ustring &object_path,
        const Glib::ustring &interface_name,
        const Glib::ustring &property_name,
        const Glib::VariantBase &value);

private:
    bool emitSignal(const std::string &propName, Glib::VariantBase &value);

    struct RegisteredObject {
        guint id;
        Glib::RefPtr<Gio::DBus::Connection> connection;
        std::string object_path;
    };

    Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
    std::vector<RegisteredObject> m_registered_objects;
    std::string m_interfaceName;
};

class PinotStub::MethodInvocation {
public:
    MethodInvocation(const Glib::RefPtr<Gio::DBus::MethodInvocation> &msg):
        m_message(msg) {}

    const Glib::RefPtr<Gio::DBus::MethodInvocation> getMessage() {
        return m_message;
    }

    void ret(Glib::Error error) {
        m_message->return_error(error);
    }

    void returnError(const Glib::ustring &domain, int code, const Glib::ustring &message) {
        m_message->return_error(domain, code, message);
    }

    void ret(guint32 p0, guint32 p1, bool p2, bool p3, bool p4) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<guint32> var0 =
            Glib::Variant<guint32>::create(p0);
        vlist.push_back(var0);
        Glib::Variant<guint32> var1 =
            Glib::Variant<guint32>::create(p1);
        vlist.push_back(var1);
        Glib::Variant<bool> var2 =
            Glib::Variant<bool>::create(p2);
        vlist.push_back(var2);
        Glib::Variant<bool> var3 =
            Glib::Variant<bool>::create(p3);
        vlist.push_back(var3);
        Glib::Variant<bool> var4 =
            Glib::Variant<bool>::create(p4);
        vlist.push_back(var4);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(bool p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<bool> var0 =
            Glib::Variant<bool>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(gint32 p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<gint32> var0 =
            Glib::Variant<gint32>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(guint32 p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<guint32> var0 =
            Glib::Variant<guint32>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(const std::vector<Glib::ustring> & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<std::vector<Glib::ustring>> var0 =
            Glib::Variant<std::vector<Glib::ustring>>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(const Glib::ustring & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<Glib::ustring> var0 =
            Glib::Variant<Glib::ustring>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> var0 =
            Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(guint32 p0, const std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> & p1) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<guint32> var0 =
            Glib::Variant<guint32>::create(p0);
        vlist.push_back(var0);
        Glib::Variant<std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>> var1 =
            Glib::Variant<std::vector<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>>::create(p1);
        vlist.push_back(var1);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

private:
    Glib::RefPtr<Gio::DBus::MethodInvocation> m_message;
};

} // fabricecolin
} // github
} // com

