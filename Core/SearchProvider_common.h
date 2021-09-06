#pragma once
#include <iostream>
#include <vector>
#include "glibmm.h"
#include "giomm.h"

namespace org {
namespace gnome {
namespace Shell {

class SearchProvider2TypeWrap {
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

    static Glib::VariantContainerBase GetInitialResultSet_pack(
        const std::vector<Glib::ustring> & arg_terms) {
        Glib::VariantContainerBase base;
        Glib::Variant<std::vector<Glib::ustring>> params =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_terms);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetSubsearchResultSet_pack(
        const std::vector<Glib::ustring> & arg_previous_results,
        const std::vector<Glib::ustring> & arg_terms) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<std::vector<Glib::ustring>> previous_results_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_previous_results);
        params.push_back(previous_results_param);

        Glib::Variant<std::vector<Glib::ustring>> terms_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_terms);
        params.push_back(terms_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase GetResultMetas_pack(
        const std::vector<Glib::ustring> & arg_identifiers) {
        Glib::VariantContainerBase base;
        Glib::Variant<std::vector<Glib::ustring>> params =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_identifiers);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase ActivateResult_pack(
        const Glib::ustring & arg_identifier,
        const std::vector<Glib::ustring> & arg_terms,
        guint32 arg_timestamp) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<Glib::ustring> identifier_param =
            Glib::Variant<Glib::ustring>::create(arg_identifier);
        params.push_back(identifier_param);

        Glib::Variant<std::vector<Glib::ustring>> terms_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_terms);
        params.push_back(terms_param);

        Glib::Variant<guint32> timestamp_param =
            Glib::Variant<guint32>::create(arg_timestamp);
        params.push_back(timestamp_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }

    static Glib::VariantContainerBase LaunchSearch_pack(
        const std::vector<Glib::ustring> & arg_terms,
        guint32 arg_timestamp) {
        Glib::VariantContainerBase base;
        std::vector<Glib::VariantBase> params;

        Glib::Variant<std::vector<Glib::ustring>> terms_param =
            Glib::Variant<std::vector<Glib::ustring>>::create(arg_terms);
        params.push_back(terms_param);

        Glib::Variant<guint32> timestamp_param =
            Glib::Variant<guint32>::create(arg_timestamp);
        params.push_back(timestamp_param);
        return Glib::VariantContainerBase::create_tuple(params);
    }
};

} // Shell
} // gnome
} // org


