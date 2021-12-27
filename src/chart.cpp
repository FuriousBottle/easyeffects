/*
 *  Copyright © 2017-2022 Wellington Wallace
 *
 *  This file is part of EasyEffects.
 *
 *  EasyEffects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EasyEffects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EasyEffects.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "chart.hpp"

namespace ui::chart {

using namespace std::string_literals;

auto constexpr log_tag = "chart_box: ";

struct Data {
 public:
  ~Data() { util::debug(log_tag + "data struct destroyed"s); }

  bool draw_bar_border, fill_bars, is_visible, rounded_bars;

  int x_axis_height, n_x_decimals, n_y_decimals;

  float mouse_y, mouse_x, margin, line_width;

  float x_min, x_max, y_min, y_max;

  float x_min_log, x_max_log;

  ChartType chart_type;

  ChartScale chart_scale;

  GdkRGBA background_color, color, color_axis_labels, gradient_color;

  std::string x_unit, y_unit;

  std::vector<float> y_axis, x_axis, x_axis_log, objects_x;
};

struct _Chart {
  GtkBox parent_instance;

  GtkEventController* controller_motion;

  Data* data;
};

G_DEFINE_TYPE(Chart, chart, GTK_TYPE_WIDGET)

void set_chart_type(Chart* self, const ChartType& value) {
  self->data->chart_type = value;
}

void set_chart_scale(Chart* self, const ChartScale& value) {
  self->data->chart_scale = value;
}

void set_background_color(Chart* self, GdkRGBA color) {
  self->data->background_color = color;
}

void set_color(Chart* self, GdkRGBA color) {
  self->data->color = color;
}

void set_axis_labels_color(Chart* self, GdkRGBA color) {
  self->data->color_axis_labels = color;
}

void set_line_width(Chart* self, const double& value) {
  self->data->line_width = value;
}

void set_draw_bar_border(Chart* self, const bool& v) {
  self->data->draw_bar_border = v;
}

void set_rounded_bars(Chart* self, const bool& v) {
  self->data->rounded_bars = v;
}

void set_fill_bars(Chart* self, const bool& v) {
  self->data->fill_bars = v;
}

void set_n_x_decimals(Chart* self, const int& v) {
  self->data->n_x_decimals = v;
}

void set_n_y_decimals(Chart* self, const int& v) {
  self->data->n_y_decimals = v;
}

void set_x_unit(Chart* self, const std::string& value) {
  self->data->x_unit = value;
}

void set_y_unit(Chart* self, const std::string& value) {
  self->data->y_unit = value;
}

void set_margin(Chart* self, const double& v) {
  self->data->margin = v;
}

auto get_is_visible(Chart* self) -> bool {
  return self->data->is_visible;
}

void set_x_data(Chart* self, const std::vector<float>& x) {
  if (self == nullptr || x.empty()) {
    return;
  }

  self->data->x_axis = x;

  self->data->x_min = std::ranges::min(x);
  self->data->x_max = std::ranges::max(x);

  self->data->objects_x.resize(x.size());

  self->data->x_min_log = std::log10(self->data->x_min);
  self->data->x_max_log = std::log10(self->data->x_max);

  self->data->x_axis_log.resize(x.size());

  for (size_t n = 0; n < self->data->x_axis_log.size(); n++) {
    self->data->x_axis_log[n] = std::log10(self->data->x_axis[n]);
  }

  // making each x value a number between 0 and 1

  std::ranges::for_each(self->data->x_axis,
                        [&](auto& v) { v = (v - self->data->x_min) / (self->data->x_max - self->data->x_min); });

  std::ranges::for_each(self->data->x_axis_log, [&](auto& v) {
    v = (v - self->data->x_min_log) / (self->data->x_max_log - self->data->x_min_log);
  });
}

void set_y_data(Chart* self, const std::vector<float>& y) {
  if (self == nullptr || y.empty()) {
    return;
  }

  self->data->y_axis = y;

  self->data->y_min = std::ranges::min(y);
  self->data->y_max = std::ranges::max(y);

  if (std::fabs(self->data->y_max - self->data->y_min) < 0.00001F) {
    std::ranges::fill(self->data->y_axis, 0.0F);
  } else {
    // making each y value a number between 0 and 1

    std::ranges::for_each(self->data->y_axis,
                          [&](auto& v) { v = (v - self->data->y_min) / (self->data->y_max - self->data->y_min); });
  }

  gtk_widget_queue_draw(GTK_WIDGET(self));
}

void on_pointer_motion(GtkEventControllerMotion* controller, double x, double y, Chart* self) {
  const int width = gtk_widget_get_allocated_width(GTK_WIDGET(self));
  const int height = gtk_widget_get_allocated_height(GTK_WIDGET(self));

  const auto usable_height = height - self->data->margin * height - self->data->x_axis_height;

  if (y < height - self->data->x_axis_height && y > self->data->margin * height && x > self->data->margin * width &&
      x < width - self->data->margin * width) {
    // At least for now the y axis is always linear

    self->data->mouse_y =
        (usable_height - y) / usable_height * (self->data->y_max - self->data->y_min) + self->data->y_min;

    switch (self->data->chart_scale) {
      case ChartScale::logarithmic: {
        const double& mouse_x_log = (x - self->data->margin * width) /
                                        static_cast<double>(width - 2 * self->data->margin * width) *
                                        (self->data->x_max_log - self->data->x_min_log) +
                                    self->data->x_min_log;

        self->data->mouse_x = std::pow(10.0, mouse_x_log);  // exp10 does not exist on FreeBSD

        break;
      }
      case ChartScale::linear: {
        self->data->mouse_x = (x - self->data->margin * width) /
                                  static_cast<double>(width - 2 * self->data->margin * width) *
                                  (self->data->x_max - self->data->x_min) +
                              self->data->x_min;

        break;
      }
    }

    gtk_widget_queue_draw(GTK_WIDGET(self));
  }
}

auto draw_unit(Chart* self, GtkSnapshot* snapshot, const int& width, const int& height, const std::string& unit) {
  auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), unit.c_str());

  auto* description = pango_font_description_from_string("monospace bold");

  pango_layout_set_font_description(layout, description);
  pango_font_description_free(description);

  int text_width = 0;
  int text_height = 0;

  pango_layout_get_pixel_size(layout, &text_width, &text_height);

  gtk_snapshot_save(snapshot);

  auto point = GRAPHENE_POINT_INIT(static_cast<float>(width - text_width), static_cast<float>(height - text_height));

  gtk_snapshot_translate(snapshot, &point);

  gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

  gtk_snapshot_restore(snapshot);

  g_object_unref(layout);
}

auto draw_x_labels(Chart* self, GtkSnapshot* snapshot, const int& width, const int& height) -> int {
  float labels_offset = 0.1 * width;

  int n_x_labels = static_cast<int>(std::ceil((width - 2 * self->data->margin * width) / labels_offset)) + 1;

  if (n_x_labels < 2) {
    return 0;
  }

  /*
    Correcting the offset based on the final n_x_labels value
  */

  labels_offset = (width - 2 * self->data->margin * width) / static_cast<double>(n_x_labels - 1);

  std::vector<float> labels;

  switch (self->data->chart_scale) {
    case ChartScale::logarithmic: {
      labels = util::logspace(self->data->x_min, self->data->x_max, n_x_labels);

      break;
    }
    case ChartScale::linear: {
      labels = util::linspace(self->data->x_min, self->data->x_max, n_x_labels);

      break;
    }
  }

  draw_unit(self, snapshot, width, height, self->data->x_unit);

  /*
    There is no space left in the window to show the last label. So we skip it
  */

  for (size_t n = 0U; n < labels.size() - 1; n++) {
    const auto msg = fmt::format("{0:.{1}f}", labels[n], self->data->n_x_decimals);

    auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), msg.c_str());

    auto* description = pango_font_description_from_string("monospace bold");

    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);

    int text_width = 0;
    int text_height = 0;

    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    gtk_snapshot_save(snapshot);

    auto point =
        GRAPHENE_POINT_INIT(self->data->margin * width + n * labels_offset, static_cast<float>(height - text_height));

    gtk_snapshot_translate(snapshot, &point);

    gtk_snapshot_append_layout(snapshot, layout, &self->data->color_axis_labels);

    gtk_snapshot_restore(snapshot);

    g_object_unref(layout);

    if (n == labels.size() - 2U) {
      return text_height;
    }
  }

  return 0;
}

