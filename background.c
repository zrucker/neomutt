/**
 * @file
 *
 * @authors
 * Copyright(C) 1996-2000,2013 Michael R. Elkins <me@mutt.org>
 * Copyright(C) 2020 Kevin J. McCarthy <kevin@8t8.us>
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

#include "config.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "gui/lib.h"
#include "mutt.h"
#include "background.h"
#include "globals.h"
#include "mutt_menu.h"
#include "opcodes.h"
#include "protos.h"
#include "send.h"

struct SendContext *BackgroundProcess = NULL;

static const struct Mapping LandingHelp[] = {
  { N_("Exit"), OP_EXIT },
  { N_("Redraw"), OP_REDRAW },
  { N_("Help"), OP_HELP },
  { NULL, 0 },
};

/**
 * mutt_background_run - XXX
 */
static pid_t mutt_background_run(const char *cmd)
{
  struct sigaction act;
  pid_t thepid;
  int fd;

  if (!cmd || !*cmd)
    return (0);

  /* must ignore SIGINT and SIGQUIT */
  mutt_sig_block_system();

  if ((thepid = fork()) == 0)
  {
    /* give up controlling terminal */
    setsid();

    /* this ensures the child can't use stdin to take control of the
     * terminal */
#if defined(OPEN_MAX)
    for (fd = 0; fd < OPEN_MAX; fd++)
      close(fd);
#elif defined(_POSIX_OPEN_MAX)
    for (fd = 0; fd < _POSIX_OPEN_MAX; fd++)
      close(fd);
#else
    close(0);
    close(1);
    close(2);
#endif

    /* reset signals for the child; not really needed, but... */
    mutt_sig_unblock_system(0);
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
    sigaction(SIGCONT, &act, NULL);

    execle(EXEC_SHELL, "sh", "-c", cmd, NULL, mutt_envlist_getlist());
    _exit(127); /* execl error */
  }

  /* reset SIGINT, SIGQUIT and SIGCHLD */
  mutt_sig_unblock_system(1);

  return (thepid);
}

/**
 * landing_redraw - XXX
 */
static void landing_redraw(struct Menu *menu)
{
  menu_redraw(menu);
  mutt_window_mvaddstr(MuttIndexWindow, 0, 0, _("Waiting for editor to exit"));
  mutt_window_mvaddstr(MuttIndexWindow, 1, 0, _("Hit <exit> to background editor."));
}

/**
 * background_edit_landing_page - Display the "waiting for editor" page
 * @param
 *
 * Returns:
 *   2 if the the menu is exited, leaving the process backgrounded
 *   0 when the waitpid() indicates the process has exited
 */
static int background_edit_landing_page(pid_t bg_pid)
{
  bool done = false;
  int rc = 0, op;
  short orig_timeout;
  pid_t wait_rc;
  struct Menu *menu;
  char helpstr[256];

  menu = mutt_menu_new(MENU_GENERIC);
  menu->help = mutt_compile_help(helpstr, sizeof(helpstr), MENU_GENERIC, LandingHelp);
  menu->pagelen = 0;
  menu->title = _("Waiting for editor to exit");

  mutt_menu_push_current(menu);

  /* Reduce timeout so we poll with bg_pid every second */
  orig_timeout = C_Timeout;
  C_Timeout = 1;

  while (!done)
  {
    wait_rc = waitpid(bg_pid, NULL, WNOHANG);
    if ((wait_rc > 0) || ((wait_rc < 0) && (errno == ECHILD)))
    {
      rc = 0;
      break;
    }

#if defined(USE_SLANG_CURSES) || defined(HAVE_RESIZETERM)
    if (SigWinch)
    {
      SigWinch = 0;
      mutt_resize_screen();
      clearok(stdscr, TRUE);
    }
#endif

    if (menu->redraw)
      landing_redraw(menu);

    op = km_dokey(MENU_GENERIC);

    switch (op)
    {
      case OP_HELP:
        mutt_help(MENU_GENERIC);
        menu->redraw = REDRAW_FULL;
        break;

      case OP_EXIT:
        rc = 2;
        done = true;
        break;

      case OP_REDRAW:
        clearok(stdscr, TRUE);
        menu->redraw = REDRAW_FULL;
        break;
    }
  }

  C_Timeout = orig_timeout;

  mutt_menu_pop_current(menu);
  mutt_menu_free(&menu);

  return rc;
}

/**
 * mutt_background_edit_file - Run editor in the background
 * @param
 *
 * After backgrounding the process, the background landing page will
 * be displayed.  The user will have the opportunity to "quit" the
 * landing page, exiting back to the index.  That will return 2
 *(chosen for consistency with other backgrounding functions).
 *
 * If they leave the landing page up, it will detect when the editor finishes
 * and return 0, indicating the callers should continue processing
 * as if it were a foreground edit.
 *
 * Returns:
 *      2  - the edit was backgrounded
 *      0  - background edit completed.
 *     -1  - an error occurred
 */
int mutt_background_edit_file(struct SendContext *sctx, const char *editor, const char *filename)
{
  int rc = -1;

  struct Buffer *cmd = mutt_buffer_pool_get();

  mutt_buffer_file_expand_fmt_quote(cmd, editor, filename);
  pid_t pid = mutt_background_run(mutt_b2s(cmd));
  if (pid <= 0)
  {
    mutt_error(_("Error running \"%s\"!"), mutt_b2s(cmd));
    goto cleanup;
  }

  rc = background_edit_landing_page(pid);
  if (rc == 2)
  {
    sctx->background_pid = pid;
    BackgroundProcess = sctx;
  }

cleanup:
  mutt_buffer_pool_release(&cmd);
  return rc;
}
