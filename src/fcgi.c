/*  This file is part of pl-fcgi.

    Copyright (C) 2015 Keri Harris <keri@gentoo.org>

    pl-fcgi is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 2.1
    of the License, or (at your option) any later version.

    pl-fcgi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pl-fcgi.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <fcgiapp.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


#define FCGI_SUCCESS 0

#define DEBUG 1

#define FCGI_debug_helper(fmt, ...) \
        { char buf[256]; \
          time_t t = time(NULL); \
          struct tm *tmp = localtime(&t); \
          strftime(buf, 256, "%c", tmp); \
          fprintf(stderr, "[%s] [pl-fcgi] [DEBUG] [PL PID: %d] " \
                  fmt "%s\n", buf, getpid(), __VA_ARGS__); \
        }

#define FCGI_debug(...) \
        do { if (DEBUG) \
             { FCGI_debug_helper(__VA_ARGS__, ""); \
             } \
           } while (0)



foreign_t fcgi_accept(control_t h);
foreign_t fcgi_finish(void);
foreign_t fcgi_param(term_t name, term_t value, control_t h);


int
fcgi_raise_error(int status, char *message)
{
  term_t except = PL_new_term_ref();
  if ( !PL_unify_term(except,
                      PL_FUNCTOR_CHARS, "error", 2,
                        PL_FUNCTOR_CHARS, "fcgi_error", 2,
                          PL_INT, status,
                          PL_UTF8_CHARS, message,
                        PL_VARIABLE) )
  { return FALSE;
  }
  return PL_raise_exception(except);
}


int
fcgi_raise_error__va(int status, char *fmt, ...)
{
  char message[1024];

  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  return fcgi_raise_error(status, message);
}


typedef struct fcgi_context
{
  FCGX_Request *req;
  IOSTREAM *pl_in, *pl_out, *pl_err;
  void *h_in, *h_out, *h_err;
  IOFUNCTIONS *fn_in, *fn_out, *fn_err;
  FCGX_Stream *fcgi_in, *fcgi_out, *fcgi_err;
  FCGX_ParamArray env;
} fcgi_context;


static ssize_t
fcgi_stream_read(void *handle, char *buf, size_t size)
{
  FCGX_Stream *s;
  ssize_t bytes_read;

  s = handle;
  bytes_read = FCGX_GetStr(buf, size, s);

  return bytes_read;
}


static ssize_t
fcgi_stream_write(void *handle, char *buf, size_t size)
{
  FCGX_Stream *s;
  ssize_t bytes_written;

  s = handle;
  bytes_written = FCGX_PutStr(buf, size, s);

  return bytes_written;
}


static IOFUNCTIONS fcgi_functions =
{ fcgi_stream_read,
  fcgi_stream_write,
  NULL,	             /* seek */
  NULL,              /* close */
  NULL,              /* control */
  NULL,	             /* seek64 */
};


static pthread_key_t key;

void
fcgi_init(void)
{
  pthread_key_create(&key, NULL);
  FCGX_Init();
}


foreign_t
fcgi_accept(control_t h)
{
  FCGX_Request *req;
  fd_set fds;
  fcgi_context *ctxt;
  int status;

  FCGI_debug("fcgi_accept()");

  if ( FCGX_IsCGI() )
  { return TRUE;
  }

  ctxt = pthread_getspecific(key);

  if ( !ctxt )
  { ctxt = malloc(sizeof(*ctxt));
    memset(ctxt, 0, sizeof(*ctxt));
  }

  if ( ctxt->req )
  { fcgi_finish();
  }

  req = malloc(sizeof(*req));
  memset(req, 0, sizeof(*req));

  status = FCGX_InitRequest(req, 0, 0);
  if ( status != FCGI_SUCCESS )
  { return fcgi_raise_error(status, "FCGX_InitRequest() failed");
  }

  FD_ZERO(&fds);
  FD_SET(req->listen_sock, &fds);
  if ( select(req->listen_sock+1, &fds, NULL, NULL, NULL) < 1 )
  { return FALSE;
  }

  status = FCGX_Accept_r(req);
  if ( status != FCGI_SUCCESS )
  { return fcgi_raise_error(status, "FCGX_Accept_r() failed");
  }

  FCGI_debug("REMOTE_ADDR: %s, REQUEST_METHOD: %s, REQUEST_URI: %s",
             FCGX_GetParam("REMOTE_ADDR", req->envp),
             FCGX_GetParam("REQUEST_METHOD", req->envp),
             FCGX_GetParam("REQUEST_URI", req->envp));

  if ( !ctxt )
  { ctxt = malloc(sizeof(*ctxt));
    memset(ctxt, 0, sizeof(*ctxt));
  }

  ctxt->req = req;

  ctxt->pl_in = Suser_input;
  ctxt->h_in = Suser_input->handle;
  ctxt->fn_in = Suser_input->functions;
  ctxt->fcgi_in = req->in;

  ctxt->pl_out = Suser_output;
  ctxt->h_out = Suser_output->handle;
  ctxt->fn_out = Suser_output->functions;
  ctxt->fcgi_out = req->out;

  ctxt->pl_err = Suser_error;
  ctxt->h_err = Suser_error->handle;
  ctxt->fn_err = Suser_error->functions;
  ctxt->fcgi_err = req->err;

  ctxt->env = req->envp;

  pthread_setspecific(key, ctxt);

  Suser_input->handle = req->in;
  Suser_input->functions = &fcgi_functions;

  Suser_output->handle = req->out;
  Suser_output->functions = &fcgi_functions;

  Suser_error->handle = req->err;
  Suser_error->functions = &fcgi_functions;

  PL_retry(0);
}


