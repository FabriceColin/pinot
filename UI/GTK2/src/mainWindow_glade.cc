// generated 2011/1/2 21:12:23 SGT by fabrice@rexor.dyndns.org.(none)
// using glademm V2.12.1
//
// DO NOT EDIT THIS FILE ! It was created using
// glade-- /home/fabrice/Projects/MetaSE/pinot/UI/GTK2/metase-gtk2.glade
// for gtk 2.14.5 and gtkmm 2.14.3
//
// Please modify the corresponding derived classes in ./src/mainWindow.cc


#if defined __GNUC__ && __GNUC__ < 3
#error This program will crash if compiled with g++ 2.x
// see the dynamic_cast bug in the gtkmm FAQ
#endif //
#include "config.h"
/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#endif
#include <gtkmmconfig.h>
#if GTKMM_MAJOR_VERSION==2 && GTKMM_MINOR_VERSION>2
#include <sigc++/sigc++.h>
#define GMM_GTKMM_22_24(a,b) b
#else //gtkmm 2.2
#define GMM_GTKMM_22_24(a,b) a
#endif //
#include "mainWindow_glade.hh"
#include <gdk/gdkkeysyms.h>
#include <gtkmm/stock.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/scrolledwindow.h>
#ifndef ENABLE_NLS
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


