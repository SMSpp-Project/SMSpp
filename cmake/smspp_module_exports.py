#!/usr/bin/env python3
###############################################################################
#                       smspp_module_exports.py                               #
#                                                                             #
# Writes the interface of the C++20 module of an SMS++ module: it reads the   #
# AST of the headers in <root>/include (dumped by clang) and exports, with a  #
# using-declaration, every name that they declare at namespace scope. The     #
# core is the module "smspp", whose interface is src/SMSpp.cppm; any other    #
# module also re-exports the modules it depends on ("export import"), e.g.:   #
#                                                                             #
#   python3 cmake/smspp_module_exports.py --clang clang++ -- -Ibuild \        #
#           -I/usr/include/eigen3 -I<fastflow>                                #
#                                                                             #
#   python3 <SMS++>/cmake/smspp_module_exports.py --root UCBlock \            #
#           --name smspp.ucblock --out src/UCBlock.cppm \                     #
#           --imports smspp -- <the flags of a source of UCBlock>             #
#                                                                             #
# A header that is compiled only with an optional library is given with       #
# --optional <header>=<macro>: both its #include and its names are then       #
# inside an #if defined( <macro> ), which CMake defines when the library is   #
# there. To be run whenever a header gains or loses a name.                   #
###############################################################################

import argparse
import collections
import fnmatch
import json
import os
import subprocess
import sys
import tempfile
import textwrap

# the kinds of declaration that a using-declaration can export; the others
# (e.g., the out-of-class definitions of members) come with their class
EXPORTED = { "CXXRecordDecl", "ClassTemplateDecl", "EnumDecl",
             "FunctionDecl", "FunctionTemplateDecl", "TypedefDecl",
             "TypeAliasDecl", "TypeAliasTemplateDecl", "VarDecl",
             "VarTemplateDecl" }


def dump_ast( clang , root , headers , flags ):
    """Returns the JSON AST of a translation unit that includes the given
    headers of root/include."""
    inc = os.path.join( root , "include" )
    with tempfile.NamedTemporaryFile( "w" , suffix = ".cpp" ,
                                      delete = False ) as tu:
        for h in headers:
            tu.write( f'#include "{h}"\n' )
    try:
        out = subprocess.run( [ clang , "-std=c++20" , "-fsyntax-only" ,
                                "-I" + inc , *flags , "-Xclang" ,
                                "-ast-dump=json" , tu.name ] ,
                              capture_output = True )
    finally:
        os.unlink( tu.name )
    if out.returncode:
        errors = [ l for l in out.stderr.decode().splitlines()
                   if "error" in l ]
        sys.exit( "the headers do not compile together:\n  " +
                  "\n  ".join( errors[ :10 ] ) )
    return json.loads( out.stdout )


def collect( ast , inc ):
    """Returns { namespace : { name : header } } for the names declared at
    namespace scope in include/, each name with the first header that
    declares it, and the list of those that cannot be exported."""
    names = collections.defaultdict( dict )
    internal = []
    # the classes declared here, and those defined here: one that is only
    # declared in advance belongs to the module that defines it
    declared , defined = set() , set()
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
            if kind in ( "CXXRecordDecl" , "ClassTemplateDecl" ):
                declared.add( ( key , node[ "name" ] ) )
            if record.get( "completeDefinition" ):
                defined.add( ( key , node[ "name" ] ) )
                names[ key ][ node[ "name" ] ] = header
            else:
                names[ key ].setdefault( node[ "name" ] , header )
            # the enumerators of an unscoped enum live in the namespace
            if ( kind == "EnumDecl" ) and not node.get( "scopedEnumTag" ):
                for e in node.get( "inner" , [] ):
                    if e.get( "kind" ) == "EnumConstantDecl":
                        names[ key ].setdefault( e[ "name" ] , header )

    walk( ast.get( "inner" , [] ) , [] )
    for key , name in declared - defined:
        names[ key ].pop( name , None )
    return names , internal


BANNER = "/*" + "-" * 74 + "*/"


def title( text ):
    text = f" {text} "
    left = ( 74 - len( text ) ) // 2
    return "/*" + "-" * left + text + "-" * ( 74 - left - len( text ) ) + "*/"


def authors_of( header ):
    """Returns the lines of the \\author and \\copyright of the @file comment
    of a header, which the interface of its module repeats."""
    lines = open( header ).read().split( "\n" )
    first = next( ( i for i , l in enumerate( lines ) if "\\author" in l ) ,
                  None )
    last = next( ( i for i , l in enumerate( lines ) if "\\copyright" in l ) ,
                 None )
    if ( first is None ) or ( last is None ) or ( last < first ):
        return []
    # the copyright may go on in the lines after it
    while ( last + 1 < len( lines ) ) and \
          lines[ last + 1 ].strip() not in ( "*" , "*/" , "" ) and \
          "\\" not in lines[ last + 1 ]:
        last += 1
    return [ " *" ] + [ l.rstrip() for l in lines[ first : last + 1 ] ]


def doc_lines( name , what , script ):
    """Returns the lines of the @file comment that say what the module is,
    wrapped at 79 columns."""
    def para( text ):
        return [ " * " + l for l in textwrap.wrap( text , 75 ,
                                                     break_on_hyphens = False ) ]
    deps = "" if name == "smspp" else \
           ", together with the modules it depends on"
    out = para( f"Interface of the C++20 module {name}, which exports every "
                f"name that the headers of {what} declare at namespace "
                f"scope{deps}; a translation unit can then write" )
    out += [ " *" , f" *     import {name};" , " *" ]
    out += para( "in place of the #include of the headers. The macros (e.g., "
                 "those of the factory in SMSFactory.h) are not exported by "
                 "a module, so a file that uses them still includes the "
                 "header that defines them." )
    out += [ " *" ]
    out += para( f"This file is written by {script}, which reads the names "
                 "from the AST of the headers: it is not to be edited by "
                 "hand." )
    return out


