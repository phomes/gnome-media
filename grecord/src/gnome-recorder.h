/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *
 *  Copyright 2002,2003,2004,2005 Iain Holmes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * 4th Februrary 2005: Christian Schaller: changed license to LGPL with
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dahlin *  and Joe Marcus Clarke
 *
 */

#ifndef __GNOME_RECORDER_H__
#define __GNOME_RECORDER_H__

#include "gsr-window.h"

#define GSR_STOCK_PLAY "media-play"
#define GSR_STOCK_STOP "media-stop"
#define GSR_STOCK_RECORD "media-record"

typedef void (*GSRForeachFunction) (GSRWindow *window,
				    gpointer closure);

#endif