foreign_t
fcgi_finish(void)
{
  fcgi_context *ctxt;

  FCGI_debug("fcgi_finish()");

  if ( FCGX_IsCGI() )
  { return TRUE;
  }

  ctxt = pthread_getspecific(key);

  Sflush(Suser_output);
  Sflush(Suser_error);
  FCGX_Finish_r(ctxt->req);

  ctxt->req = NULL;
  ctxt->fcgi_in = ctxt->fcgi_out = ctxt->fcgi_err = NULL;
  ctxt->env = NULL;

  Suser_input->handle = ctxt->h_in;
  Suser_input->functions = ctxt->fn_in;

  Suser_output->handle = ctxt->h_out;
  Suser_output->functions = ctxt->fn_out;

  Suser_error->handle = ctxt->h_err;
  Suser_error->functions = ctxt->fn_err;

  return TRUE;
}


foreign_t
fcgi_is_cgi(void)
{
  return FCGX_IsCGI();
}


extern char **environ;

foreign_t
fcgi_param(term_t name, term_t value, control_t h)
{
  fcgi_context *ctxt;
  char **env, **cgi_environ;
  char *s, *v, *sep;

  ctxt = pthread_getspecific(key);

  if ( FCGX_IsCGI() )
  { cgi_environ = environ;
  }
  else
  { cgi_environ = ctxt->env;
  }

  if ( !PL_is_variable(name) )
  {
    if ( !PL_get_atom_chars(name, &s) )
    { return PL_type_error("atom", name);
    }

    v = FCGX_GetParam(s, cgi_environ);
    if ( !v )
    { return FALSE;
    }

    return PL_unify_chars(value, PL_ATOM|REP_UTF8, -1, v);
  }

  switch ( PL_foreign_control(h) )
  {
    case PL_FIRST_CALL:
    { env = cgi_environ;
      break;
    }
    case PL_REDO:
    { env = PL_foreign_context_address(h);
      break;
    }
    case PL_PRUNED:
    default:
    { return TRUE;
    }
  }

  for ( ; *env; env++ )
  {
    s = strdup(*env);
    sep = index(s, '=');
    sep[0] = '\0';

    if ( !PL_unify_chars(name, PL_ATOM|REP_UTF8, -1, s) )
    { free(s);
      return FALSE;
    }
    if ( !PL_unify_chars(value, PL_ATOM|REP_UTF8, -1, sep+1) )
    { free(s);
      return FALSE;
    }

    free(s);
    PL_retry_address(env+1);
  }

  return FALSE;
}


install_t
install_fcgi()
{
  fcgi_init();

  PL_register_foreign("fcgi_accept", 0, fcgi_accept, PL_FA_NONDETERMINISTIC);
  PL_register_foreign("fcgi_finish", 0, fcgi_finish, 0);
  PL_register_foreign("fcgi_is_cgi", 0, fcgi_is_cgi, 0);
  PL_register_foreign("fcgi_param", 2, fcgi_param, PL_FA_NONDETERMINISTIC);
}
