static const char interfaceXml0[] = R"XML_DELIMITER(<?xml version="1.0" encoding="UTF-8" ?>
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg type="s" name="data" direction="out"/>
    </method>
  </interface>
  <!--
	WARNING: This interface is subject to change
	-->
  <interface name="com.github.fabricecolin.Pinot">
    <signal name="IndexFlushed">
      <annotation name="com.github.fabricecolin.Pinot.IndexFlushed" value="pinotDBus"/>
      <arg type="u" name="docsCount" direction="out"/>
    </signal>
    <!--
	Retrieves statistics.
	 crawledCount: the number of documents crawled
	 docsCount: the number of documents in the index
	-->
    <method name="GetStatistics">
      <annotation name="com.github.fabricecolin.Pinot.GetStatistics" value="pinotDBus"/>
      <arg type="u" name="crawledCount" direction="out"/>
      <arg type="u" name="docsCount" direction="out"/>
      <arg type="b" name="lowDiskSpace" direction="out" />
      <arg type="b" name="onBattery" direction="out" />
      <arg type="b" name="crawling" direction="out" />
    </method>
    <!--
	Instructs the daemon program to reload the configuration file.
	 reloading: TRUE if the configuration is being reloaded
	-->
    <method name="Reload">
      <annotation name="com.github.fabricecolin.Pinot.Reload" value="pinotDBus"/>
      <arg type="b" name="reloading" direction="out" />
    </method>
    <!--
	Stops the daemon program.
	 exitStatus: the daemon's exit status
	-->
    <method name="Stop">
      <annotation name="com.github.fabricecolin.Pinot.Stop" value="pinotDBus"/>
      <arg type="i" name="exitStatus" direction="out" />
    </method>
    <!--
	Checks if a URL is indexed.
	 docId: the document's ID
	-->
    <method name="HasDocument">
      <annotation name="com.github.fabricecolin.Pinot.HasDocument" value="pinotDBus"/>
      <arg type="s" name="url" direction="in"/>
      <arg type="u" name="docId" direction="out"/>
    </method>
    <!--
	Gets the list of known labels.
	 labels: array of labels
	-->
    <method name="GetLabels">
      <annotation name="com.github.fabricecolin.Pinot.GetLabels" value="pinotDBus"/>
      <arg type="as" name="labels" direction="out"/>
    </method>
    <!--
	Adds a label.
	 label: the name of the new label
        This method shouldn't be used by applications other than Pinot.
	-->
    <method name="AddLabel">
      <annotation name="com.github.fabricecolin.Pinot.AddLabel" value="pinotDBus"/>
      <arg type="s" name="label" direction="in"/>
      <arg type="s" name="label" direction="out"/>
    </method>
    <!--
	Deletes a label.
	 label: the name of the label to delete
        This method shouldn't be used by applications other than Pinot.
	-->
    <method name="DeleteLabel">
      <annotation name="com.github.fabricecolin.Pinot.DeleteLabel" value="pinotDBus"/>
      <arg type="s" name="label" direction="in"/>
      <arg type="s" name="label" direction="out"/>
    </method>
    <!--
	Retrieves a document's labels.
	 docId: the document's ID
	 labels: array of labels applied to the document
	-->
    <method name="GetDocumentLabels">
      <annotation name="com.github.fabricecolin.Pinot.GetDocumentLabels" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="as" name="labels" direction="out"/>
    </method>
    <!--
	Sets a document's labels.
	 docId: the document's ID
	 labels: array of labels to apply to the document
	 resetLabels: TRUE if existing labels should be unset
	-->
    <method name="SetDocumentLabels">
      <annotation name="com.github.fabricecolin.Pinot.SetDocumentLabels" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="as" name="labels" direction="in"/>
      <arg type="b" name="resetLabels" direction="in"/>
      <arg type="u" name="docId" direction="out"/>
    </method>
    <!--
	Sets labels on a group of documents.
	 docIds: array of document IDs
	 labels: array of labels to apply to the documents
	 resetLabels: TRUE if existing labels should be unset
	-->
    <method name="SetDocumentsLabels">
      <annotation name="com.github.fabricecolin.Pinot.SetDocumentsLabels" value="pinotDBus"/>
      <arg type="as" name="docIds" direction="in"/>
      <arg type="as" name="labels" direction="in"/>
      <arg type="b" name="resetLabels" direction="in"/>
      <arg type="b" name="status" direction="out"/>
    </method>
    <!--
	Retrieves information about a document.
	 docId: the document's ID
	 fields : array of (s name, s value) structures with name one of
	 "caption", "url", "type", "language", "modtime", "size", "extract"
	-->
    <method name="GetDocumentInfo">
      <annotation name="com.github.fabricecolin.Pinot.GetDocumentInfo" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="a(ss)" name="fields" direction="out"/>
    </method>
    <!--
	Sets information about a document.
	 docId: the document's ID
	 fields : array of (s name, s value) structures with name one of
	 "caption", "url", "type", "language", "modtime", "size", "extract"
	-->
    <method name="SetDocumentInfo">
      <annotation name="com.github.fabricecolin.Pinot.SetDocumentInfo" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="a(ss)" name="fields" direction="in"/>
      <arg type="u" name="docId" direction="out"/>
    </method>
    <!--
	Retrieves a document's terms count.
	 docId: the document's ID
	 count: the terms count
	-->
    <method name="GetDocumentTermsCount">
      <annotation name="com.github.fabricecolin.Pinot.GetDocumentTermsCount" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="u" name="count" direction="out"/>
    </method>
    <!--
	Retrieves a document's terms.
	 docId: the document's ID
	 fields : array of (s name, s value) structures with name one of
	 "caption", "url", "type", "language", "modtime", "size", "extract"
	-->
    <method name="GetDocumentTerms">
      <annotation name="com.github.fabricecolin.Pinot.GetDocumentTerms" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="as" name="terms" direction="out"/>
    </method>
    <!--
	Queries the index.
	 engineType : engine type (defaults to "xapian"). See pinot-search(1) for a list of supported types
	 engineName : engine name (defaults to "~/.pinot/daemon"). See pinot-search(1) for examples
	 searchText : search text, as would be entered in Pinot's live query field
	 startDoc: the first result to return, starting from 0
	 maxHits: the maximum number of hits desired
	 estimatedHits: an estimate of the total number of hits
	 hitsList: hit properties
	-->
    <method name="Query">
      <annotation name="com.github.fabricecolin.Pinot.Query" value="pinotDBus"/>
      <arg type="s" name="engineType" direction="in" />
      <arg type="s" name="engineName" direction="in" />
      <arg type="s" name="searchText" direction="in" />
      <arg type="u" name="startDoc" direction="in" />
      <arg type="u" name="maxHits" direction="in" />
      <arg type="u" name="estimatedHits" direction="out" />
      <arg type="aa(ss)" name="hitsList" direction="out" />
    </method>
    <!--
	Queries the index.
	 searchText : search text, as would be entered in Pinot's live query field
	 maxHits: the maximum number of hits desired
	 docIds: array of document IDs
	 docIdsCount: the number of document IDs in the array
        WARNING: this method is obsolete
	-->
    <method name="SimpleQuery">
      <annotation name="com.github.fabricecolin.Pinot.SimpleQuery" value="pinotDBus"/>
      <arg type="s" name="searchText" direction="in" />
      <arg type="u" name="maxHits" direction="in" />
      <arg type="as" name="docIds" direction="out" />
    </method>
    <!--
	Updates a document.
	 docId: the document's ID
	-->
    <method name="UpdateDocument">
      <annotation name="com.github.fabricecolin.Pinot.UpdateDocument" value="pinotDBus"/>
      <arg type="u" name="docId" direction="in"/>
      <arg type="u" name="docId" direction="out"/>
    </method>
  </interface>
