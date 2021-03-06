#
# This script was written by Renaud Deraison <deraison@cvs.nessus.org>
#
# Script audit and contributions from Carmichael Security <http://www.carmichaelsecurity.com>
#      Erik Anderson <eanders@carmichaelsecurity.com>
#      Added links to the Bugtraq message archive and Microsoft Knowledgebase
#
# See the Nessus Scripts License for details
#

if(description)
{
 script_id(10844);
 script_version ("$Revision: 1.9 $");
 name["english"] = "ASP.NET Cross Site Scripting";
 script_cve_id("CAN-2003-0223");
 script_name(english:name["english"]);

 desc["english"] = "
ASP.NET is vulnerable to a cross site scripting vulnerability.

Solution : There was no solution ready when this vulnerability was written;
Please contact the vendor for updates that address this vulnerability.

Reference : http://online.securityfocus.com/archive/1/254001
Reference : http://msdn.microsoft.com/library/en-us/dncode/html/secure07152002.asp

Risk factor : Medium";

 script_description(english:desc["english"]);

 summary["english"] = "Tests for ASP.NET CSS";

 script_summary(english:summary["english"]);

 script_category(ACT_GATHER_INFO);

 script_copyright(english:"This script is Copyright (C) 2002 Renaud Deraison",
		francais:"Ce script est Copyright (C) 2002 Renaud Deraison");
 family["english"] = "CGI abuses";
 family["francais"] = "Abus de CGI";
 script_family(english:family["english"], francais:family["francais"]);
 script_dependencie("find_service.nes", "no404.nasl", "http_version.nasl", "cross_site_scripting.nasl");
 script_require_ports("Services/www", 80);
 script_require_keys("www/iis");
 exit(0);
}


include("http_func.inc");
include("http_keepalive.inc");

port = get_kb_item("Services/www");
if(!port)port = 80;
if(!get_port_state(port)) exit(0);
if(get_kb_item(string("www/", port, "/generic_xss"))) exit(0);

str = "/~/<script>alert(document.cookie)</script>.aspx?aspxerrorpath=null";
req= http_get(item:str, port:port);
r = http_keepalive_send_recv(port:port, data:req, bodyonly:1);
if( r == NULL ) exit(0);
lookfor = "<script>alert(document.cookie)</script>";
if(lookfor >< r)
{
   security_warning(port);
}
