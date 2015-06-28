#!/usr/local/bin/swipl

/*
 *  This FCGI example outputs the FastCGI environment
 *
 *  Copyright (C) 2015 Keri Harris <keri@gentoo.org>
 */

:-use_module(library(fcgi)).

service_requests :-
	forall(fcgi_accept,
	       service_request).

service_request :-
	flag(count, N, N+1),
	current_prolog_flag(pid, PID),

	format('Content-type: text/html\n\n', []),
	format('<title>FCGI environment</title>\n', []),
	format('<h1>FCGI environment</h1>\n', []),
	format('Request number: ~w, Process ID: ~w\n', [N, PID]),

	forall(fcgi_param(Name, Value),
	       format('<p>~w=~w</p>\n', [Name, Value])),

	fcgi_param('CONTENT_LENGTH', ContentLengthAtom),
	(   atom_number(ContentLengthAtom, ContentLength)
	->  true
	;   ContentLength = 0
	),

	(   ContentLength > 0
	->  read_stream_to_codes(user_input, Codes),
	    atom_codes(Atom, Codes),
	    format('stdin: ~w\n', [Atom])
	;   format('No data from standard input\n', [])
	),

	fcgi_finish.

:- service_requests.
