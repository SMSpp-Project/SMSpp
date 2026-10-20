#!/usr/bin/env python3
###############################################################################
#                       smspp_module_exports.py                               #
#                                                                             #
# Writes src/SMSpp.cppm, the interface of the C++20 module "smspp": it reads  #
# the AST of all the headers in include/ (dumped by clang) and exports, with  #
# a using-declaration, every name that they declare at namespace scope.       #
# To be run whenever a header gains or loses a name:                          #
#                                                                             #
#   python3 cmake/smspp_module_exports.py --clang clang++ -- -Ibuild \        #
#           -I/usr/include/eigen3 -I<fastflow>                                #
#                                                                             #
# where the arguments after "--" are the include paths that the headers       #
# need besides include/ (the build directory holds SMS++Config.h).            #
###############################################################################

import argparse
import collections
import json
import os
import subprocess
import sys
import tempfile

# the kinds of declaration that a using-declaration can export; the others
# (e.g., the out-of-class definitions of members) come with their class
EXPORTED = { "CXXRecordDecl", "ClassTemplateDecl", "EnumDecl",
             "FunctionDecl", "FunctionTemplateDecl", "TypedefDecl",
             "TypeAliasDecl", "TypeAliasTemplateDecl", "VarDecl",
             "VarTemplateDecl" }


def dump_ast( clang , root , flags ):
    """Returns the JSON AST of a translation unit that includes every header
    of root/include."""
    inc = os.path.join( root , "include" )
    headers = sorted( h for h in os.listdir( inc ) if h.endswith( ".h" ) )
    with tempfile.NamedTemporaryFile( "w" , suffix = ".cpp" ,
                                      delete = False ) as tu:
        for h in headers:
            tu.write( f'#include "{h}"\n' )
    try:
        out = subprocess.run( [ clang , "-std=c++20" , "-fsyntax-only" ,
                                "-I" + inc , *flags , "-Xclang" ,
                                "-ast-dump=json" , tu.name ] ,
                              check = True , capture_output = True )
    finally:
        os.unlink( tu.name )
    return json.loads( out.stdout )


def collect( ast , inc ):
    """Returns { namespace : { name : header } } for the names declared at
    namespace scope in include/, each name with the first header that
    declares it, and the list of those that cannot be exported."""
    names = collections.defaultdict( dict )
    internal = []
    state = { "file" : "" }

    def file_of( node ):
        for key in ( "loc" , "range" ):
            loc = node.get( key , {} )
            if key == "range":
                loc = loc.get( "begin" , {} )
            loc = loc.get( "expansionLoc" , loc )
            if "file" in loc:
                state[ "file" ] = loc[ "file" ]
                return

    def walk( nodes , ns ):
        for node in nodes:
            file_of( node )
            kind = node.get( "kind" )
            if kind == "NamespaceDecl":
                walk( node.get( "inner" , [] ) ,
                      ns + [ node.get( "name" , "" ) ] )
                continue
            if kind == "LinkageSpecDecl":
                walk( node.get( "inner" , [] ) , ns )
                continue
            path = state[ "file" ]
            if ( not path.startswith( inc + os.sep ) ) or \
               ( kind not in EXPORTED ) or \
               node.get( "isImplicit" ) or ( not node.get( "name" ) ):
                continue
            if ( not ns ) or ( "" in ns ):
                continue  # global or anonymous namespace: not ours to export
            header = os.path.relpath( path , inc )
            key = "::".join( ns )
            # a function template keeps its storage class in its pattern,
            # and if that is a method it is the out-of-class definition of a
            # member, which comes with its class
            decl = node
            if kind == "FunctionTemplateDecl":
                decl = next( ( n for n in node.get( "inner" , [] )
                               if n.get( "kind" , "" ).endswith( "Decl" ) and
                                  n.get( "kind" ) != "TemplateTypeParmDecl" and
                                  n.get( "kind" ) !=
                                  "NonTypeTemplateParmDecl" and
                                  n.get( "kind" ) !=
                                  "TemplateTemplateParmDecl" ) , {} )
                if decl.get( "kind" ) != "FunctionDecl":
                    continue
            if decl.get( "storageClass" ) == "static":
                internal.append( f"{key}::{node[ 'name' ]} ({header})" )
                continue
            # a name goes under the header that defines it, not under the
            # first that declares it in advance
            record = node
            if kind == "ClassTemplateDecl":
                record = next( ( n for n in node.get( "inner" , [] )
                                 if n.get( "kind" ) == "CXXRecordDecl" ) , {} )
            if record.get( "completeDefinition" ):
                names[ key ][ node[ "name" ] ] = header
            else:
                names[ key ].setdefault( node[ "name" ] , header )
            # the enumerators of an unscoped enum live in the namespace
            if ( kind == "EnumDecl" ) and not node.get( "scopedEnumTag" ):
                for e in node.get( "inner" , [] ):
                    if e.get( "kind" ) == "EnumConstantDecl":
                        names[ key ].setdefault( e[ "name" ] , header )

    walk( ast.get( "inner" , [] ) , [] )
    return names , internal


