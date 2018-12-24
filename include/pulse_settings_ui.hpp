#ifndef PULSE_SETTINGS_UI_HPP
#define PULSE_SETTINGS_UI_HPP

#include <giomm/settings.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/combobox.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/grid.h>
#include <gtkmm/liststore.h>
#include <gtkmm/togglebutton.h>
#include "application.hpp"

class PulseSettingsUi : public Gtk::Grid {
 public:
  PulseSettingsUi(BaseObjectType* cobject,
                  const Glib::RefPtr<Gtk::Builder>& builder,
                  const Glib::RefPtr<Gio::Settings>& refSettings,
                  Application* application);

  virtual ~PulseSettingsUi();

 private:
  std::string log_tag = "pulse_settings_ui: ";

  Glib::RefPtr<Gio::Settings> settings;

  Application* app;

  Gtk::ToggleButton *use_default_sink, *use_default_source;
  Gtk::ComboBox *input_device, *output_device;
  Gtk::ComboBoxText *blocksize_in, *blocksize_out;

  Glib::RefPtr<Gtk::Adjustment> buffer_in, buffer_out, latency_in, latency_out;
  Glib::RefPtr<Gtk::ListStore> sink_list, source_list;

  std::vector<sigc::connection> connections;

  void get_object(const Glib::RefPtr<Gtk::Builder>& builder,
                  const std::string& name,
                  Glib::RefPtr<Gtk::Adjustment>& object) {
    object =
        Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder->get_object(name));
  }

  void get_object(const Glib::RefPtr<Gtk::Builder>& builder,
                  const std::string& name,
                  Glib::RefPtr<Gtk::ListStore>& object) {
    object =
        Glib::RefPtr<Gtk::ListStore>::cast_dynamic(builder->get_object(name));
  }
};

#endif
