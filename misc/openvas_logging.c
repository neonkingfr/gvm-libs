/* openvas-libraries/misc
 * $Id$
 * Description: Implementation of logging methods for openvas
 *
 * Authors:
 * Laban Mwangi <lmwangi@penguinlabs.co.ke>
 *
 * Copyright:
 * Copyright (C) 2009 PenguinLabs Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * or, at your option, any later version as published by the Free
 * Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @todo This module fulfils the reqirements to be placed in the base library.
 */

/**
 * @file openvas_logging.c
 * @brief Implementation of logging methods for OpenVAS.
 *
 * This file contains all methods needed for openvas logging. To enable logging,
 * methods in this file are called. Have a look at
 * openvas-server/openvassd/openvassd.c for an example.
 *
 * The module reuses glib datatypes and api for memory management and logging.
 */

#include <stdio.h> /* for fprintf */
#include <string.h> /* for strlen */
#include <stdlib.h> /* for atoi */
#include <unistd.h> /* for getpid */

#include "openvas_logging.h"

/**
 * @brief Returns time as specified in time_fmt strftime format.
 *
 * @param time_fmt ptr to the string format to use. The strftime
 *        man page documents the conversion specification. An
 *        example time_fmt string is "%Y-%m-%d %H:%M:%S".
 *
 * @return NULL in case the format string is NULL. A ptr to a
 *         string that contains the formatted date time value.
 *         This value must be freed using glib's g_free.
 */
gchar *
get_time (gchar * time_fmt)
{
  time_t now;
  struct tm *ts;
  gchar buf[80];

  /* Get the current time. */
  now = time (NULL);

  /* Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz." */
  ts = localtime (&now);
  strftime (buf, sizeof (buf), time_fmt, ts);

  return g_strdup_printf ("%s", buf);
}

/**
 * @brief Return the integer corresponding to a log level string.
 *
 * @param level Level name or integer.
 *
 * @return Log level integer if level matches a level name, else 0.
 */
static gint
level_int_from_string (const gchar * level)
{
  if (level && strlen (level) > 0)
    {
      if (level[0] >= '0' && level[0] <= '9')
        return atoi (level);
      if (strcasecmp (level, "critical") == 0)
        return G_LOG_LEVEL_CRITICAL;
      if (strcasecmp (level, "debug") == 0)
        return G_LOG_LEVEL_DEBUG;
      if (strcasecmp (level, "error") == 0)
        return G_LOG_LEVEL_ERROR;
      if (strcasecmp (level, "info") == 0)
        return G_LOG_LEVEL_INFO;
      if (strcasecmp (level, "message") == 0)
        return G_LOG_LEVEL_MESSAGE;
      if (strcasecmp (level, "warning") == 0)
        return G_LOG_LEVEL_WARNING;
    }
  return 0;
}

/**
 * @brief Loads parameters from a config file into a linked list.
 *
 * @param config_file A string containing the path to the configuration file
 *                   to load.
 *
 * @return NULL in case the config file could not be loaded or an error
 *         occurred otherwise, a singly linked list of parameter groups
 *         is returned.
 */