mainWindow_glade::mainWindow_glade(
) : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{  mainWindow = this;
   gmm_data = new GlademmData(get_accel_group());
   open1 = NULL;
   
   Gtk::MenuItem *separator7 = NULL;
   opencache1 = NULL;
   Gtk::Image *image1007 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-go-up"), Gtk::IconSize(1)));
   openparent1 = NULL;
   Gtk::MenuItem *separator8 = NULL;
   Gtk::Image *image1008 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-add"), Gtk::IconSize(1)));
   addtoindex1 = NULL;
   Gtk::Image *image1009 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-refresh"), Gtk::IconSize(1)));
   updateindex1 = NULL;
   Gtk::Image *image1010 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-delete"), Gtk::IconSize(1)));
   unindex1 = NULL;
   Gtk::MenuItem *separator9 = NULL;
   morelikethis1 = NULL;
   searchthisfor1 = NULL;
   Gtk::MenuItem *separator10 = NULL;
   properties1 = NULL;
   separatormenuitem1 = NULL;
   quit1 = NULL;
   Gtk::Menu *fileMenuitem_menu = Gtk::manage(new class Gtk::Menu());
   fileMenuitem = NULL;
   cut1 = NULL;
   copy1 = NULL;
   paste1 = NULL;
   delete1 = NULL;
   Gtk::MenuItem *separator5 = NULL;
   preferences1 = NULL;
   Gtk::Menu *editMenuitem_menu = Gtk::manage(new class Gtk::Menu());
   editMenuitem = NULL;
   listcontents1 = NULL;
   Gtk::RadioMenuItem::Group _RadioMIGroup_searchenginegroup1;
   searchenginegroup1 = NULL;
   hostnamegroup1 = NULL;
   Gtk::Menu *groupresults1_menu = Gtk::manage(new class Gtk::Menu());
   groupresults1 = NULL;
   showextracts1 = NULL;
   Gtk::MenuItem *separator4 = NULL;
   Gtk::Image *image1011 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-open"), Gtk::IconSize(1)));
   import1 = NULL;
   Gtk::Image *image1012 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-save-as"), Gtk::IconSize(1)));
   export1 = NULL;
   Gtk::MenuItem *separator6 = NULL;
   Gtk::Image *image1013 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-info"), Gtk::IconSize(1)));
   Gtk::ImageMenuItem *status1 = NULL;
   Gtk::Menu *optionsMenuitem_menu = Gtk::manage(new class Gtk::Menu());
   Gtk::MenuItem *optionsMenuitem = NULL;
   about1 = NULL;
   Gtk::Menu *helpMenuitem_menu = Gtk::manage(new class Gtk::Menu());
   helpMenuitem = NULL;
   Gtk::MenuBar *mainMenubar = Gtk::manage(new class Gtk::MenuBar());
   enginesVbox = Gtk::manage(new class Gtk::VBox(false, 0));
   
   Gtk::Image *image439 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-add"), Gtk::IconSize(4)));
   addIndexButton = Gtk::manage(new class Gtk::Button());
   
   Gtk::Image *image438 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-remove"), Gtk::IconSize(4)));
   removeIndexButton = Gtk::manage(new class Gtk::Button());
   
   Gtk::HBox *indexHbox = Gtk::manage(new class Gtk::HBox(true, 4));
   leftVbox = Gtk::manage(new class Gtk::VBox(false, 0));
   
   Gtk::Image *image623 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-network"), Gtk::IconSize(4)));
   enginesTogglebutton = Gtk::manage(new class Gtk::ToggleButton());
   
   Gtk::Label *liveQueryLabel = Gtk::manage(new class Gtk::Label(_("Query:")));
   liveQueryEntry = Gtk::manage(new class Gtk::Entry());
   findButton = Gtk::manage(new class Gtk::Button(Gtk::StockID("gtk-find")));
   
#if GTK_VERSION_LT(3, 0)
   Gtk::VButtonBox *findVbuttonbox = Gtk::manage(new class Gtk::VButtonBox(Gtk::BUTTONBOX_DEFAULT_STYLE, 0));
#else
   Gtk::VButtonBox *findVbuttonbox = Gtk::manage(new class Gtk::VButtonBox());
#endif
   Gtk::HBox *liveQueryHbox = Gtk::manage(new class Gtk::HBox(false, 0));
   queryTreeview = Gtk::manage(new class Gtk::TreeView());
   
   Gtk::ScrolledWindow *queryScrolledwindow = Gtk::manage(new class Gtk::ScrolledWindow());
   addQueryButton = Gtk::manage(new class Gtk::Button(Gtk::StockID("gtk-add")));
   removeQueryButton = Gtk::manage(new class Gtk::Button(Gtk::StockID("gtk-remove")));
   queryHistoryButton = Gtk::manage(new class Gtk::Button(_("History")));
   findQueryButton = Gtk::manage(new class Gtk::Button(Gtk::StockID("gtk-find")));
   
   Gtk::VButtonBox *queryVbuttonbox = Gtk::manage(new class Gtk::VButtonBox(Gtk::BUTTONBOX_START, 0));
   Gtk::HBox *queryHbox = Gtk::manage(new class Gtk::HBox(false, 0));
   Gtk::Label *queryLabel = Gtk::manage(new class Gtk::Label(_("Stored queries")));
#if GTK_VERSION_GT(2, 3)
   queryExpander = Gtk::manage(new class Gtk::Expander());
#else //
   queryExpander = Gtk::manage(new class Gtk::HandleBox());
#endif //
   rightVbox = Gtk::manage(new class Gtk::VBox(false, 0));
   mainHpaned = Gtk::manage(new class Gtk::HPaned());
   mainProgressbar = Gtk::manage(new class Gtk::ProgressBar());
   mainStatusbar = Gtk::manage(new class Gtk::Statusbar());
   
   Gtk::HBox *mainHbox = Gtk::manage(new class Gtk::HBox(false, 0));
   Gtk::VBox *mainVbox = Gtk::manage(new class Gtk::VBox(false, 0));
   
#if GTK_VERSION_LT(3, 0)
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-open")));
   open1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator7 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Open Cache")));
   opencache1 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Open Parent"), *image1007));
   openparent1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator8 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("_Add To Index"), *image1008));
   addtoindex1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Update"), *image1009));
   updateindex1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Unindex"), *image1010));
   unindex1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator9 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("More Like This")));
   morelikethis1 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Search This For")));
   searchthisfor1 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator10 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-properties")));
   properties1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separatormenuitem1 = (Gtk::MenuItem *)&fileMenuitem_menu->items().back();
   
   fileMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-quit")));
   quit1 = (Gtk::ImageMenuItem *)&fileMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-cut")));
   cut1 = (Gtk::ImageMenuItem *)&editMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-copy")));
   copy1 = (Gtk::ImageMenuItem *)&editMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-paste")));
   paste1 = (Gtk::ImageMenuItem *)&editMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-delete")));
   delete1 = (Gtk::ImageMenuItem *)&editMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator5 = (Gtk::MenuItem *)&editMenuitem_menu->items().back();
   
   editMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-preferences")));
   preferences1 = (Gtk::ImageMenuItem *)&editMenuitem_menu->items().back();
   
   groupresults1_menu->items().push_back(Gtk::Menu_Helpers::RadioMenuElem(_RadioMIGroup_searchenginegroup1, _("Search Engine")));
   searchenginegroup1 = (Gtk::RadioMenuItem *)&groupresults1_menu->items().back();
   
   groupresults1_menu->items().push_back(Gtk::Menu_Helpers::RadioMenuElem(_RadioMIGroup_searchenginegroup1, _("Host Name")));
   hostnamegroup1 = (Gtk::RadioMenuItem *)&groupresults1_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("List Contents Of")));
   listcontents1 = (Gtk::MenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Group Results By"), *groupresults1_menu));
   groupresults1 = (Gtk::MenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::CheckMenuElem(_("Show Extracts")));
   showextracts1 = (Gtk::CheckMenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator4 = (Gtk::MenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Import URL"), *image1011));
   import1 = (Gtk::ImageMenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Export List"), *image1012));
   export1 = (Gtk::ImageMenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
   separator6 = (Gtk::MenuItem *)&optionsMenuitem_menu->items().back();
   
   optionsMenuitem_menu->items().push_back(Gtk::Menu_Helpers::ImageMenuElem(_("Status"), *image1013));
   status1 = (Gtk::ImageMenuItem *)&optionsMenuitem_menu->items().back();
   
   helpMenuitem_menu->items().push_back(Gtk::Menu_Helpers::StockMenuElem(Gtk::StockID("gtk-about")));
   about1 = (Gtk::ImageMenuItem *)&helpMenuitem_menu->items().back();
   
   mainMenubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_File"), *fileMenuitem_menu));
   fileMenuitem = (Gtk::MenuItem *)&mainMenubar->items().back();
   
   mainMenubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Edit"), *editMenuitem_menu));
   editMenuitem = (Gtk::MenuItem *)&mainMenubar->items().back();
   
   mainMenubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Options"), *optionsMenuitem_menu));
   optionsMenuitem = (Gtk::MenuItem *)&mainMenubar->items().back();
   
   mainMenubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Help"), *helpMenuitem_menu));
   helpMenuitem = (Gtk::MenuItem *)&mainMenubar->items().back();
#else
   open1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::OPEN));
   fileMenuitem_menu->append(*open1);

   separator7 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   fileMenuitem_menu->append(*separator7);

   opencache1 = Gtk::manage(new class Gtk::MenuItem(_("Open Cache")));
   fileMenuitem_menu->append(*opencache1);
   
   openparent1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1007, _("Open Parent")));
   fileMenuitem_menu->append(*openparent1);
   
   separator8 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   fileMenuitem_menu->append(*separator8);
   
   addtoindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1008, _("_Add To Index"), true));
   fileMenuitem_menu->append(*addtoindex1);
   
   updateindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1009, _("Update")));
   fileMenuitem_menu->append(*updateindex1);

   unindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1010, _("Unindex")));
   fileMenuitem_menu->append(*unindex1);
   
   separator9 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   fileMenuitem_menu->append(*separator9);
   
   morelikethis1 = Gtk::manage(new class Gtk::MenuItem(_("More Like This")));
   fileMenuitem_menu->append(*morelikethis1);
   
   searchthisfor1 = Gtk::manage(new class Gtk::MenuItem(_("Search This For")));
   fileMenuitem_menu->append(*searchthisfor1);
   
   separator10 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   fileMenuitem_menu->append(*separator10);
   
   properties1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PROPERTIES));
   fileMenuitem_menu->append(*properties1);
   
   separatormenuitem1 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   fileMenuitem_menu->append(*separatormenuitem1);
   
   quit1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::QUIT));
   fileMenuitem_menu->append(*quit1);
   
   cut1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::CUT));
   editMenuitem_menu->append(*cut1);
   
   copy1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::COPY));
   editMenuitem_menu->append(*copy1);
   
   paste1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PASTE));
   editMenuitem_menu->append(*paste1);
   
   delete1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::DELETE));
   editMenuitem_menu->append(*delete1);
   
   separator5 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   editMenuitem_menu->append(*separator5);
   
   preferences1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PREFERENCES));
   editMenuitem_menu->append(*preferences1);
   
   searchenginegroup1 = Gtk::manage(new class Gtk::RadioMenuItem(_RadioMIGroup_searchenginegroup1, _("Search Engine")));
   groupresults1_menu->append(*searchenginegroup1);
   
   hostnamegroup1 = Gtk::manage(new class Gtk::RadioMenuItem(_RadioMIGroup_searchenginegroup1, _("Host Name")));
   groupresults1_menu->append(*hostnamegroup1);
   
   listcontents1 = Gtk::manage(new class Gtk::MenuItem(_("List Contents Of")));
   optionsMenuitem_menu->append(*listcontents1);
   
   groupresults1 = Gtk::manage(new class Gtk::MenuItem(_("Group Results By")));
   groupresults1->set_submenu(*groupresults1_menu);
   optionsMenuitem_menu->append(*groupresults1);
   
   showextracts1 = Gtk::manage(new class Gtk::CheckMenuItem(_("Show Extracts")));
   optionsMenuitem_menu->append(*showextracts1);
   
   separator4 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   optionsMenuitem_menu->append(*separator4);
   
   import1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1011, _("Import URL")));
   optionsMenuitem_menu->append(*import1);
   
   export1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1012, _("Export List")));
   optionsMenuitem_menu->append(*export1);

   separator6 = Gtk::manage(new class Gtk::SeparatorMenuItem());
   optionsMenuitem_menu->append(*separator6);
   
   status1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1013, _("Status")));
   optionsMenuitem_menu->append(*status1);
   
   about1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::ABOUT));
   helpMenuitem_menu->append(*about1);
   
   fileMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_File"), true));
   fileMenuitem->set_submenu(*fileMenuitem_menu);
   mainMenubar->append(*fileMenuitem);
   
   editMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Edit"), true));
   editMenuitem->set_submenu(*editMenuitem_menu);
   mainMenubar->append(*editMenuitem);
   
   optionsMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Options"), true));
   optionsMenuitem->set_submenu(*optionsMenuitem_menu);
   mainMenubar->append(*optionsMenuitem);
   
   helpMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Help"), true));
   helpMenuitem->set_submenu(*helpMenuitem_menu);
   mainMenubar->append(*helpMenuitem);
