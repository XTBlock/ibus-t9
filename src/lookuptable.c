/*
 * lookuptable.c
 *
 *  Created on: 2010-1-30
 *      Author: cai
 */
#include <config.h>

#ifdef HAVE_GETTEXT

#include <locale.h>
#include <libintl.h>

#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#else

#define _(x) (x)

#endif

#include <string.h>
#include <cairo/cairo.h>
#include "engine.h"
#include <librsvg/rsvg-cairo.h>
#include "lookuptable.h"

#define YFORSVG  63

static void
svg_set_size(gint *width, gint *height, gpointer user_data)
{
//  g_print("w = %d h = %d\n", *width, *height);
  *width = *height = GPOINTER_TO_SIZE(user_data);
}

static void
draw_inputed(GdkDrawable * gw, GdkGC * gc, GString * inputed,RsvgHandle * icons[])
{
  int i;
  GdkPixbuf * px;
  for (i = 0; i < 5; i++)
    rsvg_handle_set_size_callback(icons[i], svg_set_size,GSIZE_TO_POINTER(14), 0);

  for (i = 0; i < inputed ->len; i++)
    {
      switch (inputed->str[i])
        {
      case 'h':
        px = rsvg_handle_get_pixbuf(icons[0]);
        break;
      case 's':
        px = rsvg_handle_get_pixbuf(icons[1]);
        break;
      case 'p':
        px = rsvg_handle_get_pixbuf(icons[2]);
        break;
      case 'n':
        px = rsvg_handle_get_pixbuf(icons[3]);
        break;
      case 'z':
        px = rsvg_handle_get_pixbuf(icons[4]);
        break;
        }
      gdk_draw_pixbuf(gw,gc,px,0,0,i*15,2,14,14,GDK_RGB_DITHER_NONE,0,0);

      g_object_unref(px);
    }
}

gboolean
on_paint(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{

  GdkGC * gc;
  GdkPixbuf * pixbuf;
  IBusT9Engine * engine;
  GdkWindow* gw;

  PangoLayout * ly;

  engine = (IBusT9Engine *) (user_data);

  int size = 37;

  gw = widget->window;

  gc = gdk_gc_new(gw);

  int i;

  for (i = 0; i < 5; i++) //画笔画
    {
      //按下状态还是不一样的
      rsvg_handle_set_size_callback(engine->keysicon[i], svg_set_size,
          GSIZE_TO_POINTER(37), 0);

      pixbuf = rsvg_handle_get_pixbuf_sub(engine->keysicon[i],
          engine->iconstate[i] ? "#layer2" : "#layer1");

//      gdk_draw_rectangle(gw,gc,1,i * 40 + 1, YFORSVG, 37, 37);

      if (!pixbuf)
        {
          pixbuf = rsvg_handle_get_pixbuf(engine->keysicon[i]);
        }

      gdk_draw_pixbuf(gw, gc, pixbuf, 0, 0, i * 40 + 1, YFORSVG, 37, 37,
          GDK_RGB_DITHER_NONE, 0, 0);

      g_object_unref(pixbuf);
    }
  //画已经输入的笔画
  if (widget, engine->inputed->len)
    {
      draw_inputed(gw,gc,engine->inputed,engine->keysicon);
    }

  //画候选字

  int len = engine->matched->len;
  if (len > 10)
    len = 10;

  for (i = 0; i < len; ++i)
    {

      char phrase[32];
      snprintf(phrase,32,"%d.%s",i,g_array_index(engine->matched,MATCHED,i).hanzi);

      ly = gtk_widget_create_pango_layout(widget,phrase);

      gdk_draw_layout(gw, gc, 40 *( i % 5) + 5, (i /5 ) * 22 + 20, ly);

      g_object_unref(ly);

    }

//  g_object_unref(ly);
  g_object_unref(gc);
  return TRUE;

}

static gboolean
time_out(gpointer user_data)
{
  int i;
  IBusT9Engine * engine;

  engine = (IBusT9Engine *) (user_data);

  for (i = 0; i < 5; ++i)
    {
      engine->iconstate[i] = 0;
    }

  return FALSE;
}

gboolean
on_mouse_move(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  int i;
  IBusT9Engine * engine;
  GdkWindow * gw;
  GdkRectangle regtangle;
  GdkRegion * reg;

  engine = (IBusT9Engine *) (user_data);

  gw = widget->window;

  if (engine->drag)
    {
      engine->laststate.x = event->x_root - engine->lastpoint.x;
      engine->laststate.y = event->y_root - engine->lastpoint.y;
      gtk_window_move(GTK_WINDOW(widget), engine->laststate.x,
          engine->laststate.y);
    }
  else
    {
        g_print("move\n");
//        return TRUE;
//      for (i = 0; i < 5; ++i)
//        {
//          regtangle.height = regtangle.width = 37;
//          regtangle.y = YFORSVG;
//          regtangle.x = i * 40 + 3;
//
//          reg = gdk_region_rectangle(&regtangle);
//
//          g_print("state[%d] = %d \n ",i,engine->iconstate[i] = gdk_region_point_in(reg, event->x, event->y));
//
//          gdk_region_destroy(reg);
//        }
//      gdk_window_invalidate_rect(gw, 0, 0);
    }
  return TRUE;
}

gboolean
on_button(GtkWidget* widget, GdkEventButton *event, gpointer user_data)
{
  int i;
  IBusT9Engine * engine;
  IBusT9EngineClass   *klass;
  GdkRegion * reg;
  GdkRectangle  regtangle;

  engine = (IBusT9Engine *) (user_data);
  klass = IBUS_T9_ENGINE_GET_CLASS(engine);

  engine->drag = event->button != 1;
  switch (event->type)
    {

  case GDK_BUTTON_PRESS:
    if (event->button != 1)
      {
        engine->lastpoint.x = event->x;
        engine->lastpoint.y = event->y;
        return FALSE;
      }
    else
      {
        for (i = 0; i < 5; ++i)
          {
            regtangle.height = regtangle.width = 37;
            regtangle.y = YFORSVG;
            regtangle.x = i * 40 + 3;

            reg = gdk_region_rectangle(&regtangle);

            engine ->iconstate[i]
                = gdk_region_point_in(reg, event->x, event->y);

       //     g_timeout_add(500, time_out, engine);

            static char bihua[5][8] =
              { "横", "竖", "撇", "捺", "折" };
            if (engine ->iconstate[i])
              {
                g_printf("%s clicked\n", bihua[i]);
//                g_signal_emit_by_name(engine,"process_key_event",engine,IBUS_KP_0,0,0,&val);
                klass->parent.process_key_event(IBUS_ENGINE(engine), IBUS_KP_1 + i, IBUS_KP_1 + i, 0);
              }
            gdk_region_destroy(reg);
          }
      }
    break;
  case GDK_BUTTON_RELEASE:
    for (i = 0; i < 5; ++i)
      {
        engine->iconstate[i] = 0;
      }
    break;
    }
  gdk_window_invalidate_rect(widget->window, 0, 0);
  return TRUE;
}
