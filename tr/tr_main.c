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
#include <jansson.h>

#include <tr.h>
#include <trust_router/tid.h>
#include <tr_config.h>
#include <tr_comm.h>
#include <tr_idp.h>

/* Structure to hold TR instance and original request in one cookie */
typedef struct tr_resp_cookie {
  TR_INSTANCE *tr;
  TID_REQ *orig_req;
} TR_RESP_COOKIE;

static void tr_tidc_resp_handler (TIDC_INSTANCE *tidc, 
			TID_REQ *req,
			TID_RESP *resp, 
			void *resp_cookie) 
{
  fprintf(stderr, "tr_tidc_resp_handler: Response received (conn = %d)! Realm = %s, Community = %s.\n", ((TR_RESP_COOKIE *)resp_cookie)->orig_req->conn, resp->realm->buf, resp->comm->buf);
  req->resp_rcvd = 1;

  /* TBD -- handle concatentation of multiple responses to single req */
  tids_send_response(((TR_RESP_COOKIE *)resp_cookie)->tr->tids, 
		     ((TR_RESP_COOKIE *)resp_cookie)->orig_req, 
		     resp);
  
  return;
}

static int tr_tids_req_handler (TIDS_INSTANCE * tids,
		      TID_REQ *orig_req, 
		      TID_RESP **resp,
		      void *tr)
{
  TIDC_INSTANCE *tidc = NULL;
  TR_RESP_COOKIE resp_cookie;
  TR_AAA_SERVER *aaa_servers = NULL;
  TR_NAME *apc = NULL;
  TID_REQ *fwd_req = NULL;
  TR_COMM *cfg_comm = NULL;
  int rc;

  if ((!tids) || (!orig_req) || (!resp) || (!(*resp))) {
    fprintf(stderr, "tids_req_handler: Bad parameters\n");
    return -1;
  }

  fprintf(stdout, "tr_tids_req_handler: Request received (conn = %d)! Realm = %s, Comm = %s\n", orig_req->conn, 
	 orig_req->realm->buf, orig_req->comm->buf);
  if (tids)
    tids->req_count++;

  /* Duplicate the request, so we can modify and forward it */
  if (NULL == (fwd_req = tid_dup_req(orig_req))) {
    fprintf(stderr, "tr_tids_req_handler: Unable to duplicate request.\n");
    return -1;
  }

  /* Map the comm in the request from a COI to an APC, if needed */
  if (NULL == (cfg_comm = tr_comm_lookup((TR_INSTANCE *)tids->cookie, orig_req->comm))) {
    fprintf(stderr, "tr_tids_req_hander: Request for unknown comm: %s.\n", orig_req->comm->buf);
    tids_send_err_response(tids, orig_req, "Unknown community");
    return -1;
  }

  /* TBD -- check that the rp_realm is a member of the original community */

  /* If the community is a COI, switch to the apc */
  if (TR_COMM_COI == cfg_comm->type) {
    fprintf(stderr, "tr_tids_req_handler: Community was a COI, switching.\n");
    /* TBD -- In theory there can be more than one?  How would that work? */
    if ((!cfg_comm->apcs) || (!cfg_comm->apcs->id)) {
      fprintf(stderr, "No valid APC for COI %s.\n", orig_req->comm->buf);
      tids_send_err_response(tids, orig_req, "No valid APC for community");
      return -1;
    }
    apc = tr_dup_name(cfg_comm->apcs->id);
    fwd_req->comm = apc;
    fwd_req->orig_coi = orig_req->comm;
  }

  /* Find the AAA server(s) for this request */
  if (NULL == (aaa_servers = tr_idp_aaa_server_lookup((TR_INSTANCE *)tids->cookie, 
						      orig_req->realm, 
						      orig_req->comm))) {
      fprintf(stderr, "tr_tids_req_handler: No AAA Servers for realm %s.\n", orig_req->realm->buf);
      tids_send_err_response(tids, orig_req, "No path to AAA Servers for realm");
      return -1;
    }
  /* send a TID request to the AAA server(s), and get the answer(s) */
  /* TBD -- Handle multiple servers */

  /* Create a TID client instance */
  if (NULL == (tidc = tidc_create())) {
    fprintf(stderr, "tr_tids_req_hander: Unable to allocate TIDC instance.\n");
    tids_send_err_response(tids, orig_req, "Memory allocation failure");
    return -1;
  }

  /* Use the DH parameters from the original request */
  /* TBD -- this needs to be fixed when we handle more than one req per conn */
  tidc->client_dh = orig_req->tidc_dh;

  /* Save information about this request for the response */
  resp_cookie.tr = tr;
  resp_cookie.orig_req = orig_req;

  /* Set-up TID connection */
  /* TBD -- handle IPv6 Addresses */
  if (-1 == (fwd_req->conn = tidc_open_connection(tidc, 
					      inet_ntoa(aaa_servers->aaa_server_addr), 
					      &(fwd_req->gssctx)))) {
    fprintf(stderr, "tr_tids_req_handler: Error in tidc_open_connection.\n");
    tids_send_err_response(tids, orig_req, "Can't open connection to next hop TIDS");
    return -1;
  };

  /* Send a TID request */
  if (0 > (rc = tidc_fwd_request(tidc, fwd_req, &tr_tidc_resp_handler, (void *)&resp_cookie))) {
    fprintf(stderr, "Error from tidc_fwd_request, rc = %d.\n", rc);
    tids_send_err_response(tids, orig_req, "Can't forward request to next hop TIDS");
    return -1;
  }
    
  return 0;
}

int main (int argc, const char *argv[])
{
  TR_INSTANCE *tr = NULL;
  struct dirent **cfg_files = NULL;
  json_t *jcfg = NULL;
  TR_CFG_RC rc = TR_CFG_SUCCESS;	/* presume success */
  int err = 0, n = 0;;

  /* parse command-line arguments? -- TBD */

  /* create a Trust Router instance */
  if (NULL == (tr = tr_create())) {
    fprintf(stderr, "Unable to create Trust Router instance, exiting.\n");
    return 1;
  }

  /* find the configuration files */
  if (0 == (n = tr_find_config_files(&cfg_files))) {
    fprintf (stderr, "Can't locate configuration files, exiting.\n");
    exit(1);
  }

  /* read and parse initial configuration */
  if (NULL == (jcfg = tr_read_config (n, cfg_files))) {
    fprintf (stderr, "Error reading or parsing configuration files, exiting.\n");
    exit(1);
  }
  if (TR_CFG_SUCCESS != tr_parse_config(tr, jcfg)) {
    fprintf (stderr, "Error decoding configuration information, exiting.\n");
    exit(1);
  }

  /* apply initial configuration */
  if (TR_CFG_SUCCESS != (rc = tr_apply_new_config(tr))) {
    fprintf (stderr, "Error applying configuration, rc = %d.\n", rc);
    exit(1);
  }

  /* initialize the trust path query server instance */
  if (0 == (tr->tids = tids_create ())) {
    fprintf (stderr, "Error initializing Trust Path Query Server instance.\n");
    exit(1);
  }

  /* start the trust path query server, won't return unless fatal error. */
  if (0 != (err = tids_start(tr->tids, &tr_tids_req_handler, (void *)tr))) {
    fprintf (stderr, "Error from Trust Path Query Server, err = %d.\n", err);
    exit(err);
  }

  tids_destroy(tr->tids);
  tr_destroy(tr);
  exit(0);
}
