#pragma once
#include <iostream>
#include <vector>
#include "glibmm.h"
#include "giomm.h"

namespace org {
namespace freedesktop {
namespace DBus {

class IntrospectableTypeWrap {
public:
    template<typename T>
    static void unwrapList(std::vector<T> &list, const Glib::VariantContainerBase &wrapped) {
        for (uint i = 0; i < wrapped.get_n_children(); i++) {
            Glib::Variant<T> item;
            wrapped.get_child(item, i);
            list.push_back(item.get());
        }
    }

    static std::vector<Glib::ustring> stdStringVecToGlibStringVec(const std::vector<std::string> &strv) {
        std::vector<Glib::ustring> newStrv;
        for (uint i = 0; i < strv.size(); i++) {
            newStrv.push_back(strv[i]);
        }

        return newStrv;
    }

    static std::vector<std::string> glibStringVecToStdStringVec(const std::vector<Glib::ustring> &strv) {
        std::vector<std::string> newStrv;
        for (uint i = 0; i < strv.size(); i++) {
            newStrv.push_back(strv[i]);
        }

        return newStrv;
    }
};

} // DBus
} // freedesktop
} // org

namespace com {
namespace github {
namespace fabricecolin {

class PinotTypeWrap {
public:
    template<typename T>
    static void unwrapList(std::vector<T> &list, const Glib::VariantContainerBase &wrapped) {
        for (uint i = 0; i < wrapped.get_n_children(); i++) {
            Glib::Variant<T> item;
            wrapped.get_child(item, i);
            list.push_back(item.get());
        }
    }

    static std::vector<Glib::ustring> stdStringVecToGlibStringVec(const std::vector<std::string> &strv) {
        std::vector<Glib::ustring> newStrv;
        for (uint i = 0; i < strv.size(); i++) {
            newStrv.push_back(strv[i]);
        }

        return newStrv;
    }

    static std::vector<std::string> glibStringVecToStdStringVec(const std::vector<Glib::ustring> &strv) {
        std::vector<std::string> newStrv;
        for (uint i = 0; i < strv.size(); i++) {
            newStrv.push_back(strv[i]);
        }

        return newStrv;
    }