void snapshot(GtkWidget* widget, GtkSnapshot* snapshot) {
  auto* self = EE_CHART(widget);

  int width = gtk_widget_get_width(widget);
  int height = gtk_widget_get_height(widget);

  auto widget_rectangle = GRAPHENE_RECT_INIT(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height));

  gtk_snapshot_append_color(snapshot, &self->data->background_color, &widget_rectangle);

  if (const auto n_points = self->data->y_axis.size(); n_points > 0) {
    double usable_width = width - 2 * (self->data->line_width + self->data->margin * width);

    int usable_height = static_cast<int>(height - self->data->margin * height) - self->data->x_axis_height;

    switch (self->data->chart_scale) {
      case ChartScale::logarithmic: {
        for (size_t n = 0; n < n_points; n++) {
          self->data->objects_x[n] =
              usable_width * self->data->x_axis_log[n] + self->data->line_width + self->data->margin * width;
        }

        break;
      }
      case ChartScale::linear: {
        for (size_t n = 0; n < n_points; n++) {
          self->data->objects_x[n] =
              usable_width * self->data->x_axis[n] + self->data->line_width + self->data->margin * width;
        }

        break;
      }
    }

    self->data->x_axis_height = draw_x_labels(self, snapshot, width, height);

    switch (self->data->chart_type) {
      case ChartType::bar: {
        for (uint n = 0U; n < n_points; n++) {
          double bar_height = static_cast<double>(usable_height) * self->data->y_axis[n];

          float rect_x = self->data->objects_x[n];
          float rect_y = self->data->margin * height + static_cast<float>(usable_height) - bar_height;
          float rect_height = bar_height;
          float rect_width = static_cast<float>(width) / static_cast<float>(n_points);

          if (self->data->draw_bar_border) {
            rect_width -= self->data->line_width;
          }

          auto bar_rectangle = GRAPHENE_RECT_INIT(rect_x, rect_y, rect_width, rect_height);

          float radius = (self->data->rounded_bars) ? 5.0F : 0.0F;

          // auto bar_rectangle = GRAPHENE_RECT_INIT(rect_x - radius, rect_y - radius, rect_width, rect_width);

          GskRoundedRect outline;

          gsk_rounded_rect_init_from_rect(&outline, &bar_rectangle, radius);

          if (self->data->fill_bars) {
            gtk_snapshot_push_rounded_clip(snapshot, &outline);

            gtk_snapshot_append_color(snapshot, &self->data->color, &outline.bounds);

            gtk_snapshot_pop(snapshot);
          } else {
            float lw = static_cast<float>(self->data->line_width);

            auto border_width = std::to_array({lw, lw, lw, lw});

            auto border_color =
                std::to_array({self->data->color, self->data->color, self->data->color, self->data->color});

            gtk_snapshot_append_border(snapshot, &outline, border_width.data(), border_color.data());
          }
        }

        break;
      }
      case ChartType::line: {
        auto* ctx = gtk_snapshot_append_cairo(snapshot, &widget_rectangle);

        cairo_set_source_rgba(ctx, self->data->color.red, self->data->color.green, self->data->color.blue,
                              self->data->color.alpha);

        if (self->data->fill_bars) {
          cairo_move_to(ctx, self->data->margin * width,
                        self->data->margin * height + static_cast<float>(usable_height));
        } else {
          const auto point_height = self->data->y_axis.front() * static_cast<float>(usable_height);

          cairo_move_to(ctx, self->data->objects_x.front(),
                        self->data->margin * height + static_cast<float>(usable_height) - point_height);
        }

        for (uint n = 0U; n < n_points - 1U; n++) {
          const auto next_point_height = self->data->y_axis[n + 1] * static_cast<float>(usable_height);

          cairo_line_to(ctx, self->data->objects_x[n + 1],
                        self->data->margin * height + static_cast<float>(usable_height) - next_point_height);
        }

        if (self->data->fill_bars) {
          cairo_line_to(ctx, self->data->objects_x.back(),
                        self->data->margin * height + static_cast<float>(usable_height));

          cairo_move_to(ctx, self->data->objects_x.back(),
                        self->data->margin * height + static_cast<float>(usable_height));

          cairo_close_path(ctx);
        }

        cairo_set_line_width(ctx, self->data->line_width);

        if (self->data->fill_bars) {
          cairo_fill(ctx);
        } else {
          cairo_stroke(ctx);
        }

        cairo_destroy(ctx);

        break;
      }
    }

    if (gtk_event_controller_motion_contains_pointer(GTK_EVENT_CONTROLLER_MOTION(self->controller_motion)) != 0) {
      const auto msg =
          fmt::format("x = {0:.{1}f} {2} y = {3:.{4}f} {5}", self->data->mouse_x, self->data->n_x_decimals,
                      self->data->x_unit, self->data->mouse_y, self->data->n_y_decimals, self->data->y_unit);

      auto* layout = gtk_widget_create_pango_layout(GTK_WIDGET(self), msg.c_str());

      auto* description = pango_font_description_from_string("monospace bold");

      pango_layout_set_font_description(layout, description);
      pango_font_description_free(description);

      int text_width = 0;
      int text_height = 0;

      pango_layout_get_pixel_size(layout, &text_width, &text_height);

      gtk_snapshot_save(snapshot);

      auto point = GRAPHENE_POINT_INIT(static_cast<float>(width) - static_cast<float>(text_width), 0.0F);

      gtk_snapshot_translate(snapshot, &point);

      gtk_snapshot_append_layout(snapshot, layout, &self->data->color);

      gtk_snapshot_restore(snapshot);

      g_object_unref(layout);
    }
  }
}