GSList *
load_log_configuration (gchar * config_file)
{
  GKeyFile *key_file;
  GKeyFileFlags flags;
  GError *error = NULL;
  /* key_file *_has_* functions requires this. */

  // FIXME: If a g_* function that takes error fails, then free error.

  /* Groups found in the conf file. */
  gchar **groups;
  /* Temp variable to iterate over groups. */
  gchar **group;

  /* Structure to hold per group settings. */
  openvasd_logging *log_domain_entry;
  /* The link list for the structure above and it's tmp helper */
  GSList *log_domain_list = NULL;

  /* Create a new GKeyFile object and a bitwise list of flags. */
  key_file = g_key_file_new ();
  flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

  /* Load the GKeyFile from conf or return. */
  if (!g_key_file_load_from_file (key_file, config_file, flags, &error))
    {
      g_error ("%s:  %s", config_file, error->message);
    }

  /* Get all the groups available. */
  groups = g_key_file_get_groups (key_file, NULL);

  /* Point to the group head. */
  group = groups;
  /* Iterate till we get to the end of the array. */
  while (*group != NULL)
    {
      /* Create the struct. */
      log_domain_entry = g_malloc (sizeof (openvasd_logging));
      /* Set the logdomain. */
      log_domain_entry->log_domain = g_strdup (*group);
      /* Initialize everything else to NULL. */
      log_domain_entry->prepend_string = NULL;
      log_domain_entry->prepend_time_format = NULL;
      log_domain_entry->log_file = NULL;
      log_domain_entry->default_level = 0;
      log_domain_entry->log_channel = NULL;


      /* Look for the prepend string. */
      if (g_key_file_has_key (key_file, *group, "prepend", &error))
        {
          log_domain_entry->prepend_string =
            g_key_file_get_value (key_file, *group, "prepend", &error);
        }

      /* Look for the prepend time format string. */
      if (g_key_file_has_key (key_file, *group, "prepend_time_format", &error))
        {
          log_domain_entry->prepend_time_format =
            g_key_file_get_value (key_file, *group, "prepend_time_format",
                                  &error);
        }

      /* Look for the log file string. */
      if (g_key_file_has_key (key_file, *group, "file", &error))
        {
          log_domain_entry->log_file =
            g_key_file_get_value (key_file, *group, "file", &error);
        }

      /* Look for the prepend log level string. */
      if (g_key_file_has_key (key_file, *group, "level", &error))
        {
          gchar *level;

          level = g_key_file_get_value (key_file, *group, "level", &error);
          level = g_strchug (level);
          log_domain_entry->default_level = level_int_from_string (level);
          g_free (level);
        }

      /* Attach the struct to the list. */
      log_domain_list = g_slist_prepend (log_domain_list, log_domain_entry);
      group++;
    }
  /* Free the groups array. */
  g_strfreev (groups);

  /* Free the key file. */
  g_key_file_free (key_file);

  return log_domain_list;
}

/**
 * @brief Frees all resources loaded by the config loader.
 *
 * @param log_domain_list Head of the link list.
 */
void
free_log_configuration (GSList * log_domain_list)
{
  GSList *log_domain_list_tmp;
  openvasd_logging *log_domain_entry;

  /* Free the struct fields then the struct and then go the next
   * item in the link list.
   */

  /* Go the the head of the list. */
  log_domain_list_tmp = log_domain_list;
  while (log_domain_list_tmp != NULL)
    {
      /* Get the list data which is an openvasd_logging struct. */
      log_domain_entry = log_domain_list_tmp->data;

      /* Free the struct contents. */
      g_free (log_domain_entry->log_domain);
      g_free (log_domain_entry->prepend_string);
      g_free (log_domain_entry->prepend_time_format);
      g_free (log_domain_entry->log_file);

      /* Drop the reference to the GIOChannel. */
      if (log_domain_entry->log_channel)
        g_io_channel_unref (log_domain_entry->log_channel);

      /* Free the struct. */
      g_free (log_domain_entry);

      /* Go to the next item. */
      log_domain_list_tmp = g_slist_next (log_domain_list_tmp);

    }
  /* Free the link list. */
  g_slist_free (log_domain_list);
}

/**
 * @brief Creates the formatted string and outputs it to the log destination.
 *
 * @param log_domain A string containing the message's log domain.
 * @param log_level  Flags defining the message's log level.
 * @param message    A string containing the log message.
 * @param openvas_log_config_list A pointer to the configuration linked list.
 */