#endif
   image1007->set_alignment(0.5,0.5);
   image1007->set_padding(0,0);
   image1008->set_alignment(0.5,0.5);
   image1008->set_padding(0,0);
   image1009->set_alignment(0.5,0.5);
   image1009->set_padding(0,0);
   image1010->set_alignment(0.5,0.5);
   image1010->set_padding(0,0);
   searchenginegroup1->set_active(true);
   hostnamegroup1->set_active(false);
   showextracts1->set_active(true);
   image1011->set_alignment(0.5,0.5);
   image1011->set_padding(0,0);
   image1012->set_alignment(0.5,0.5);
   image1012->set_padding(0,0);
   image1013->set_alignment(0.5,0.5);
   image1013->set_padding(0,0);
   image439->set_alignment(0.5,0.5);
   image439->set_padding(0,0);
#if GTK_VERSION_LT(3, 0)
   addIndexButton->set_flags(Gtk::CAN_FOCUS);
   addIndexButton->set_flags(Gtk::CAN_DEFAULT);
#else
   addIndexButton->set_can_focus();
   addIndexButton->set_can_default();
#endif
   addIndexButton->set_relief(Gtk::RELIEF_NORMAL);
   addIndexButton->add(*image439);
   image438->set_alignment(0.5,0.5);
   image438->set_padding(0,0);
