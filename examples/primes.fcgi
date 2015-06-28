#!/usr/local/bin/swipl

/*
 *  This FCGI example contains a costly initialization section
 *
 *  Copyright (C) 2015 Keri Harris <keri@gentoo.org>
 */

:-use_module(library(fcgi)).



/* Initialization */

:- dynamic
	composite/1,
	nth_prime/2.

sieve(N) :-
	retractall(composite(_)),
	retractall(nth_prime(_, _)),
	assertz(nth_prime(1, 2)),
	sieve_1(3, 2, N),
	retractall(composite(_)).
 
sieve_1(I0, J0, N) :-
	I0 =< N, !,
	I1 is I0+2,
	(   composite(I0)
	->  sieve_1(I1, J0, N)
	;   IS is I0*I0,
	    (   IS =< N
	    ->  DI is 2*I0,
	        add_composites(IS, N, DI)
	    ;   true 
	    ),
	    assertz(nth_prime(J0, I0)),
	    J1 is J0+1,
            sieve_1(I1, J1, N)
	).

sieve_1(_, _, _).


 
add_composites(I0, N, D) :-
	I0 =< N, !,
	(   composite(I0)
	->  true
	;   assert(composite(I0))
	),
	I1 is I0+D,
	add_composites(I1, N, D).

add_composites(_, _, _).

:- sieve(1000000).



/* Request Loop */

n_from_input(N) :-
	fcgi_param('CONTENT_LENGTH', ContentLengthAtom),
	atom_number(ContentLengthAtom, ContentLength),
	ContentLength > 0,
	read_stream_to_codes(user_input, Codes),
	atom_codes(InputAtom, Codes),
	atomic_list_concat(['n', NthAtom], '=', InputAtom),
	atom_number(NthAtom, N).

service_requests :-
	forall(fcgi_accept,
	       service_request).

service_request :-
	format('Content-type: text/html\n\n', []),
	format('<title>Nth Prime</title>\n', []),
	format('<h1>Nth Prime</h1>\n', []),

	(   n_from_input(N)
	->  (   nth_prime(N, Prime)
	    ->  format('N: ~w, Prime Number: ~w\n', [N, Prime])
	    ;   format('Input must be between 1 and 78498, received: ~w\n', [N])
	    )
	;   format('ERROR: No integer data read from standard input\n', [])
	),

	fcgi_finish.

:- service_requests.