void
openvas_log_func (const char *log_domain, GLogLevelFlags log_level,
                  const char *message, gpointer openvas_log_config_list)
{
  gchar *prepend;
  gchar *prepend_buf;
  gchar *prepend_tmp;
  gchar *prepend_tmp1;
  gchar *tmp;

  /* For link list operations. */
  GSList *log_domain_list_tmp;
  openvasd_logging *log_domain_entry;

  /* Channel to log through. */
  GIOChannel *channel;
  GError *error = NULL;

  /* The default parameters to be used. The group '*' will override
   * these defaults if it's found.
   */
  gchar *prepend_format = "%p %t - ";
  gchar *time_format = "%Y-%m-%d %Hh%M.%S %Z";

  /** @todo Move log_separator to the conf file too. */
  gchar *log_separator = ":";
  gchar *log_file = "-";
  GLogLevelFlags default_level = G_LOG_LEVEL_DEBUG;
  channel = NULL;

  /* Let's load the default configuration file directives from the
   * linked list. Scanning the link list twice is inefficient but
   * leaves the source cleaner.
   */
  if (openvas_log_config_list != NULL && log_domain != NULL)
    {

      /* Go the the head of the list. */
      log_domain_list_tmp = (GSList *) openvas_log_config_list;

      while (log_domain_list_tmp != NULL)
        {
          openvasd_logging *entry;

          entry = log_domain_list_tmp->data;

          /* Override defaults if the current linklist group name is '*'. */
          if (g_ascii_strcasecmp (entry->log_domain, "*") == 0)
            {
              /* Get the list data for later use. */
              log_domain_entry = entry;

              /* Override defaults if the group items are not null. */
              if (log_domain_entry->prepend_string)
                prepend_format = log_domain_entry->prepend_string;
              if (log_domain_entry->prepend_time_format)
                time_format = log_domain_entry->prepend_time_format;
              if (log_domain_entry->log_file)
                log_file = log_domain_entry->log_file;
              if (log_domain_entry->default_level)
                default_level = log_domain_entry->default_level;
              if (log_domain_entry->log_channel)
                channel = log_domain_entry->log_channel;
              break;
            }

          /* Go to the next item. */
          log_domain_list_tmp = g_slist_next (log_domain_list_tmp);
        }
    }

  /* Let's load the configuration file directives if a linked list item for
   * the log domain group exists.
   */
  if (openvas_log_config_list != NULL && log_domain != NULL)
    {

      /* Go the the head of the list. */
      log_domain_list_tmp = (GSList *) openvas_log_config_list;

      while (log_domain_list_tmp != NULL)
        {
          openvasd_logging *entry;

          entry = log_domain_list_tmp->data;

          /* Search for the log domain in the link list. */
          if (g_ascii_strcasecmp (entry->log_domain, log_domain) == 0)
            {
              /* Get the list data which is an openvasd_logging struct. */
              log_domain_entry = entry;

              /* Get the struct contents. */
              prepend_format = log_domain_entry->prepend_string;
              time_format = log_domain_entry->prepend_time_format;
              log_file = log_domain_entry->log_file;
              default_level = log_domain_entry->default_level;
              channel = log_domain_entry->log_channel;
              break;
            }

          /* Go to the next item. */
          log_domain_list_tmp = g_slist_next (log_domain_list_tmp);
        }
    }

  /* If the current log entry is less severe than the specified log level,
   * let's exit.
   */
  if (default_level < log_level)
    return;


  /* Prepend buf is a newly allocated empty string. Makes life easier. */
  prepend_buf = g_strdup ("");


  /* Make the tmp pointer (for iteration) point to the format string. */
  tmp = prepend_format;

  while (*tmp != '\0')
    {
      /* If the current char is a % and the next one is a p, get the pid. */
      if ((*tmp == '%') && (*(tmp + 1) == 'p'))
        {
          /* Use g_strdup, a new string returned. Store it in a tmp var until
           * we free the old one. */
          prepend_tmp =
            g_strdup_printf ("%s%s%d", prepend_buf, log_separator,
                             (int) getpid ());
          /* Free the old string. */
          g_free (prepend_buf);
          /* Point the buf ptr to the new string. */
          prepend_buf = prepend_tmp;
          /* Skip over the two chars we've processed '%p'. */
          tmp += 2;
        }
      else if ((*tmp == '%') && (*(tmp + 1) == 't'))
        {
          /* Get time returns a newly allocated string.
           * Store it in a tmp var.
           */
          prepend_tmp1 = get_time (time_format);
          /* Use g_strdup. New string returned. Store it in a tmp var until
           * we free the old one.
           */
          prepend_tmp =
            g_strdup_printf ("%s%s%s", prepend_buf, log_separator,
                             prepend_tmp1);
          /* Free the time tmp var. */
          g_free (prepend_tmp1);
          /* Free the old string. */
          g_free (prepend_buf);
          /* Point the buf ptr to the new string. */
          prepend_buf = prepend_tmp;
          /* Skip over the two chars we've processed '%t.' */
          tmp += 2;
        }
      else
        {
          /* Jump to the next character. */
          tmp++;
        }
    }

  /* Step through all possible messages prefixing them with an appropriate
   * tag.
   */
  switch (log_level)
    {
    case G_LOG_FLAG_RECURSION:
      prepend = g_strdup_printf ("RECURSION%s", prepend_buf);
      break;

    case G_LOG_FLAG_FATAL:
      prepend = g_strdup_printf ("FATAL%s", prepend_buf);
      break;

    case G_LOG_LEVEL_ERROR:
      prepend = g_strdup_printf ("ERROR%s", prepend_buf);
      break;

    case G_LOG_LEVEL_CRITICAL:
      prepend = g_strdup_printf ("CRITICAL%s", prepend_buf);
      break;
    case G_LOG_LEVEL_WARNING:
      prepend = g_strdup_printf ("WARNING%s", prepend_buf);
      break;

    case G_LOG_LEVEL_MESSAGE:
      prepend = g_strdup_printf ("MESSAGE%s", prepend_buf);
      break;

    case G_LOG_LEVEL_INFO:
      prepend = g_strdup_printf ("   INFO%s", prepend_buf);
      break;

    case G_LOG_LEVEL_DEBUG:
      prepend = g_strdup_printf ("  DEBUG%s", prepend_buf);
      break;

    default:
      prepend = g_strdup_printf ("UNKWOWN%s", prepend_buf);
      break;
    }

  /* If the current log entry is more severe than the specified log level,
   * print out the message.
   */
  GString *log_str = g_string_new ("");
  g_string_append_printf (log_str,
                          "%s%s%s%s %s\n",
                          log_domain ? log_domain : "", log_separator,
                          prepend, log_separator, message);

  gchar *tmpstr = g_string_free (log_str, FALSE);
  /* Output everything to stderr if logfile is "-". */
  if (g_ascii_strcasecmp (log_file, "-") == 0)
    {
      fprintf (stderr, "%s", tmpstr);
      fflush (stderr);
    }
  else
    {
      /* Open a channel and store it in the struct or
       * retrieve and use an already existing channel.
       */
      if (channel == NULL)
        {
          channel = g_io_channel_new_file (log_file, "a", &error);
          if (!channel)
            {
              g_error ("Can not open '%s' logfile. %s", log_file,
                       error->message);
            }

          /* Store it in the struct for later use. */
          if (log_domain_entry != NULL)
            log_domain_entry->log_channel = channel;
        }
      g_io_channel_write_chars (channel, (const gchar *) tmpstr, -1, NULL,
                                &error);
      g_io_channel_flush (channel, NULL);

    }
  g_free (tmpstr);
  g_free (prepend_buf);
}