void unroot(GtkWidget* widget) {
  auto* self = EE_CHART(widget);

  self->data->is_visible = false;

  GTK_WIDGET_CLASS(chart_parent_class)->unmap(widget);
}

void finalize(GObject* object) {
  auto* self = EE_CHART(object);

  delete self->data;

  util::debug(log_tag + "finalized"s);

  G_OBJECT_CLASS(chart_parent_class)->finalize(object);
}

void chart_class_init(ChartClass* klass) {
  auto* object_class = G_OBJECT_CLASS(klass);
  auto* widget_class = GTK_WIDGET_CLASS(klass);

  object_class->finalize = finalize;

  widget_class->snapshot = snapshot;
  widget_class->unroot = unroot;

  gtk_widget_class_set_template_from_resource(widget_class, "/com/github/wwmm/easyeffects/ui/chart.ui");
}

void chart_init(Chart* self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  self->data = new Data();

  self->data->draw_bar_border = true;
  self->data->fill_bars = true;
  self->data->is_visible = true;
  self->data->x_axis_height = 0;
  self->data->n_x_decimals = 1;
  self->data->n_y_decimals = 1;
  self->data->line_width = 2.0F;
  self->data->margin = 0.02;

  self->data->x_min = 0.0F;
  self->data->y_min = 0.0F;
  self->data->x_max = 1.0F;
  self->data->y_max = 1.0F;

  self->data->background_color = GdkRGBA{0.0F, 0.0F, 0.0F, 1.0F};
  self->data->color = GdkRGBA{1.0F, 1.0F, 1.0F, 1.0F};
  self->data->color_axis_labels = GdkRGBA{1.0F, 1.0F, 1.0F, 1.0F};
  self->data->gradient_color = GdkRGBA{1.0F, 1.0F, 1.0F, 1.0F};

  self->data->chart_type = ChartType::bar;
  self->data->chart_scale = ChartScale::logarithmic;

  self->controller_motion = gtk_event_controller_motion_new();

  g_signal_connect(self->controller_motion, "motion", G_CALLBACK(on_pointer_motion), self);

  g_signal_connect(GTK_WIDGET(self), "hide",
                   G_CALLBACK(+[](GtkWidget* widget, Chart* self) { self->data->is_visible = false; }), self);

  g_signal_connect(GTK_WIDGET(self), "show",
                   G_CALLBACK(+[](GtkWidget* widget, Chart* self) { self->data->is_visible = true; }), self);

  gtk_widget_add_controller(GTK_WIDGET(self), self->controller_motion);
}

auto create() -> Chart* {
  return static_cast<Chart*>(g_object_new(EE_TYPE_CHART, nullptr));
}

}  // namespace ui::chart