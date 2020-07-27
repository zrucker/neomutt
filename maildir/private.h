/**
 * @file
 * Maildir/MH private types
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_MAILDIR_PRIVATE_H
#define MUTT_MAILDIR_PRIVATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

struct Account;
struct Buffer;
struct Email;
struct Mailbox;
struct Message;
struct Progress;

/**
 * struct MaildirEmailData - Maildir-specific Email data - @extends Email
 */
struct MaildirEmailData
{
  char *maildir_flags; ///< Unknown Maildir flags
};

/**
 * struct MaildirMboxData - Maildir-specific Mailbox data - @extends Mailbox
 */
struct MaildirMboxData
{
  struct timespec mtime_cur;
  mode_t mh_umask;
};

/**
 * struct MaildirEmailData - Maildir-specific Email data - @extends Email
 */
struct MaildirEmailData
{
  struct Email *email;
  char *canon_fname;
  bool header_parsed : 1;
  ino_t inode;
  struct MaildirEmailData *next;
};

typedef uint8_t MhSeqFlags;     ///< Flags, e.g. #MH_SEQ_UNSEEN
#define MH_SEQ_NO_FLAGS      0  ///< No flags are set
#define MH_SEQ_UNSEEN  (1 << 0) ///< Email hasn't been read
#define MH_SEQ_REPLIED (1 << 1) ///< Email has been replied to
#define MH_SEQ_FLAGGED (1 << 2) ///< Email is flagged

/**
 * struct MhSequences - Set of MH sequence numbers
 */
struct MhSequences
{
  int max;           ///< Number of flags stored
  MhSeqFlags *flags; ///< Flags for each email
};

extern bool  C_CheckNew;
extern bool  C_MaildirCheckCur;
extern bool  C_MaildirTrash;
extern bool  C_MhPurge;
extern char *C_MhSeqFlagged;
extern char *C_MhSeqReplied;
extern char *C_MhSeqUnseen;

/* MXAPI shared functions */
int             maildir_ac_add     (struct Account *a, struct Mailbox *m);
struct Account *maildir_ac_find    (struct Account *a, const char *path);
int             maildir_mbox_check (struct Mailbox *m);
int             maildir_path_canon (char *buf, size_t buflen);
int             maildir_path_parent(char *buf, size_t buflen);
int             maildir_path_pretty(char *buf, size_t buflen, const char *folder);
int             mh_mbox_check      (struct Mailbox *m);
int             mh_mbox_close      (struct Mailbox *m);
int             mh_mbox_sync       (struct Mailbox *m);
int             mh_msg_close       (struct Mailbox *m, struct Message *msg);
int             mh_msg_save_hcache (struct Mailbox *m, struct Email *e);

/* Maildir/MH shared functions */
void                    maildir_canon_filename (struct Buffer *dest, const char *src);
void                    maildir_edata_free     (void **ptr);
struct MaildirEmailData *maildir_edata_get     (struct Email *e);
struct MaildirEmailData *maildir_edata_new     (void);
void                    maildir_delayed_parsing(struct Mailbox *m, struct MaildirEmailData **md, struct Progress *progress);
size_t                  maildir_hcache_keylen  (const char *fn);
struct MaildirMboxData *maildir_mdata_get      (struct Mailbox *m);
int                     maildir_mh_open_message(struct Mailbox *m, struct Message *msg, int msgno, bool is_maildir);
int                     maildir_move_to_mailbox(struct Mailbox *m, struct MaildirEmailData **ptr);
int                     maildir_parse_dir      (struct Mailbox *m, struct MaildirEmailData ***last, const char *subdir, int *count, struct Progress *progress);
int                     md_commit_message      (struct Mailbox *m, struct Message *msg, struct Email *e);
int                     mh_commit_msg          (struct Mailbox *m, struct Message *msg, struct Email *e, bool updseq);
int                     mh_mkstemp             (struct Mailbox *m, FILE **fp, char **tgt);
int                     mh_read_dir            (struct Mailbox *m, const char *subdir);
int                     mh_read_sequences      (struct MhSequences *mhs, const char *path);
MhSeqFlags              mhs_check              (struct MhSequences *mhs, int i);
void                    mhs_sequences_free     (struct MhSequences *mhs);
MhSeqFlags              mhs_set                (struct MhSequences *mhs, int i, MhSeqFlags f);
mode_t                  mh_umask               (struct Mailbox *m);
void                    mh_update_maildir      (struct MaildirEmailData *md, struct MhSequences *mhs);
void                    mh_update_sequences    (struct Mailbox *m);
bool                    mh_valid_message       (const char *s);

int mh_sync_message(struct Mailbox *m, int msgno);
int maildir_sync_message(struct Mailbox *m, int msgno);
int mh_rewrite_message(struct Mailbox *m, int msgno);

#endif /* MUTT_MAILDIR_PRIVATE_H */
