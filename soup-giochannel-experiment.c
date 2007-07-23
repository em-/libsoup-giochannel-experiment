/*
 * soup-giochannel-experiment.c - Experimenting with LibSoup and GIOChannel
 * Copyright (C) 2007 Emanuele Aina <em@nerd.ocracy.org>
 *                    Marco Barisione <marco@barisione.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsoup/soup.h>
#include <libsoup/soup-server.h>
#include <libsoup/soup-server-message.h>

static GMainLoop *loop;

/*
 * Data is available from the channel so we can send it.
 */
static gboolean
input_channel_readable_cb (GIOChannel *source,
                           GIOCondition condition,
                           gpointer user_data)
{
  SoupMessage *msg = user_data;
  GIOStatus status;
  gchar *buff;
  gsize bytes_read;

#define BUFF_SIZE 32
  g_printerr ("INPUT READABLE\n");

  if (condition & G_IO_IN)
    {
      buff = g_malloc (BUFF_SIZE);
      status = g_io_channel_read_chars (source, buff, BUFF_SIZE,
          &bytes_read, NULL);
      switch (status)
        {
        case G_IO_STATUS_NORMAL:
          soup_message_add_chunk (msg, SOUP_BUFFER_SYSTEM_OWNED,
              buff, bytes_read);
          soup_message_io_unpause (msg);
          g_printerr ("WRITTEN %d bytes\n", bytes_read);
          return FALSE;
        case G_IO_STATUS_AGAIN:
          g_printerr ("AGAIN\n");
          g_free (buff);
          break;
        case G_IO_STATUS_EOF:
          g_printerr ("EOF\n");
          g_free (buff);
          goto stop;
        default:
          g_printerr ("ERROR\n");
          g_free (buff);
          goto stop;
      }
    }

#undef BUFF_SIZE

stop:
  g_printerr ("\nSTOP!\n\n");
  soup_message_add_final_chunk (msg);
  soup_message_io_unpause (msg);

  g_main_loop_quit (loop);

  return FALSE;
}

static void
http_server_wrote_chunk_cb (SoupMessage *msg,
                            gpointer user_data)
{
  GIOChannel *channel = user_data;

  g_printerr ("WROTE CHUNK, add watch on %p\n", channel);
  if (channel)
    {
      g_io_add_watch (channel, G_IO_IN | G_IO_HUP,
          input_channel_readable_cb, msg);
    }
}

static void
http_server_cb (SoupServerContext *context,
                SoupMessage *msg,
                gpointer user_data)
{
  GIOChannel *channel = user_data;
  const SoupUri *uri = soup_message_get_uri (msg);

  if (context->method_id != SOUP_METHOD_ID_GET)
    {
      soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
      return;
    }

  if (strcmp (uri->path, "/test") != 0)
    {
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

  soup_message_set_status (msg, SOUP_STATUS_OK);
  soup_server_message_set_encoding (SOUP_SERVER_MESSAGE (msg),
      SOUP_TRANSFER_CHUNKED);

  g_signal_connect (msg, "wrote-chunk",
      G_CALLBACK (http_server_wrote_chunk_cb), channel);

  http_server_wrote_chunk_cb (msg, channel);
}

int
main (int argc, char *argv[])
{
  SoupServer *server;
  GIOChannel *channel;

  g_type_init();
  g_thread_init(NULL);

  loop = g_main_loop_new(NULL, FALSE);

  channel = g_io_channel_unix_new (0);

  server = soup_server_new(SOUP_SERVER_PORT, 3333, NULL);

  soup_server_add_handler (server, "/test", NULL, http_server_cb, NULL, channel);

  g_printerr ("Run server\n");
  soup_server_run_async (server);

  g_main_loop_run(loop);

  return 0;
}

