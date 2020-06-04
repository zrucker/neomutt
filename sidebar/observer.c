/**
 * @file
 * Sidebar observers
 *
 * @authors
 * Copyright (C) 2020 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page sidebar_observers Sidebar observers
 *
 * Sidebar observers
 */

#include "config.h"
#include <stdbool.h>
#include <string.h>
#include "private.h"
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "lib.h"
#include "init.h"
#include "mutt_globals.h"

struct Command;

/**
 * account_event - Handle an Account Event
 * @param win Sidebar Window
 * @param ea  Account Event data
 * @retval 0 Always
 */
static int account_event(struct MuttWindow *win, struct EventAccount *ea)
{
  // NT_ACCOUNT
  //   NT_ACCOUNT_ADD
  //   NT_ACCOUNT_REMOVE

  // if (nc->event_type == NT_ACCOUNT)
  // {
  //   struct EventAccount *ea = nc->event_data;
  //   return sb_account(win, ea);
  // }

  return 0;
}

/**
 * color_event - Handle a colour-change Event
 * @param win Sidebar Window
 * @param cid Colour that changed, e.g. #MT_COLOR_SIDEBAR_HIGHLIGHT
 * @param ec  Type of change
 * @retval num 0 Always
 */
static int color_event(struct MuttWindow *win, enum ColorId cid, struct EventColor *ec)
{
  // Default colour changed
  if (cid == MT_COLOR_NORMAL)
  {
    win->actions |= WA_RECALC;
    return 0;
  }

  // Indicator colour changed and Sidebar indicator colour isn't set
  if ((cid == MT_COLOR_INDICATOR) && (Colors->defs[MT_COLOR_SIDEBAR_INDICATOR] == 0))
  {
    win->actions |= WA_RECALC;
    return 0;
  }

  // A non-Sidebar colour changed
  if ((cid < MT_COLOR_SIDEBAR_DIVIDER) || (cid > MT_COLOR_SIDEBAR_UNREAD))
    return 0;

  // Sidebar colour isn't set
  if (Colors->defs[cid] == 0)
    return 0;

  win->actions |= WA_RECALC;
  return 0;
}

/**
 * command_event - Handle a Sidebar Command Event
 * @param win Sidebar Window
 * @param cmd Command
 * @retval num 0 Always
 */
static int command_event(struct MuttWindow *win, struct Command *cmd)
{
  static const struct Command *wl = NULL;
  static const struct Command *uwl = NULL;

  if (!wl)
  {
    wl = mutt_command_get("sidebar_whitelist");
    uwl = mutt_command_get("unsidebar_whitelist");
  }

  if ((cmd != wl) && (cmd != uwl))
    return 0;

  win->actions |= WA_RECALC;
  return 0;
}

/**
 * config_event - Handle a Config change
 * @param win Sidebar Window
 * @param ec  Config event data
 * @retval 0 Always
 */
static int config_event(struct MuttWindow *win, struct EventConfig *ec)
{
  //   ascii_chars                    - recalc divider
  //   folder                         - recalc text
  //   spoolfile                      - recalc colours
  if ((strcmp(ec->name, "ascii_chars") == 0) ||
      (strcmp(ec->name, "folder") == 0) || (strcmp(ec->name, "spoolfile") == 0))
  {
    win->actions |= WA_RECALC;
    return 0;
  }

  if (mutt_str_startswith(ec->name, "sidebar_") != 8)
    return 0;

  //   sidebar_next_new_wrap          - NO ACTION
  if (strcmp(ec->name, "sidebar_next_new_wrap") == 0)
    return 0;

  //   sidebar_on_right               - reflow | repaint
  if (strcmp(ec->name, "sidebar_on_right") == 0)
  {
    struct MuttWindow *parent = win->parent;
    struct MuttWindow *first = TAILQ_FIRST(&parent->children);

    if ((C_SidebarOnRight && (first == win)) || (!C_SidebarOnRight && (first != win)))
    {
      // Swap the Sidebar and the Container of the Index/Pager
      TAILQ_REMOVE(&parent->children, first, entries);
      TAILQ_INSERT_TAIL(&parent->children, first, entries);
    }

    win->actions |= WA_REFLOW;
    return 0;
  }

  //   sidebar_visible                - reflow
  if (strcmp(ec->name, "sidebar_visible") == 0)
  {
    window_set_visible(win, C_SidebarVisible);
    win->actions |= WA_REFLOW;
    return 0;
  }

  //   sidebar_width                  - reflow | recalc text
  if (strcmp(ec->name, "sidebar_width") == 0)
  {
    win->req_cols = C_SidebarWidth;
    win->actions |= WA_REFLOW;
    return 0;
  }

  //   sidebar_component_depth        - recalc text
  //   sidebar_delim_chars            - recalc text
  //   sidebar_divider_char           - recalc divider
  //   sidebar_folder_indent          - recalc text
  //   sidebar_format                 - recalc text
  //   sidebar_indent_string          - recalc text
  //   sidebar_new_mail_only          - recalc text
  //   sidebar_non_empty_mailbox_only - recalc text
  //   sidebar_short_path             - recalc text
  //   sidebar_sort_method            - recalc text
  win->actions |= WA_RECALC;

  return 0;
}

