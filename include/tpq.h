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

#ifndef TPQ_H
#define TPQ_H

#define TRUST_PATH_QUERY_PORT	12309

typedef void (*TPQC_RESP_FUNC)(TPQC_INSTANCE *, TPQC_REQ *, TPQC_RESP *, void *);

struct tpqc_req_t {
  struct tpqc_req_t next_req;
  char realm[1024];
  char coi[1024];
  int conn;
  TPQC_RESP_FUNC *func
};

typedef struct tpqc_req_t TPQC_REQ;

struct tpqc_instance_t {
  TPQC_REQ *req_list
};

typedef void TPQC_RESP;

typedef struct tpqc_instance_t TPQC_INSTANCE;

TPQC_INSTANCE *tpqc_create (void);
void tpqc_release (TPQC_INSTANCE *tpqc);
int tpqc_open_connection (TPQC_INSTANCE *tpqc, char *server);
TPQC_REQ *tpqc_send_request (TPQC_INSTANCE *tpqc, int conn, char *realm, char *coi, TPQC_RESP_FUNC *resp_handler);

typedef void TPQS_INSTANCE;

TPQS_INSTANCE *tpqs_init ();
int tpqs_start (TPQS_INSTANCE *tpqs);


#endif