#if GTK_VERSION_LT(3, 0)
   removeIndexButton->set_flags(Gtk::CAN_FOCUS);
   removeIndexButton->set_flags(Gtk::CAN_DEFAULT);
#else
   removeIndexButton->set_can_focus();
   removeIndexButton->set_can_default();
#endif
   removeIndexButton->set_relief(Gtk::RELIEF_NORMAL);
   removeIndexButton->add(*image438);
   indexHbox->pack_start(*addIndexButton);
   indexHbox->pack_start(*removeIndexButton);
   leftVbox->pack_start(*enginesVbox);
   leftVbox->pack_start(*indexHbox, Gtk::PACK_SHRINK, 0);
   image623->set_alignment(0.5,0.5);
   image623->set_padding(0,0);
#if GTK_VERSION_LT(3, 0)
   enginesTogglebutton->set_flags(Gtk::CAN_FOCUS);
#else
   enginesTogglebutton->set_can_focus();
#endif
   enginesTogglebutton->set_relief(Gtk::RELIEF_NORMAL);
   enginesTogglebutton->set_active(false);
   enginesTogglebutton->add(*image623);
   liveQueryLabel->set_alignment(0.5,0.5);
   liveQueryLabel->set_padding(0,0);
   liveQueryLabel->set_justify(Gtk::JUSTIFY_LEFT);
   liveQueryLabel->set_line_wrap(false);
   liveQueryLabel->set_use_markup(false);
   liveQueryLabel->set_selectable(false);
   liveQueryLabel->set_ellipsize(Pango::ELLIPSIZE_NONE);
   liveQueryLabel->set_width_chars(-1);
   liveQueryLabel->set_angle(0);
   liveQueryLabel->set_single_line_mode(false);
