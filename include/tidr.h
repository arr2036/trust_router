/*
 * Copyright (c) 2012, JANET(UK)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of JANET(UK) nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef TIDR_H
#define TIDR_H

#include <gsscon.h>
#include <tr_name.h>

#define TIDR_PORT	12310

typedef struct tidr_req {
  struct tidr_req *next_req;
  int conn;
  TR_NAME *realm;
  TR_NAME *coi;
  void *resp_func;
  void *cookie;
} TIDR_REQ;

typedef struct tidr_resp {
  TR_NAME *realm;
  TR_NAME *coi;
  /* Address of AAA Server */
  /* Credentials */
  /* Trust Path Used */
} TIDR_RESP;

typedef struct tidrc_instance {
  TIDR_REQ *req_list;
} TIDRC_INSTANCE;

typedef struct tidrs_instance {
  int req_count;
  void *req_handler;
  void *cookie;
} TIDRS_INSTANCE;

typedef void (TIDRC_RESP_FUNC)(TIDRC_INSTANCE *, TIDR_RESP *, void *);
typedef int (TIDRS_REQ_FUNC)(TIDRS_INSTANCE *, TIDR_REQ *, TIDR_RESP *, void *);

TIDRC_INSTANCE *tidrc_create (void);
int tidrc_open_connection (TIDRC_INSTANCE *tidrc, char *server, gss_ctx_id_t *gssctx);
int tidrc_send_request (TIDRC_INSTANCE *tidrc, int conn, gss_ctx_id_t gssctx, char *realm, char *coi, TIDRC_RESP_FUNC *resp_handler, void *cookie);
void tidrc_destroy (TIDRC_INSTANCE *tpqc);

TIDRS_INSTANCE *tidrs_create ();
int tidrs_start (TIDRS_INSTANCE *tidrs, TIDRS_REQ_FUNC *req_handler, void *cookie);
void tidrs_destroy (TIDRS_INSTANCE *tidrs);

#endif
