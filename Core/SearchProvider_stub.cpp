static const char interfaceXml0[] = R"XML_DELIMITER(<!DOCTYPE node PUBLIC
'-//freedesktop//DTD D-BUS Object Introspection 1.0//EN'
'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>

  <!--
      org.gnome.Shell.SearchProvider2:
      @short_description: Search provider interface

      The interface used for integrating into GNOME Shell's search
      interface (version 2).
  -->
  <interface name="org.gnome.Shell.SearchProvider2">

    <!--
        GetInitialResultSet:
        @terms: Array of search terms, which the provider should treat as logical AND.
        @results: An array of result identifier strings representing items which match the given search terms. Identifiers must be unique within the provider's domain, but other than that may be chosen freely by the provider.

        Called when the user first begins a search.
    -->
    <method name="GetInitialResultSet">
      <arg type="as" name="terms" direction="in" />
      <arg type="as" name="results" direction="out" />
    </method>

    <!--
        GetSubsearchResultSet:
        @previous_results: Array of results previously returned by GetInitialResultSet().
        @terms: Array of updated search terms, which the provider should treat as logical AND.
        @results: An array of result identifier strings representing items which match the given search terms. Identifiers must be unique within the provider's domain, but other than that may be chosen freely by the provider.

        Called when a search is performed which is a "subsearch" of
        the previous search, e.g. the method may return less results, but
        not more or different results.

        This allows search providers to only search through the previous
        result set, rather than possibly performing a full re-query.
    -->
    <method name="GetSubsearchResultSet">
      <arg type="as" name="previous_results" direction="in" />
      <arg type="as" name="terms" direction="in" />
      <arg type="as" name="results" direction="out" />
    </method>

    <!--
        GetResultMetas:
        @identifiers: An array of result identifiers as returned by GetInitialResultSet() or GetSubsearchResultSet()
        @metas: A dictionary describing the given search result, containing a human-readable 'name' (string), along with the result identifier this meta is for, 'id' (string). Optionally, 'icon' (a serialized GIcon as obtained by g_icon_serialize) can be specified if the result can be better served with a thumbnail of the content (such as with images). 'gicon' (a serialized GIcon as obtained by g_icon_to_string) or 'icon-data' (raw image data as (iiibiiay) - width, height, rowstride, has-alpha, bits per sample, channels, data) are deprecated values that can also be used for that purpose. A 'description' field (string) may also be specified if more context would help the user find the desired result.

        Return an array of meta data used to display each given result
    -->
    <method name="GetResultMetas">
      <arg type="as" name="identifiers" direction="in" />
      <arg type="aa{sv}" name="metas" direction="out" />
    </method>

    <!--
        ActivateResult:
        @identifier: A result identifier as returned by GetInitialResultSet() or GetSubsearchResultSet()
        @terms: Array of search terms, which the provider should treat as logical AND.
        @timestamp: A timestamp of the user interaction that triggered this call

        Called when the users chooses a given result. The result should
        be displayed in the application associated with the corresponding
        provider. The provided search terms can be used to allow launching a full search in
        the application.
    -->
    <method name="ActivateResult">
      <arg type="s" name="identifier" direction="in" />
      <arg type="as" name="terms" direction="in" />
      <arg type="u" name="timestamp" direction="in" />
    </method>

    <!--
        LaunchSearch:
        @terms: Array of search terms, which the provider should treat as logical AND.
        @timestamp: A timestamp of the user interaction that triggered this call

        Asks the search provider to launch a full search in the application for the provided terms.
    -->
    <method name="LaunchSearch">
      <arg type="as" name="terms" direction="in" />
      <arg type="u" name="timestamp" direction="in" />
    </method>
  </interface>
</node>
)XML_DELIMITER";

#include "SearchProvider_stub.h"

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

org::gnome::Shell::SearchProvider2Stub::SearchProvider2Stub():
    m_interfaceName("org.gnome.Shell.SearchProvider2")
{
}

org::gnome::Shell::SearchProvider2Stub::~SearchProvider2Stub()
{
    unregister_object();
}

guint org::gnome::Shell::SearchProvider2Stub::register_object(
    const Glib::RefPtr<Gio::DBus::Connection> &connection,
    const Glib::ustring &object_path)
{
    if (!introspection_data) {
        try {
            introspection_data = Gio::DBus::NodeInfo::create_for_xml(interfaceXml0);
        } catch(const Glib::Error& ex) {
            g_warning("Unable to create introspection data for %s: %s", object_path.c_str(), ex.what().c_str());
            return 0;
        }
    }

    Gio::DBus::InterfaceVTable *interface_vtable =
        new Gio::DBus::InterfaceVTable(
            sigc::mem_fun(this, &SearchProvider2Stub::on_method_call),
            sigc::mem_fun(this, &SearchProvider2Stub::on_interface_get_property),
            sigc::mem_fun(this, &SearchProvider2Stub::on_interface_set_property));

    guint registration_id;
    try {
        registration_id = connection->register_object(object_path,
            introspection_data->lookup_interface("org.gnome.Shell.SearchProvider2"),
            *interface_vtable);
    } catch(const Glib::Error &ex) {
        g_warning("Registration of object %s failed: %s", object_path.c_str(), ex.what().c_str());
        return 0;
    }

    m_registered_objects.emplace_back(RegisteredObject {
        registration_id,
        connection,
        object_path
    });

    return registration_id;
}