</node>
)XML_DELIMITER";

#include "PinotDBus_stub.h"

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

org::freedesktop::DBus::IntrospectableStub::IntrospectableStub():
    m_interfaceName("org.freedesktop.DBus.Introspectable")
{
}

org::freedesktop::DBus::IntrospectableStub::~IntrospectableStub()
{
    unregister_object();
}

guint org::freedesktop::DBus::IntrospectableStub::register_object(
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
            sigc::mem_fun(this, &IntrospectableStub::on_method_call),
            sigc::mem_fun(this, &IntrospectableStub::on_interface_get_property),
            sigc::mem_fun(this, &IntrospectableStub::on_interface_set_property));

    guint registration_id;
    try {
        registration_id = connection->register_object(object_path,
            introspection_data->lookup_interface("org.freedesktop.DBus.Introspectable"),
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

void org::freedesktop::DBus::IntrospectableStub::unregister_object()
{
    for (const RegisteredObject &obj: m_registered_objects) {
        obj.connection->unregister_object(obj.id);
    }
    m_registered_objects.clear();
}

void org::freedesktop::DBus::IntrospectableStub::on_method_call(
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

    if (method_name.compare("Introspect") == 0) {
        MethodInvocation methodInvocation(invocation);
        Introspect(
            methodInvocation);
    }

}

void org::freedesktop::DBus::IntrospectableStub::on_interface_get_property(
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

bool org::freedesktop::DBus::IntrospectableStub::on_interface_set_property(
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


bool org::freedesktop::DBus::IntrospectableStub::emitSignal(
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
}com::github::fabricecolin::PinotStub::PinotStub():
    m_interfaceName("com.github.fabricecolin.Pinot")
{
    IndexFlushed_signal.connect(sigc::bind<0>(sigc::mem_fun(this, &PinotStub::IndexFlushed_emitter),
            std::vector<Glib::ustring>({""})) );
    IndexFlushed_selectiveSignal.connect(sigc::mem_fun(this, &PinotStub::IndexFlushed_emitter));
}

com::github::fabricecolin::PinotStub::~PinotStub()
{
    unregister_object();
}

guint com::github::fabricecolin::PinotStub::register_object(
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
            sigc::mem_fun(this, &PinotStub::on_method_call),
            sigc::mem_fun(this, &PinotStub::on_interface_get_property),
            sigc::mem_fun(this, &PinotStub::on_interface_set_property));

    guint registration_id;
    try {
        registration_id = connection->register_object(object_path,
            introspection_data->lookup_interface("com.github.fabricecolin.Pinot"),
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

void com::github::fabricecolin::PinotStub::unregister_object()
{
    for (const RegisteredObject &obj: m_registered_objects) {
        obj.connection->unregister_object(obj.id);
    }
    m_registered_objects.clear();
}

void com::github::fabricecolin::PinotStub::on_method_call(
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

    if (method_name.compare("GetStatistics") == 0) {
        MethodInvocation methodInvocation(invocation);
        GetStatistics(
            methodInvocation);
    }

    if (method_name.compare("Reload") == 0) {
        MethodInvocation methodInvocation(invocation);
        Reload(
            methodInvocation);
    }

    if (method_name.compare("Stop") == 0) {
        MethodInvocation methodInvocation(invocation);
        Stop(
            methodInvocation);
    }

    if (method_name.compare("HasDocument") == 0) {
        Glib::Variant<Glib::ustring> base_url;
        parameters.get_child(base_url, 0);
        Glib::ustring p_url = specialGetter(base_url);

        MethodInvocation methodInvocation(invocation);
        HasDocument(
            (p_url),
            methodInvocation);
    }

    if (method_name.compare("GetLabels") == 0) {
        MethodInvocation methodInvocation(invocation);
        GetLabels(
            methodInvocation);
    }

    if (method_name.compare("AddLabel") == 0) {
        Glib::Variant<Glib::ustring> base_label;
        parameters.get_child(base_label, 0);
        Glib::ustring p_label = specialGetter(base_label);

        MethodInvocation methodInvocation(invocation);
        AddLabel(
            (p_label),
            methodInvocation);
    }

    if (method_name.compare("DeleteLabel") == 0) {
        Glib::Variant<Glib::ustring> base_label;
        parameters.get_child(base_label, 0);
        Glib::ustring p_label = specialGetter(base_label);

        MethodInvocation methodInvocation(invocation);
        DeleteLabel(
            (p_label),
            methodInvocation);
    }

    if (method_name.compare("GetDocumentLabels") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        MethodInvocation methodInvocation(invocation);
        GetDocumentLabels(
            (p_docId),
            methodInvocation);
    }

    if (method_name.compare("SetDocumentLabels") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        Glib::Variant<std::vector<Glib::ustring>> base_labels;
        parameters.get_child(base_labels, 1);
        std::vector<Glib::ustring> p_labels = specialGetter(base_labels);

        Glib::Variant<bool> base_resetLabels;
        parameters.get_child(base_resetLabels, 2);
        bool p_resetLabels = specialGetter(base_resetLabels);

        MethodInvocation methodInvocation(invocation);
        SetDocumentLabels(
            (p_docId),
            (p_labels),
            (p_resetLabels),
            methodInvocation);
    }

    if (method_name.compare("SetDocumentsLabels") == 0) {
        Glib::Variant<std::vector<Glib::ustring>> base_docIds;
        parameters.get_child(base_docIds, 0);
        std::vector<Glib::ustring> p_docIds = specialGetter(base_docIds);

        Glib::Variant<std::vector<Glib::ustring>> base_labels;
        parameters.get_child(base_labels, 1);
        std::vector<Glib::ustring> p_labels = specialGetter(base_labels);

        Glib::Variant<bool> base_resetLabels;
        parameters.get_child(base_resetLabels, 2);
        bool p_resetLabels = specialGetter(base_resetLabels);

        MethodInvocation methodInvocation(invocation);
        SetDocumentsLabels(
            (p_docIds),
            (p_labels),
            (p_resetLabels),
            methodInvocation);
    }

    if (method_name.compare("GetDocumentInfo") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        MethodInvocation methodInvocation(invocation);
        GetDocumentInfo(
            (p_docId),
            methodInvocation);
    }

    if (method_name.compare("SetDocumentInfo") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> base_fields;
        parameters.get_child(base_fields, 1);
        std::vector<std::tuple<Glib::ustring,Glib::ustring>> p_fields = specialGetter(base_fields);

        MethodInvocation methodInvocation(invocation);
        SetDocumentInfo(
            (p_docId),
            (p_fields),
            methodInvocation);
    }

    if (method_name.compare("GetDocumentTermsCount") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        MethodInvocation methodInvocation(invocation);
        GetDocumentTermsCount(
            (p_docId),
            methodInvocation);
    }

    if (method_name.compare("GetDocumentTerms") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        MethodInvocation methodInvocation(invocation);
        GetDocumentTerms(
            (p_docId),
            methodInvocation);
    }

    if (method_name.compare("Query") == 0) {
        Glib::Variant<Glib::ustring> base_engineType;
        parameters.get_child(base_engineType, 0);
        Glib::ustring p_engineType = specialGetter(base_engineType);

        Glib::Variant<Glib::ustring> base_engineName;
        parameters.get_child(base_engineName, 1);
        Glib::ustring p_engineName = specialGetter(base_engineName);

        Glib::Variant<Glib::ustring> base_searchText;
        parameters.get_child(base_searchText, 2);
        Glib::ustring p_searchText = specialGetter(base_searchText);

        Glib::Variant<guint32> base_startDoc;
        parameters.get_child(base_startDoc, 3);
        guint32 p_startDoc = specialGetter(base_startDoc);

        Glib::Variant<guint32> base_maxHits;
        parameters.get_child(base_maxHits, 4);
        guint32 p_maxHits = specialGetter(base_maxHits);

        MethodInvocation methodInvocation(invocation);
        Query(
            (p_engineType),
            (p_engineName),
            (p_searchText),
            (p_startDoc),
            (p_maxHits),
            methodInvocation);
    }

    if (method_name.compare("SimpleQuery") == 0) {
        Glib::Variant<Glib::ustring> base_searchText;
        parameters.get_child(base_searchText, 0);
        Glib::ustring p_searchText = specialGetter(base_searchText);

        Glib::Variant<guint32> base_maxHits;
        parameters.get_child(base_maxHits, 1);
        guint32 p_maxHits = specialGetter(base_maxHits);

        MethodInvocation methodInvocation(invocation);
        SimpleQuery(
            (p_searchText),
            (p_maxHits),
            methodInvocation);
    }

    if (method_name.compare("UpdateDocument") == 0) {
        Glib::Variant<guint32> base_docId;
        parameters.get_child(base_docId, 0);
        guint32 p_docId = specialGetter(base_docId);

        MethodInvocation methodInvocation(invocation);
        UpdateDocument(
            (p_docId),
            methodInvocation);
    }

}

void com::github::fabricecolin::PinotStub::on_interface_get_property(
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

bool com::github::fabricecolin::PinotStub::on_interface_set_property(
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

void com::github::fabricecolin::PinotStub::IndexFlushed_emitter(
    const std::vector<Glib::ustring> &destination_bus_names,guint32 docsCount)
{
    std::vector<Glib::VariantBase> paramsList;

    paramsList.push_back(Glib::Variant<guint32>::create((docsCount)));;

    const Glib::VariantContainerBase params =
        Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple(paramsList);
    for (const RegisteredObject &obj: m_registered_objects) {
        for (const auto &bus_name: destination_bus_names) {
            obj.connection->emit_signal(
                    obj.object_path,
                    "com.github.fabricecolin.Pinot",
                    "IndexFlushed",
                    bus_name,
                    params);
        }
    }
}


bool com::github::fabricecolin::PinotStub::emitSignal(
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
