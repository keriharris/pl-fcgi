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

:- module(fcgi, [
                  fcgi_accept/0,
                  fcgi_finish/0,
                  fcgi_is_cgi/0,
                  fcgi_param/2
                ]).


:- use_foreign_library(foreign(fcgi), install_fcgi).


%%      fcgi_accept is nondet.
%
%       Accept a new FastCGI request. Information about the request such as
%       REMOTE_ADDR, REQUEST_METHOD, REQUEST_URI etc can be accessed via
%       fcgi_param/2.


%%      fcgi_finish is det.
%
%       Finish the request accepted by the previous call to fcgi_accept/0.
%       A call to fcgi_finish/0 is generally not required as it is automatically
%       called when backtracking into the next iteration of fcgi_accept/0.


%%      fcgi_is_cgi is det.
%
%       Succeeds iff the Prolog interpreter is running as a standalone CGI
%       process rather than a FastCGI process.


%%      fcgi_param(?Name, -Value) is nondet.
%
%       Obtain the value of a FCGI parameter for the currently active request.
