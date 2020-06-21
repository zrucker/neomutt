/**
 * @file
 *
 * @authors
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

#ifndef MUTT_BACKGROUND_H
#define MUTT_BACKGROUND_H

struct SendContext;

extern struct SendContext *BackgroundProcess;

int mutt_background_edit_file(struct SendContext *sctx, const char *editor, const char *filename);

#endif /* MUTT_BACKGROUND_H */
