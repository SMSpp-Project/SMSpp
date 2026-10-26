#!/usr/bin/env python3
###############################################################################
#                       smspp_module_exports.py                               #
#                                                                             #
# Writes the interface of the C++20 module of an SMS++ module: it reads the   #
# AST of the headers in <root>/include (dumped by clang) and exports, with a  #
# using-declaration, every name that they declare in the namespace            #
# SMSpp_di_unipi_it, its nested ones included. The core is the module         #
# "smspp", whose interface is src/SMSpp.cppm; any other module also           #
# re-exports the modules it depends on ("export import"), e.g.:               #
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
#                                                                             #
# The global module fragment includes only the headers that no other header   #
# of the module includes, which bring in the others; the names are read from  #
# all of them anyway.                                                         #
#                                                                             #
# An interface already written says how it was made (its name, the modules    #
# it imports, its optional and excluded headers), so it can be written again  #
# from itself, with the flags with which a build compiles it, read from the   #
# compile_commands.json of a build with Clang, Ninja and SMSpp_BUILD_MODULE:  #
#                                                                             #
#   python3 cmake/smspp_module_exports.py --clang clang++ \                   #
#           --compdb build/compile_commands.json --update */src/*.cppm        #
#                                                                             #
# where the names of an optional header whose library is not in the build     #
# are kept as they are. With --check the interfaces are not written: the      #
# differences are printed, and the exit status is 1 if there are any.         #
###############################################################################

import argparse
import collections
import difflib
import fnmatch
import json
import os
import re
import shlex
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


def run_on( command , headers ):
    """Runs command on a translation unit that includes the given headers.
    The unit is alone in a directory of its own, since an #include "..." is
    looked for first in the directory of the file that has it."""
    with tempfile.TemporaryDirectory() as tmp:
        tu = os.path.join( tmp , "tu.cpp" )
        with open( tu , "w" ) as f:
            for h in headers:
                f.write( f'#include "{h}"\n' )
        return subprocess.run( command + [ tu ] , capture_output = True )


# the namespace of the names that a module exports; the AST is dumped only
# for it, since that of the whole translation unit, the standard library,
# Boost and Eigen included, takes some gigabytes
NAMESPACE = "SMSpp_di_unipi_it"


def dump_ast( clang , root , headers , flags ):
    """Returns the JSON AST of the blocks of NAMESPACE in a translation unit
    that includes the given headers of root/include."""
    inc = os.path.join( root , "include" )
    out = run_on( [ clang , "-std=c++20" , "-fsyntax-only" , "-I" + inc ,
                    *flags , "-Xclang" , "-ast-dump=json" , "-Xclang" ,
                    "-ast-dump-filter=" + NAMESPACE ] , headers )
    if out.returncode:
        errors = [ l for l in out.stderr.decode().splitlines()
                   if "error" in l ]
        sys.exit( "the headers do not compile together:\n  " +
                  "\n  ".join( errors[ :10 ] ) )
    # one JSON object for each block of the namespace, one after the other
    text , dec , pos , blocks = out.stdout.decode() , json.JSONDecoder() , \
                                0 , []
    while True:
        while ( pos < len( text ) ) and text[ pos ].isspace():
            pos += 1
        if pos == len( text ):
            break
        block , pos = dec.raw_decode( text , pos )
        blocks.append( block )
    return { "inner" : blocks }


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
    # a namespace whose classes are only declared in advance has no names
    return { k : v for k , v in names.items() if v } , internal


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
        return [ " * " + l for l in
                 textwrap.wrap( text , 75 , break_on_hyphens = False ) ]
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


