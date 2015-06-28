#!/usr/local/bin/swipl

/*
 *  This FCGI example involves multiple threads accepting requests
 *
 *  Copyright (C) 2015 Keri Harris <keri@gentoo.org>
 */

:-use_module(library(fcgi)).

service_requests(ID) :-
	forall(fcgi_accept,
	       service_request(ID)).

service_request(ID) :-
	format(atom(Flag), 'count__~w', [ID]),
	flag(Flag, N, N+1),

	format('Content-type: text/html\n\n', []),
	format('<title>Multi-threaded FCGI</title>\n', []),
	format('<h1>Multi-threaded FCGI</h1>\n', []),
	format('<p>Requests handled:</p>\n', []),
	forall(between(0, 9, X),
               ( format(atom(ThreadFlag), 'count__~w', [X]),
	         flag(ThreadFlag, TN, TN),
                 format('<p>Thread ~w: ~w</p>\n', [X, TN])
	       )),

	fcgi_finish.

main :-
	forall(between(1, 9, X),
               thread_create(service_requests(X), _, [detached(true)])),
	service_requests(0).

:- main.

