#pragma once
#include <string>
#include <vector>
#include <glibmm.h>
#include <giomm.h>
#include "SearchProvider_common.h"

namespace org {
namespace gnome {
namespace Shell {

class SearchProvider2Stub : public sigc::trackable {
public:
    SearchProvider2Stub();
    virtual ~SearchProvider2Stub();

    SearchProvider2Stub(const SearchProvider2Stub &other) = delete;
    SearchProvider2Stub(SearchProvider2Stub &&other) = delete;
    SearchProvider2Stub &operator=(const SearchProvider2Stub &other) = delete;
    SearchProvider2Stub &operator=(SearchProvider2Stub &&other) = delete;

    guint register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &object_path);
    void unregister_object();

    unsigned int usage_count() const {
        return static_cast<unsigned int>(m_registered_objects.size());
    }

    class MethodInvocation;

protected:
    virtual void GetInitialResultSet(
        const std::vector<Glib::ustring> & terms,
        MethodInvocation &invocation) = 0;
    virtual void GetSubsearchResultSet(
        const std::vector<Glib::ustring> & previous_results,
        const std::vector<Glib::ustring> & terms,
        MethodInvocation &invocation) = 0;
    virtual void GetResultMetas(
        const std::vector<Glib::ustring> & identifiers,
        MethodInvocation &invocation) = 0;
    virtual void ActivateResult(
        const Glib::ustring & identifier,
        const std::vector<Glib::ustring> & terms,
        guint32 timestamp,
        MethodInvocation &invocation) = 0;
    virtual void LaunchSearch(
        const std::vector<Glib::ustring> & terms,
        guint32 timestamp,
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

class SearchProvider2Stub::MethodInvocation {
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

    void ret(const std::vector<Glib::ustring> & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<std::vector<Glib::ustring>> var0 =
            Glib::Variant<std::vector<Glib::ustring>>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret(const std::vector<std::map<Glib::ustring,Glib::VariantBase>> & p0) {
        std::vector<Glib::VariantBase> vlist;
        Glib::Variant<std::vector<std::map<Glib::ustring,Glib::VariantBase>>> var0 =
            Glib::Variant<std::vector<std::map<Glib::ustring,Glib::VariantBase>>>::create(p0);
        vlist.push_back(var0);

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

    void ret() {
        std::vector<Glib::VariantBase> vlist;

        m_message->return_value(Glib::Variant<Glib::VariantBase>::create_tuple(vlist));
    }

private:
    Glib::RefPtr<Gio::DBus::MethodInvocation> m_message;
};

} // Shell
} // gnome
} // org