#if GTK_VERSION_LT(3, 0)
   liveQueryEntry->set_flags(Gtk::CAN_FOCUS);
#else
   liveQueryEntry->set_can_focus();
#endif
   liveQueryEntry->grab_focus();
   liveQueryEntry->set_visibility(true);
   liveQueryEntry->set_editable(true);
   liveQueryEntry->set_max_length(0);
   liveQueryEntry->set_has_frame(true);
   liveQueryEntry->set_activates_default(false);
#if GTK_VERSION_LT(3, 0)
   findButton->set_flags(Gtk::CAN_FOCUS);
   findButton->set_flags(Gtk::CAN_DEFAULT);
#else
   findButton->set_can_focus();
   findButton->set_can_default();
#endif
   findButton->set_relief(Gtk::RELIEF_NORMAL);
   findVbuttonbox->pack_start(*findButton);
   liveQueryHbox->pack_start(*enginesTogglebutton, Gtk::PACK_SHRINK, 4);
   liveQueryHbox->pack_start(*liveQueryLabel, Gtk::PACK_SHRINK, 4);
   liveQueryHbox->pack_start(*liveQueryEntry, Gtk::PACK_EXPAND_WIDGET, 4);
   liveQueryHbox->pack_start(*findVbuttonbox, Gtk::PACK_SHRINK, 4);
   queryTreeview->set_events(Gdk::BUTTON_PRESS_MASK);
#if GTK_VERSION_LT(3, 0)
   queryTreeview->set_flags(Gtk::CAN_FOCUS);
#else
   queryTreeview->set_can_focus();
#endif
   queryTreeview->set_headers_visible(true);
   queryTreeview->set_rules_hint(false);
   queryTreeview->set_reorderable(false);
   queryTreeview->set_enable_search(false);
   queryTreeview->set_fixed_height_mode(false);
   queryTreeview->set_hover_selection(false);
   queryTreeview->set_hover_expand(false);
#if GTK_VERSION_LT(3, 0)
   queryScrolledwindow->set_flags(Gtk::CAN_FOCUS);
#else
   queryScrolledwindow->set_can_focus();
#endif
   queryScrolledwindow->set_shadow_type(Gtk::SHADOW_NONE);
   queryScrolledwindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
   queryScrolledwindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
   queryScrolledwindow->add(*queryTreeview);
#if GTK_VERSION_LT(3, 0)
   addQueryButton->set_flags(Gtk::CAN_FOCUS);
   addQueryButton->set_flags(Gtk::CAN_DEFAULT);