def includes_of( clang , root , present , optional , flags ):
    """Returns the headers of root/include that the global module fragment
    includes: the optional ones, each in its #if, and among the others those
    that no other of them includes, plus any that these do not bring in after
    all (e.g., one included only inside an #if), as the dependencies that
    clang finds say."""
    inc = os.path.join( root , "include" )
    plain = [ h for h in present if h not in optional ]
    included = set()
    for h in plain:
        for l in open( os.path.join( inc , h ) , errors = "replace" ):
            m = re.match( r'\s*#\s*include\s*"([^"]+)"' , l )
            if m and ( m.group( 1 ) != h ):
                included.add( m.group( 1 ) )
    tops = [ h for h in plain if h not in included ]
    while True:
        reached = reached_by( clang , inc , tops , flags )
        missing = [ h for h in plain
                    if ( h not in tops ) and ( h not in reached ) ]
        if not missing:
            break
        tops.append( missing[ 0 ] )
    return sorted( tops + [ h for h in optional if h in
                            os.listdir( inc ) ] )


def reached_by( clang , inc , headers , flags ):
    """Returns the names of the headers of inc that a translation unit
    including the given ones reads."""
    out = run_on( [ clang , "-std=c++20" , "-M" , "-I" + inc , *flags ] ,
                  headers )
    if out.returncode:
        sys.exit( "the dependencies of the headers cannot be found:\n  " +
                  out.stderr.decode()[ :2000 ] )
    deps = out.stdout.decode().replace( "\\\n" , " " ).split()
    inc = os.path.realpath( inc )
    return { os.path.basename( d ) for d in deps
             if os.path.dirname( os.path.realpath( d ) ) == inc }


def interface( name , out_file , imports , optional , includes , names ,
               authors , what , excluded ):
    """Returns the text of the interface of the module name."""
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
    for h in includes:
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
    return "\n".join( out ) + "\n"


def read_interface( path ):
    """Returns how the interface in path was made: its name, the modules it
    imports, its optional headers with their macros, its excluded headers,
    the lines of its authors and the names that it exports, as
    { namespace : { name : header } }."""
    text = open( path ).read()
    lines = text.split( "\n" )
    name = re.search( r"^export module (\S+);" , text , re.M ).group( 1 )
    imports = re.findall( r"^export import (\S+);" , text , re.M )
    optional = dict( ( h , m ) for m , h in re.findall(
        r'^#if defined\( (\w+) \)\n #include "([^"]+)"' , text , re.M ) )
    end = lines.index( " */" )
    doc = " ".join( l[ 3: ] for l in lines[ :end ] if l.startswith( " * " ) )
    m = re.search( r"The headers (.*?) are not in this module" , doc )
    excluded = m.group( 1 ).split( ", " ) if m else []
    first = next( ( i for i in range( end ) if "\\author" in lines[ i ] ) ,
                  None )
    authors = [ " *" ] + lines[ first : end ] if first is not None else []
    names = collections.defaultdict( dict )
    ns , header = [] , ""
    for l in lines:
        m = re.match( r"^\s*(?:export )?namespace (\S+) \{$" , l )
        if m:
            ns.append( m.group( 1 ) )
            continue
        if re.match( r"^\s*\}$" , l ) and ns:
            ns.pop()
            continue
        m = re.match( r"^\s*// (\S+\.h)$" , l )
        if m:
            header = m.group( 1 )
            continue
        m = re.match( r"^\s*using (\S+);$" , l )
        if m:
            names[ "::".join( ns ) ][ m.group( 1 ).split( "::" )[ -1 ] ] = \
                header
    return name , imports , optional , excluded , authors , names


def flags_from( compdb , path ):
    """Returns the -I, -isystem, -D and -std flags with which the
    compile_commands.json compdb compiles the file path."""
    path = os.path.realpath( path )
    for e in json.load( open( compdb ) ):
        f = e[ "file" ]
        if not os.path.isabs( f ):
            f = os.path.join( e[ "directory" ] , f )
        if os.path.realpath( f ) != path:
            continue
        args = e.get( "arguments" ) or shlex.split( e[ "command" ] )
        out , i = [] , 1
        while i < len( args ):
            a = args[ i ]
            if a in ( "-isystem" , "-I" , "-D" ) and ( i + 1 < len( args ) ):
                out += [ a , args[ i + 1 ] ]
                i += 2
                continue
            if a.startswith( ( "-I" , "-isystem" , "-D" , "-std=" ) ):
                out.append( a )
            i += 1
        return out
    return None


