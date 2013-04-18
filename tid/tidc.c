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

#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include <gsscon.h>

#include <trust_router/tr_dh.h>
#include <trust_router/tid.h>
#include <tr_msg.h>
#include <gsscon.h>

int tmp_len = 32;

TIDC_INSTANCE *tidc_create ()
{
  TIDC_INSTANCE *tidc = NULL;

  if (tidc = malloc(sizeof(TIDC_INSTANCE))) 
    memset(tidc, 0, sizeof(TIDC_INSTANCE));
  else
    return NULL;

  return tidc;
}

void tidc_destroy (TIDC_INSTANCE *tidc)
{
  if (tidc)
    free(tidc);
}

int tidc_open_connection (TIDC_INSTANCE *tidc, 
			  char *server,
			  gss_ctx_id_t *gssctx)
{
  int err = 0;
  int conn = -1;

  err = gsscon_connect(server, TID_PORT, &conn);

  if (!err)
    err = gsscon_active_authenticate(conn, NULL, "trustidentity", gssctx);

  if (!err)
    return conn;
  else
    return -1;
}

int tidc_send_request (TIDC_INSTANCE *tidc, 
		       int conn, 
		       gss_ctx_id_t gssctx,
		       char *rp_realm,
		       char *realm, 
		       char *comm,
		       TIDC_RESP_FUNC *resp_handler,
		       void *cookie)

{
  int err = 0;
  char *req_buf = NULL;
  char *resp_buf = NULL;
  size_t resp_buflen = 0;
  TR_MSG *msg = NULL;
  TID_REQ *tid_req = NULL;
  TR_MSG *resp_msg = NULL;

  /* Create and populate a TID msg structure */
  if ((!(msg = malloc(sizeof(TR_MSG)))) ||
      (!(tid_req = malloc(sizeof(TID_REQ)))))
    return -1;

  memset(tid_req, 0, sizeof(tid_req));

  msg->msg_type = TID_REQUEST;

  msg->tid_req = tid_req;

  tid_req->conn = conn;
  tid_req->gssctx = gssctx;

  /* TBD -- error handling */
  tid_req->rp_realm = tr_new_name(rp_realm);
  tid_req->realm = tr_new_name(realm);
  tid_req->comm = tr_new_name(comm);

  tid_req->tidc_dh = tidc->client_dh;

  tid_req->resp_func = resp_handler;
  tid_req->cookie = cookie;

  /* Encode the request into a json string */
  if (!(req_buf = tr_msg_encode(msg))) {
    printf("Error encoding TID request.\n");
    return -1;
  }

  printf ("Sending TID request:\n");
  printf ("%s\n", req_buf);

  /* Send the request over the connection */
  if (err = gsscon_write_encrypted_token (conn, gssctx, req_buf, 
					  strlen(req_buf))) {
    fprintf(stderr, "Error sending request over connection.\n");
    return -1;
  }

  /* TBD -- should queue request on instance, resps read in separate thread */
  /* Read the response from the connection */

  if (err = gsscon_read_encrypted_token(conn, gssctx, &resp_buf, &resp_buflen)) {
    if (resp_buf)
      free(resp_buf);
    return -1;
  }

  fprintf(stdout, "Response Received (%u bytes).\n", (unsigned) resp_buflen);
  fprintf(stdout, "%s\n", resp_buf);

  if (NULL == (resp_msg = tr_msg_decode(resp_buf, resp_buflen))) {
    fprintf(stderr, "Error decoding response.\n");
    return -1;
  }

  /* TBD -- Check if this is actually a valid response */
  if (!resp_msg->tid_resp) {
    fprintf(stderr, "Error: No response in the response!\n");
    return -1;
  }
  
  /* Call the caller's response function */
  (*tid_req->resp_func)(tidc, tid_req, resp_msg->tid_resp, cookie);


  if (msg)
    free(msg);
  if (tid_req)
    free(tid_req);
  if (req_buf)
    free(req_buf);
  if (resp_buf)
    free(resp_buf);

  /* TBD -- free the decoded response */

  return 0;
}