#else
   addQueryButton->set_can_focus();
   addQueryButton->set_can_default();
#endif
   addQueryButton->set_border_width(4);
   addQueryButton->set_relief(Gtk::RELIEF_NORMAL);
#if GTK_VERSION_LT(3, 0)
   removeQueryButton->set_flags(Gtk::CAN_FOCUS);
   removeQueryButton->set_flags(Gtk::CAN_DEFAULT);
#else
   removeQueryButton->set_can_focus();
   removeQueryButton->set_can_default();
#endif
   removeQueryButton->set_border_width(4);
   removeQueryButton->set_relief(Gtk::RELIEF_NORMAL);
#if GTK_VERSION_LT(3, 0)
   queryHistoryButton->set_flags(Gtk::CAN_FOCUS);
   queryHistoryButton->set_flags(Gtk::CAN_DEFAULT);
#else
   queryHistoryButton->set_can_focus();
   queryHistoryButton->set_can_default();
#endif
   queryHistoryButton->set_border_width(4);
   queryHistoryButton->set_relief(Gtk::RELIEF_NORMAL);
#if GTK_VERSION_LT(3, 0)
   findQueryButton->set_flags(Gtk::CAN_FOCUS);
   findQueryButton->set_flags(Gtk::CAN_DEFAULT);
#else
   findQueryButton->set_can_focus();
   findQueryButton->set_can_default();
#endif
   findQueryButton->set_border_width(4);
   findQueryButton->set_relief(Gtk::RELIEF_NORMAL);
   queryVbuttonbox->pack_start(*addQueryButton);
   queryVbuttonbox->pack_start(*removeQueryButton);
   queryVbuttonbox->pack_start(*queryHistoryButton);
   queryVbuttonbox->pack_start(*findQueryButton);
   queryHbox->pack_start(*queryScrolledwindow);
   queryHbox->pack_start(*queryVbuttonbox, Gtk::PACK_SHRINK, 0);
   queryLabel->set_alignment(0.5,0.5);
   queryLabel->set_padding(0,0);
   queryLabel->set_justify(Gtk::JUSTIFY_LEFT);
   queryLabel->set_line_wrap(false);
   queryLabel->set_use_markup(false);
   queryLabel->set_selectable(false);
   queryLabel->set_ellipsize(Pango::ELLIPSIZE_NONE);
   queryLabel->set_width_chars(-1);
   queryLabel->set_angle(0);
   queryLabel->set_single_line_mode(false);
#if GTK_VERSION_LT(3, 0)
   queryExpander->set_flags(Gtk::CAN_FOCUS);
#else
   queryExpander->set_can_focus();
#endif
   queryExpander->set_expanded(false);
   queryExpander->set_spacing(0);
   queryExpander->add(*queryHbox);
   queryExpander->set_label_widget(*queryLabel);
   rightVbox->pack_start(*liveQueryHbox, Gtk::PACK_SHRINK, 0);
   rightVbox->pack_start(*queryExpander, Gtk::PACK_SHRINK, 0);
#if GTK_VERSION_LT(3, 0)
   mainHpaned->set_flags(Gtk::CAN_FOCUS);
#else
   mainHpaned->set_can_focus();