BANNER = "/*" + "-" * 74 + "*/"


def title( text ):
    text = f" {text} "
    left = ( 74 - len( text ) ) // 2
    return "/*" + "-" * left + text + "-" * ( 74 - left - len( text ) ) + "*/"


def write_module( root , names ):
    inc = os.path.join( root , "include" )
    headers = sorted( h for h in os.listdir( inc ) if h.endswith( ".h" ) )
    out = [ BANNER , title( "File SMSpp.cppm" ) , BANNER ,
            "/** @file" ,
            " * Interface of the C++20 module smspp, which exports every name "
            "that the" ,
            " * headers of SMS++ declare at namespace scope; a translation "
            "unit can then" ,
            " * write" ,
            " *" ,
            " *     import smspp;" ,
            " *" ,
            " * in place of the #include of the headers. The macros (e.g., "
            "those of the" ,
            " * factory in SMSFactory.h) are not exported by a module, so a "
            "file that" ,
            " * uses them still includes the header that defines them." ,
            " *" ,
            " * This file is written by cmake/smspp_module_exports.py, which "
            "reads the" ,
            " * names from the AST of the headers: it is not to be edited by "
            "hand." ,
            " *" ,
            " * \\author Antonio Frangioni \\n" ,
            " *         Dipartimento di Informatica \\n" ,
            " *         Universita' di Pisa \\n" ,
            " *" ,
            " * \\author Donato Meoli \\n" ,
            " *         Dipartimento di Informatica \\n" ,
            " *         Universita' di Pisa \\n" ,
            " *" ,
            " * \\copyright &copy; by Antonio Frangioni, Donato Meoli" ,
            " */" ,
            BANNER , title( "GLOBAL MODULE FRAGMENT" ) , BANNER , "" ,
            "module;" , "" ]
    out += [ f'#include "{h}"' for h in headers ]
    out += [ "" , BANNER , title( "MODULE PURVIEW" ) , BANNER , "" ,
             "export module smspp;" , "" ]
    for ns in sorted( names ):
        by_header = collections.defaultdict( list )
        for name , header in names[ ns ].items():
            by_header[ header ].append( name )
        parts = ns.split( "::" )
        out.append( "export namespace " + parts[ 0 ] + " {" )
        for depth , p in enumerate( parts[ 1: ] , 1 ):
            out.append( " " * depth + "namespace " + p + " {" )
        pad = " " * len( parts )
        for header in sorted( by_header ):
            out.append( "" )
            out.append( pad + "// " + header )
            for name in sorted( by_header[ header ] ):
                out.append( f"{pad}using {ns}::{name};" )
        for depth in range( len( parts ) - 1 , -1 , -1 ):
            out.append( " " * depth + "}" )
        out.append( "" )
    out += [ BANNER , title( "End File SMSpp.cppm" ) , BANNER ]
    with open( os.path.join( root , "src" , "SMSpp.cppm" ) , "w" ) as f:
        f.write( "\n".join( out ) + "\n" )


def main():
    ap = argparse.ArgumentParser( description = __doc__ )
    ap.add_argument( "--clang" , default = "clang++" )
    ap.add_argument( "--root" ,
                     default = os.path.dirname( os.path.dirname(
                         os.path.abspath( __file__ ) ) ) )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()
    inc = os.path.join( os.path.abspath( args.root ) , "include" )
    names , internal = collect( dump_ast( args.clang , args.root ,
                                          args.flags ) , inc )
    if internal:
        sys.exit( "names with internal linkage, which a module cannot "
                  "export:\n  " + "\n  ".join( internal ) )
    write_module( args.root , names )


if __name__ == "__main__":
    main()

###############################################################################
#                     End File smspp_module_exports.py                        #
###############################################################################