def make( clang , root , out_file , name , imports , optional , excluded ,
          authors , flags , kept = None ):
    """Returns the interface out_file of the module name of root, with names of
    optional headers whose macro is not among the flags taken from kept,
    the names of an interface written before."""
    inc = os.path.join( root , "include" )
    exc = sorted( h for h in os.listdir( inc ) if h.endswith( ".h" ) and
                  any( fnmatch.fnmatch( h , e ) for e in excluded ) )
    headers = sorted( h for h in os.listdir( inc )
                      if h.endswith( ".h" ) and ( h not in exc ) )
    # an optional header whose macro is not among the flags is not there
    defined = set()
    for i , f in enumerate( flags ):
        if f == "-D" and ( i + 1 < len( flags ) ):
            defined.add( flags[ i + 1 ].split( "=" )[ 0 ] )
        elif f.startswith( "-D" ):
            defined.add( f[ 2: ].split( "=" )[ 0 ] )
    present = [ h for h in headers
                if ( h not in optional ) or ( optional[ h ] in defined ) ]
    names , internal = collect( dump_ast( clang , root , present , flags ) ,
                                inc )
    if internal:
        sys.exit( "names with internal linkage, which a module cannot "
                  "export:\n  " + "\n  ".join( internal ) )
    if kept:
        absent = { h for h in optional if h not in present }
        for ns , d in kept.items():
            for n , h in d.items():
                if h in absent:
                    names.setdefault( ns , {} )[ n ] = h

    what = "SMS++" if name == "smspp" else os.path.basename( root )
    if authors is None:
        if name == "smspp":
            authors = authors_of( os.path.join( inc , "Block.h" ) )
        else:
            # the header named as the module, or else the first that has them
            authors = []
            for h in [ what + ".h" ] + headers:
                if os.path.exists( os.path.join( inc , h ) ):
                    authors = authors_of( os.path.join( inc , h ) )
                    if authors:
                        break
    includes = includes_of( clang , root , present , optional , flags )
    return interface( name , out_file , imports , optional ,
                      includes , names , authors , what , sorted( excluded ) )


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
    ap.add_argument( "--update" , nargs = "*" , metavar = "CPPM" ,
                     help = "interfaces to write again from themselves, "
                            "with the flags from --compdb" )
    ap.add_argument( "--compdb" ,
                     help = "the compile_commands.json of a build of the "
                            "modules with Clang and Ninja" )
    ap.add_argument( "--check" , action = "store_true" ,
                     help = "with --update, prints the differences instead "
                            "of writing the interfaces" )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()

    if args.update is None:
        root = os.path.abspath( args.root )
        authors = None
        if args.authors:
            authors = authors_of( os.path.join( root , "include" ,
                                                args.authors ) )
        text = make( args.clang , root , args.out , args.name , args.imports ,
                     dict( o.split( "=" , 1 ) for o in args.optional ) ,
                     args.exclude , authors , args.flags )
        with open( os.path.join( root , args.out ) , "w" ) as f:
            f.write( text )
        return

    if not args.compdb:
        sys.exit( "--update needs --compdb" )
    differ = False
    for cppm in args.update:
        cppm = os.path.abspath( cppm )
        flags = flags_from( args.compdb , cppm )
        if flags is None:
            print( f"{cppm}: not in {args.compdb}, skipped" )
            continue
        root = os.path.dirname( os.path.dirname( cppm ) )
        name , imports , optional , excluded , authors , kept = \
            read_interface( cppm )
        text = make( args.clang , root , cppm , name , imports , optional ,
                     excluded , authors , flags , kept )
        old = open( cppm ).read()
        if text == old:
            continue
        if args.check:
            differ = True
            sys.stdout.writelines( difflib.unified_diff(
                old.splitlines( True ) , text.splitlines( True ) ,
                cppm , cppm + " (written again)" ) )
        else:
            with open( cppm , "w" ) as f:
                f.write( text )
            print( f"{cppm}: written again" )
    if differ:
        sys.exit( "the interfaces above are not those that the headers "
                  "give: write them again with --update" )


if __name__ == "__main__":
    main()

###############################################################################
#                     End File smspp_module_exports.py                        #
###############################################################################