#endif
   mainHpaned->set_position(200);
   mainHpaned->pack1(*leftVbox, Gtk::SHRINK);
   mainHpaned->pack2(*rightVbox, Gtk::EXPAND|Gtk::SHRINK);
   mainHbox->pack_start(*mainProgressbar, Gtk::PACK_SHRINK, 0);
   mainHbox->pack_start(*mainStatusbar);
   mainVbox->pack_start(*mainMenubar, Gtk::PACK_SHRINK, 0);
   mainVbox->pack_start(*mainHpaned, Gtk::PACK_EXPAND_WIDGET, 4);
   mainVbox->pack_start(*mainHbox, Gtk::PACK_SHRINK, 0);
   mainWindow->set_events(Gdk::BUTTON_PRESS_MASK);
   mainWindow->set_title(_("Pinot"));
   mainWindow->set_default_size(680,500);
   mainWindow->set_modal(false);
   mainWindow->property_window_position().set_value(Gtk::WIN_POS_NONE);
   mainWindow->set_resizable(true);
   mainWindow->property_destroy_with_parent().set_value(false);
   mainWindow->add(*mainVbox);
   open1->show();
   separator7->show();
   opencache1->show();
   image1007->show();
   openparent1->show();
   separator8->show();
   image1008->show();
   addtoindex1->show();
   image1009->show();
   updateindex1->show();
   image1010->show();
   unindex1->show();
   separator9->show();
   morelikethis1->show();
   searchthisfor1->show();
   separator10->show();
   properties1->show();
   separatormenuitem1->show();
   quit1->show();
   fileMenuitem->show();
   cut1->show();
   copy1->show();
   paste1->show();
   delete1->show();
   separator5->show();
   preferences1->show();
   editMenuitem->show();
   listcontents1->show();
   searchenginegroup1->show();
   hostnamegroup1->show();
   groupresults1->show();
   showextracts1->show();
   separator4->show();
   image1011->show();
   import1->show();
   image1012->show();
   export1->show();
   separator6->show();
   image1013->show();
   status1->show();
   optionsMenuitem->show();
   about1->show();
   helpMenuitem->show();
   mainMenubar->show();
   enginesVbox->show();
   image439->show();
   addIndexButton->show();
   image438->show();
   removeIndexButton->show();
   indexHbox->show();
   image623->show();
   enginesTogglebutton->show();
   liveQueryLabel->show();
   liveQueryEntry->show();
   findButton->show();
   liveQueryHbox->show();
   queryTreeview->show();
   queryScrolledwindow->show();
   addQueryButton->show();
   removeQueryButton->show();
   queryHistoryButton->show();
   findQueryButton->show();
   queryVbuttonbox->show();
   queryHbox->show();
   queryLabel->show();
   queryExpander->show();
   rightVbox->show();
   mainHpaned->show();
   mainProgressbar->show();
   mainStatusbar->show();
   mainHbox->show();
   mainVbox->show();
   open1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_open_activate), false);
   openparent1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_openparent_activate), false);
   addtoindex1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_addtoindex_activate), false);
   updateindex1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_updateindex_activate), false);
   unindex1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_unindex_activate), false);
   morelikethis1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_morelikethis_activate), false);
   properties1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_properties_activate), false);
   quit1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_quit_activate), false);
   cut1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_cut_activate), false);
   copy1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_copy_activate), false);
   paste1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_paste_activate), false);
   delete1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_delete_activate), false);
   preferences1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_preferences_activate), false);
   searchenginegroup1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_groupresults_activate), false);
   showextracts1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_showextracts_activate), false);
   import1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_import_activate), false);
   export1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_export_activate), false);
   status1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_status_activate), false);
   about1->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_about_activate), false);
   addIndexButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_addIndexButton_clicked), false);
   removeIndexButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_removeIndexButton_clicked), false);
   enginesTogglebutton->signal_toggled().connect(sigc::mem_fun(*this, &mainWindow_glade::on_enginesTogglebutton_toggled), false);
   liveQueryEntry->signal_changed().connect(sigc::mem_fun(*this, &mainWindow_glade::on_liveQueryEntry_changed), false);
   liveQueryEntry->signal_activate().connect(sigc::mem_fun(*this, &mainWindow_glade::on_liveQueryEntry_activate), false);
   findButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_findButton_clicked), false);
   queryTreeview->signal_button_press_event().connect(sigc::mem_fun(*this, &mainWindow_glade::on_queryTreeview_button_press_event), false);
   addQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_addQueryButton_clicked), false);
   removeQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_removeQueryButton_clicked), false);
   queryHistoryButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_queryHistoryButton_clicked), false);
   findQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &mainWindow_glade::on_findQueryButton_clicked), false);
   mainWindow->signal_delete_event().connect(sigc::mem_fun(*this, &mainWindow_glade::on_mainWindow_delete_event), false);
}

mainWindow_glade::~mainWindow_glade()
{  delete gmm_data;
}