/**
 * mailbox_event - Handle a Mailbox Event
 * @param win     Sidebar Window
 * @param subtype Event subtype, e.g. #NT_MAILBOX_ADD
 * @param em      Mailbox Event data
 * @retval 0 Always
 */
static int mailbox_event(struct MuttWindow *win, enum NotifyMailbox subtype,
                         struct EventMailbox *em)
{
  // NT_MAILBOX
  //   NT_MAILBOX_ADD
  //   NT_MAILBOX_REMOVE
  //   NT_MAILBOX_CLOSED
  //   NT_MAILBOX_INVALID
  //   NT_MAILBOX_RESORT
  //   NT_MAILBOX_SWITCH
  //   NT_MAILBOX_UPDATE
  //   NT_MAILBOX_UNTAG

  struct SidebarWindowData *wdata = sb_wdata_get(win);
  if (subtype == NT_MAILBOX_ADD)
  {
    sb_notify_mailbox(win, wdata, em->mailbox, SBN_CREATED);
    mutt_debug(LL_DEBUG1, "notify: mailbox add\n");
  }
  else if (subtype == NT_MAILBOX_REMOVE)
  {
    sb_notify_mailbox(win, wdata, em->mailbox, SBN_DELETED);
    mutt_debug(LL_DEBUG1, "notify: mailbox remove\n");
  }
  else if (subtype == NT_MAILBOX_SWITCH)
  {
    sb_set_open_mailbox(win, em->mailbox);
    mutt_debug(LL_DEBUG1, "notify: mailbox switch\n");
  }
  else if (subtype == NT_MAILBOX_CHANGED)
  {
    win->actions |= WA_RECALC;
  }
  else
  {
    mutt_debug(LL_DEBUG1, "notify: mailbox UNKNOWN\n");
  }

  return 0;
}

/**
 * window_event - Handle a change to the Sidebar Window
 * @param win Sidebar Window
 * @param ew  Window Event data
 * @retval 0 Always
 */
static int window_event(struct MuttWindow *win, struct EventWindow *ew)
{
  // NT_WINDOW
  //   NT_WINDOW_NEW
  //   NT_WINDOW_DELETE
  //   NT_WINDOW_STATE

  return 0;
}

/**
 * sb_dialog_observer - Listen for changes to the Index Dialog - Implements ::observer_t
 */
int sb_dialog_observer(struct NotifyCallback *nc)
{
  if (!nc->event_data || !nc->global_data)
    return -1;
  if (nc->event_type != NT_WINDOW)
    return 0;

  struct MuttWindow *win = nc->global_data;
  // struct SidebarWindowData *sb_data = win->wdata;

  return window_event(win, nc->event_data);

  // if (!nc->event_data || !nc->global_data)
  //   return -1;
  // if (nc->event_type != NT_WINDOW)
  //   return 0;
  // if (nc->event_subtype != NT_WINDOW_DELETE)
  //   return 0;

  // struct MuttWindow *win_sidebar = nc->global_data;

  // notify_observer_remove(NeoMutt->notify, sb_neomutt_event, win_sidebar);

  // if (!nc->event_data || !nc->global_data)
  //   return -1;
  // if (nc->event_type != NT_WINDOW)
  //   return 0;
  // if (nc->event_subtype != NT_INDEX_MAILBOX)
  //   return 0;

  // struct MuttWindow *win_sidebar = nc->global_data;
  // struct SidebarWindowData *sb_data = win_sidebar->wdata;
  // if (!sb_data)
  //   return -1;

  // win_sidebar: flag WA_RECALC
  // sb_data: update ptr to current Mailbox
  // eid: don't need this, wait for call to calc()

  return 0;
}

/**
 * sb_insertion_observer - Listen for new Dialogs - Implements ::observer_t
 */
int sb_insertion_observer(struct NotifyCallback *nc)
{
  if ((nc->event_type != NT_WINDOW) || (nc->event_subtype != NT_WINDOW_DIALOG))
    return 0;

  struct EventWindow *ew = nc->event_data;
  if (ew->win->type != WT_DLG_INDEX)
    return 0;

  if (ew->flags & WN_VISIBLE)
    sb_win_init(ew->win);
  else if (ew->flags & WN_HIDDEN)
    sb_win_shutdown(ew->win);

  return 0;
}

/**
 * sb_neomutt_observer - Listen for global changes - Implements ::observer_t
 */
int sb_neomutt_observer(struct NotifyCallback *nc)
{
  struct MuttWindow *win = nc->global_data;
  // struct SidebarWinData *wdata = win ? win->wdata : NULL;

  switch (nc->event_type)
  {
    case NT_ACCOUNT:
      return account_event(win, nc->event_data);
    case NT_COLOR:
      return color_event(win, nc->event_subtype, nc->event_data);
    case NT_COMMAND:
      return command_event(win, nc->event_data);
    case NT_CONFIG:
      return config_event(win, nc->event_data);
    case NT_MAILBOX:
      return mailbox_event(win, nc->event_subtype, nc->event_data);
    default:
      return 0; // Ignore everything else
  }
}