void org::gnome::Shell::SearchProvider2Stub::unregister_object()
{
    for (const RegisteredObject &obj: m_registered_objects) {
        obj.connection->unregister_object(obj.id);
    }
    m_registered_objects.clear();
}

void org::gnome::Shell::SearchProvider2Stub::on_method_call(
    const Glib::RefPtr<Gio::DBus::Connection> &/* connection */,
    const Glib::ustring &/* sender */,
    const Glib::ustring &/* object_path */,
    const Glib::ustring &/* interface_name */,
    const Glib::ustring &method_name,
    const Glib::VariantContainerBase &parameters,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation)
{
    static_cast<void>(method_name); // maybe unused
    static_cast<void>(parameters); // maybe unused
    static_cast<void>(invocation); // maybe unused

    if (method_name.compare("GetInitialResultSet") == 0) {
        Glib::Variant<std::vector<Glib::ustring>> base_terms;
        parameters.get_child(base_terms, 0);
        std::vector<Glib::ustring> p_terms = specialGetter(base_terms);

        MethodInvocation methodInvocation(invocation);
        GetInitialResultSet(
            (p_terms),
            methodInvocation);
    }

    if (method_name.compare("GetSubsearchResultSet") == 0) {
        Glib::Variant<std::vector<Glib::ustring>> base_previous_results;
        parameters.get_child(base_previous_results, 0);
        std::vector<Glib::ustring> p_previous_results = specialGetter(base_previous_results);

        Glib::Variant<std::vector<Glib::ustring>> base_terms;
        parameters.get_child(base_terms, 1);
        std::vector<Glib::ustring> p_terms = specialGetter(base_terms);

        MethodInvocation methodInvocation(invocation);
        GetSubsearchResultSet(
            (p_previous_results),
            (p_terms),
            methodInvocation);
    }

    if (method_name.compare("GetResultMetas") == 0) {
        Glib::Variant<std::vector<Glib::ustring>> base_identifiers;
        parameters.get_child(base_identifiers, 0);
        std::vector<Glib::ustring> p_identifiers = specialGetter(base_identifiers);

        MethodInvocation methodInvocation(invocation);
        GetResultMetas(
            (p_identifiers),
            methodInvocation);
    }

    if (method_name.compare("ActivateResult") == 0) {
        Glib::Variant<Glib::ustring> base_identifier;
        parameters.get_child(base_identifier, 0);
        Glib::ustring p_identifier = specialGetter(base_identifier);

        Glib::Variant<std::vector<Glib::ustring>> base_terms;
        parameters.get_child(base_terms, 1);
        std::vector<Glib::ustring> p_terms = specialGetter(base_terms);

        Glib::Variant<guint32> base_timestamp;
        parameters.get_child(base_timestamp, 2);
        guint32 p_timestamp = specialGetter(base_timestamp);

        MethodInvocation methodInvocation(invocation);
        ActivateResult(
            (p_identifier),
            (p_terms),
            (p_timestamp),
            methodInvocation);
    }

    if (method_name.compare("LaunchSearch") == 0) {
        Glib::Variant<std::vector<Glib::ustring>> base_terms;
        parameters.get_child(base_terms, 0);
        std::vector<Glib::ustring> p_terms = specialGetter(base_terms);

        Glib::Variant<guint32> base_timestamp;
        parameters.get_child(base_timestamp, 1);
        guint32 p_timestamp = specialGetter(base_timestamp);

        MethodInvocation methodInvocation(invocation);
        LaunchSearch(
            (p_terms),
            (p_timestamp),
            methodInvocation);
    }

}

void org::gnome::Shell::SearchProvider2Stub::on_interface_get_property(
    Glib::VariantBase &property,
    const Glib::RefPtr<Gio::DBus::Connection> &/* connection */,
    const Glib::ustring &/* sender */,
    const Glib::ustring &/* object_path */,
    const Glib::ustring &/* interface_name */,
    const Glib::ustring &property_name)
{
    static_cast<void>(property); // maybe unused
    static_cast<void>(property_name); // maybe unused

}

bool org::gnome::Shell::SearchProvider2Stub::on_interface_set_property(
    const Glib::RefPtr<Gio::DBus::Connection> &/* connection */,
    const Glib::ustring &/* sender */,
    const Glib::ustring &/* object_path */,
    const Glib::ustring &/* interface_name */,
    const Glib::ustring &property_name,
    const Glib::VariantBase &value)
{
    static_cast<void>(property_name); // maybe unused
    static_cast<void>(value); // maybe unused

    return true;
}


bool org::gnome::Shell::SearchProvider2Stub::emitSignal(
    const std::string &propName,
    Glib::VariantBase &value)
{
    std::map<Glib::ustring, Glib::VariantBase> changedProps;
    std::vector<Glib::ustring> changedPropsNoValue;

    changedProps[propName] = value;

    Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>> changedPropsVar =
        Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(changedProps);
    Glib::Variant<std::vector<Glib::ustring>> changedPropsNoValueVar =
        Glib::Variant<std::vector<Glib::ustring>>::create(changedPropsNoValue);
    std::vector<Glib::VariantBase> ps;
    ps.push_back(Glib::Variant<Glib::ustring>::create(m_interfaceName));
    ps.push_back(changedPropsVar);
    ps.push_back(changedPropsNoValueVar);
    Glib::VariantContainerBase propertiesChangedVariant =
        Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple(ps);

    for (const RegisteredObject &obj: m_registered_objects) {
        obj.connection->emit_signal(
            obj.object_path,
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            Glib::ustring(),
            propertiesChangedVariant);
    }

    return true;
}