/**
 * @brief Sets up routing of logdomains to log handlers.
 *
 * Iterates over the link list and adds the groups to the handler.
 *
 * @param openvas_log_config_list A pointer to the configuration linked list.
 */
void
setup_log_handlers (GSList * openvas_log_config_list)
{
  GSList *log_domain_list_tmp;
  openvasd_logging *log_domain_entry;
  if (openvas_log_config_list != NULL)
    {
      /* Go to the head of the list. */
      log_domain_list_tmp = (GSList *) openvas_log_config_list;

      while (log_domain_list_tmp != NULL)
        {
          /* Get the list data which is an openvasd_logging struct. */
          log_domain_entry = log_domain_list_tmp->data;

          if (g_ascii_strcasecmp (log_domain_entry->log_domain, "*"))
            {
              g_log_set_handler (log_domain_entry->log_domain,
                                 (GLogLevelFlags) (G_LOG_LEVEL_DEBUG |
                                                   G_LOG_LEVEL_INFO |
                                                   G_LOG_LEVEL_MESSAGE |
                                                   G_LOG_LEVEL_WARNING |
                                                   G_LOG_LEVEL_CRITICAL |
                                                   G_LOG_LEVEL_ERROR |
                                                   G_LOG_FLAG_FATAL |
                                                   G_LOG_FLAG_RECURSION),
                                 (GLogFunc) openvas_log_func,
                                 openvas_log_config_list);
            }
          else
            {
              g_log_set_default_handler ((GLogFunc) openvas_log_func,
                                         openvas_log_config_list);
            }

          /* Go to the next item. */
          log_domain_list_tmp = g_slist_next (log_domain_list_tmp);
        }
    }
  g_log_set_handler ("",
                     (GLogLevelFlags) (G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO |
                                       G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING
                                       | G_LOG_LEVEL_CRITICAL |
                                       G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL |
                                       G_LOG_FLAG_RECURSION),
                     (GLogFunc) openvas_log_func,
                     openvas_log_config_list);
}