    static Glib::VariantContainerBase GetDocumentInfo_pack(
        guint32 arg_docId) {
        Glib::VariantContainerBase base;
        Glib::Variant<guint32> params =
            Glib::Variant<guint32>::create(arg_docId);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetDocumentTermsCount_pack(
        guint32 arg_docId) {
        Glib::VariantContainerBase base;
        Glib::Variant<guint32> params =
            Glib::Variant<guint32>::create(arg_docId);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetDocumentTerms_pack(
        guint32 arg_docId) {
        Glib::VariantContainerBase base;
        Glib::Variant<guint32> params =
            Glib::Variant<guint32>::create(arg_docId);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase AddLabel_pack(
        const Glib::ustring & arg_label) {
        Glib::VariantContainerBase base;
        Glib::Variant<Glib::ustring> params =
            Glib::Variant<Glib::ustring>::create(arg_label);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase DeleteLabel_pack(
        const Glib::ustring & arg_label) {
        Glib::VariantContainerBase base;
        Glib::Variant<Glib::ustring> params =
            Glib::Variant<Glib::ustring>::create(arg_label);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase HasLabel_pack(
        guint32 arg_docId,
        const Glib::ustring & arg_label) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<guint32> docId_param =
            Glib::Variant<guint32>::create(arg_docId);
        params.push_back(docId_param);

        Glib::Variant<Glib::ustring> label_param =
            Glib::Variant<Glib::ustring>::create(arg_label);
        params.push_back(label_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetDocumentLabels_pack(
        guint32 arg_docId) {
        Glib::VariantContainerBase base;
        Glib::Variant<guint32> params =
            Glib::Variant<guint32>::create(arg_docId);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase SetDocumentLabels_pack(
        guint32 arg_docId,
        const std::vector<Glib::ustring> & arg_labels,
        bool arg_resetLabels) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<guint32> docId_param =
            Glib::Variant<guint32>::create(arg_docId);
        params.push_back(docId_param);

        Glib::Variant<std::vector<Glib::ustring>> labels_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_labels);
        params.push_back(labels_param);

        Glib::Variant<bool> resetLabels_param =
            Glib::Variant<bool>::create(arg_resetLabels);
        params.push_back(resetLabels_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase SetDocumentsLabels_pack(
        const std::vector<Glib::ustring> & arg_docIds,
        const std::vector<Glib::ustring> & arg_labels,
        bool arg_resetLabels) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<std::vector<Glib::ustring>> docIds_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_docIds);
        params.push_back(docIds_param);

        Glib::Variant<std::vector<Glib::ustring>> labels_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_labels);
        params.push_back(labels_param);

        Glib::Variant<bool> resetLabels_param =
            Glib::Variant<bool>::create(arg_resetLabels);
        params.push_back(resetLabels_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase HasDocument_pack(
        const Glib::ustring & arg_url) {
        Glib::VariantContainerBase base;
        Glib::Variant<Glib::ustring> params =
            Glib::Variant<Glib::ustring>::create(arg_url);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetCloseTerms_pack(
        const Glib::ustring & arg_term) {
        Glib::VariantContainerBase base;
        Glib::Variant<Glib::ustring> params =
            Glib::Variant<Glib::ustring>::create(arg_term);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetDocumentsCount_pack(
        const Glib::ustring & arg_label) {
        Glib::VariantContainerBase base;
        Glib::Variant<Glib::ustring> params =
            Glib::Variant<Glib::ustring>::create(arg_label);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase ListDocuments_pack(
        const Glib::ustring & arg_term,
        guint32 arg_termType,
        guint32 arg_maxCount,
        guint32 arg_startOffset) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<Glib::ustring> term_param =
            Glib::Variant<Glib::ustring>::create(arg_term);
        params.push_back(term_param);

        Glib::Variant<guint32> termType_param =
            Glib::Variant<guint32>::create(arg_termType);
        params.push_back(termType_param);

        Glib::Variant<guint32> maxCount_param =
            Glib::Variant<guint32>::create(arg_maxCount);
        params.push_back(maxCount_param);

        Glib::Variant<guint32> startOffset_param =
            Glib::Variant<guint32>::create(arg_startOffset);
        params.push_back(startOffset_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase UpdateDocument_pack(
        guint32 arg_docId) {
        Glib::VariantContainerBase base;
        Glib::Variant<guint32> params =
            Glib::Variant<guint32>::create(arg_docId);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase SetDocumentInfo_pack(
        guint32 arg_docId,
        const std::vector<std::tuple<Glib::ustring,Glib::ustring>> & arg_fields) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<guint32> docId_param =
            Glib::Variant<guint32>::create(arg_docId);
        params.push_back(docId_param);

        Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>> fields_param =
            Glib::Variant<std::vector<std::tuple<Glib::ustring,Glib::ustring>>>::create(arg_fields);
        params.push_back(fields_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase Query_pack(
        const Glib::ustring & arg_engineType,
        const Glib::ustring & arg_engineName,
        const Glib::ustring & arg_searchText,
        guint32 arg_startDoc,
        guint32 arg_maxHits) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<Glib::ustring> engineType_param =
            Glib::Variant<Glib::ustring>::create(arg_engineType);
        params.push_back(engineType_param);

        Glib::Variant<Glib::ustring> engineName_param =
            Glib::Variant<Glib::ustring>::create(arg_engineName);
        params.push_back(engineName_param);

        Glib::Variant<Glib::ustring> searchText_param =
            Glib::Variant<Glib::ustring>::create(arg_searchText);
        params.push_back(searchText_param);

        Glib::Variant<guint32> startDoc_param =
            Glib::Variant<guint32>::create(arg_startDoc);
        params.push_back(startDoc_param);

        Glib::Variant<guint32> maxHits_param =
            Glib::Variant<guint32>::create(arg_maxHits);
        params.push_back(maxHits_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase SimpleQuery_pack(
        const Glib::ustring & arg_searchText,
        guint32 arg_maxHits) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<Glib::ustring> searchText_param =
            Glib::Variant<Glib::ustring>::create(arg_searchText);
        params.push_back(searchText_param);

        Glib::Variant<guint32> maxHits_param =
            Glib::Variant<guint32>::create(arg_maxHits);
        params.push_back(maxHits_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }
};

} // fabricecolin
} // github
} // com