def write_module( root , name , out_file , imports , optional , headers ,
                  names , authors , what , excluded ):
    """Writes the interface of the module name to root/out_file."""
    base = os.path.basename( out_file )
    script = "cmake/smspp_module_exports.py" if name == "smspp" else \
             "SMS++/cmake/smspp_module_exports.py"
    out = [ BANNER , title( f"File {base}" ) , BANNER , "/** @file" ]
    out += doc_lines( name , what , script )
    if excluded:
        text = ( "The headers " + ", ".join( excluded ) + " are not in this "
                 "module: they need a library of their own, or only a "
                 "source includes them." )
        out += [ " *" ] + [ " * " + l for l in textwrap.wrap( text , 75 ) ]
    out += authors
    out += [ " */" ,
             BANNER , title( "GLOBAL MODULE FRAGMENT" ) , BANNER , "" ,
             "module;" , "" ]
    for h in headers:
        if h in optional:
            out += [ f"#if defined( {optional[ h ]} )" , f' #include "{h}"' ,
                     "#endif" ]
        else:
            out.append( f'#include "{h}"' )
    out += [ "" , BANNER , title( "MODULE PURVIEW" ) , BANNER , "" ,
             f"export module {name};" , "" ]
    if imports:
        out += [ f"export import {m};" for m in imports ] + [ "" ]
    for ns in sorted( names ):
        by_header = collections.defaultdict( list )
        for n , header in names[ ns ].items():
            by_header[ header ].append( n )
        parts = ns.split( "::" )
        out.append( "export namespace " + parts[ 0 ] + " {" )
        for depth , p in enumerate( parts[ 1: ] , 1 ):
            out.append( " " * depth + "namespace " + p + " {" )
        pad = " " * len( parts )
        for header in sorted( by_header ):
            out.append( "" )
            out.append( pad + "// " + header )
            if header in optional:
                out.append( f"#if defined( {optional[ header ]} )" )
            for n in sorted( by_header[ header ] ):
                out.append( f"{pad}using {ns}::{n};" )
            if header in optional:
                out.append( "#endif" )
        for depth in range( len( parts ) - 1 , -1 , -1 ):
            out.append( " " * depth + "}" )
        out.append( "" )
    out += [ BANNER , title( f"End File {base}" ) , BANNER ]
    with open( os.path.join( root , out_file ) , "w" ) as f:
        f.write( "\n".join( out ) + "\n" )


def main():
    ap = argparse.ArgumentParser(
        description = "Writes the interface of the C++20 module of an SMS++ "
                      "module." )
    ap.add_argument( "--clang" , default = "clang++" )
    ap.add_argument( "--root" ,
                     default = os.path.dirname( os.path.dirname(
                         os.path.abspath( __file__ ) ) ) ,
                     help = "the directory of the module, with include/" )
    ap.add_argument( "--name" , default = "smspp" )
    ap.add_argument( "--out" , default = "src/SMSpp.cppm" ,
                     help = "the interface, relative to --root" )
    ap.add_argument( "--imports" , nargs = "*" , default = [] ,
                     help = "the modules to re-export" )
    ap.add_argument( "--optional" , nargs = "*" , default = [] ,
                     metavar = "HEADER=MACRO" )
    ap.add_argument( "--exclude" , nargs = "*" , default = [] ,
                     metavar = "HEADER" ,
                     help = "headers (or patterns, e.g., *_maps.h) left out: "
                            "those of another library (e.g., the one that "
                            "needs Torch) and those that only a source "
                            "includes" )
    ap.add_argument( "--authors" ,
                     help = "the header whose @file comment gives the "
                            "authors (default: the one named as the module "
                            "directory)" )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()

    root = os.path.abspath( args.root )
    inc = os.path.join( root , "include" )
    excluded = sorted( h for h in os.listdir( inc ) if h.endswith( ".h" ) and
                       any( fnmatch.fnmatch( h , e ) for e in args.exclude ) )
    headers = sorted( h for h in os.listdir( inc )
                      if h.endswith( ".h" ) and ( h not in excluded ) )
    optional = dict( o.split( "=" , 1 ) for o in args.optional )
    # an optional header whose macro is not among the flags is not there
    defined = { f[ 2: ].split( "=" )[ 0 ] for f in args.flags
                if f.startswith( "-D" ) }
    present = [ h for h in headers
                if ( h not in optional ) or ( optional[ h ] in defined ) ]
    names , internal = collect( dump_ast( args.clang , root , present ,
                                          args.flags ) , inc )
    if internal:
        sys.exit( "names with internal linkage, which a module cannot "
                  "export:\n  " + "\n  ".join( internal ) )

    what = "SMS++" if args.name == "smspp" else os.path.basename( root )
    if args.authors:
        authors = authors_of( os.path.join( inc , args.authors ) )
    elif args.name == "smspp":
        authors = authors_of( os.path.join( inc , "Block.h" ) )
    else:
        # the header named as the module, or else the first that has them
        authors = []
        for h in [ what + ".h" ] + headers:
            if os.path.exists( os.path.join( inc , h ) ):
                authors = authors_of( os.path.join( inc , h ) )
                if authors:
                    break
    write_module( root , args.name , args.out , args.imports , optional ,
                  headers , names , authors , what , sorted( args.exclude ) )


if __name__ == "__main__":
    main()

###############################################################################
#                     End File smspp_module_exports.py                        #
###############################################################################
