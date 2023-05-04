%define api.pure full
%locations

%code requires {
    #include <memory>
    #include <cstdio>
    #include <cstdint>
    #include <cstring>
    #include <iostream>
    #include "tree-gen.hpp"
    typedef void* yyscan_t;
}

%code {
    int yylex(YYSTYPE* yylvalp, YYLTYPE* yyllocp, yyscan_t scanner);
    void yyerror(YYLTYPE* yyllocp, yyscan_t scanner, tree_gen::Specification &specification, const char* msg);
}

%code top {
    #define TRY try {
    #define CATCH } catch (std::exception &e) { yyerror(&yyloc, scanner, specification, e.what()); }

    #include <algorithm>
    #include <cctype>
    #include <locale>

    // trim from start (in place)
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline std::string trim(const std::string &s) {
        auto copy = std::string(s);
        ltrim(copy);
        rtrim(copy);
        return copy;
    }

}

%param { yyscan_t scanner }
%parse-param { tree_gen::Specification &specification }

/* YYSTYPE union */
%union {
    char                        *str;
    std::string                 *xstr;
    std::list<std::string>      *xsl;
    tree_gen::NodeBuilder       *nbld;
};

/* Typenames for nonterminals */
%type <xstr> Documentation Identifier String
%type <xsl>  Identifiers
%type <nbld> Node

/* Tokens */
%token <str> DOCSTRING
%token <str> INCLUDE SRC_INCLUDE PY_INCLUDE
%token SOURCE HEADER PYTHON TREE_NS SUPPORT_NS INIT_FN SERDES_FNS SOURCE_LOC
%token NAMESPACE NAMESPACE_SEP
%token ERROR
%token MAYBE ONE ANY MANY OLINK LINK EXT
%token REORDER
%token <str> IDENT STRING
%token '{' '}' '(' ')' '<' '>' ':' ';'
%token BAD_CHARACTER

/* Misc. Yacc directives */
%define parse.error verbose
%start Root

%%

Documentation   :                                                               { TRY $$ = new std::string(); CATCH }
                | Documentation DOCSTRING                                       { TRY $$ = $1; if (!$$->empty()) *$$ += " "; *$$ += trim(std::string($2 + 1)); std::free($2); CATCH }
                ;

Identifier      : IDENT                                                         { TRY $$ = new std::string($1); std::free($1); CATCH }
                | Identifier NAMESPACE_SEP IDENT                                { TRY $$ = $1; *$$ += "::"; *$$ += $3; std::free($3); CATCH }
                ;

String          : STRING                                                        { TRY $1[std::strlen($1) - 1] = 0; $$ = new std::string($1 + 1); std::free($1); CATCH }
                ;

Identifiers     : Identifier                                                    { TRY $$ = new std::list<std::string>(); $$->emplace_back(std::move(*$1)); delete $1; CATCH }
                | Identifiers ',' Identifier                                    { TRY $$ = $1; $$->emplace_back(std::move(*$3)); delete $3; CATCH }
                ;

Node            : Documentation IDENT '{'                                       { TRY auto nb = std::make_shared<tree_gen::NodeBuilder>(std::string($2), *$1); specification.add_node(nb); $$ = nb.get(); delete $1; std::free($2); CATCH }
                | Node ERROR ';'                                                { TRY $$ = $1->mark_error(); CATCH }
                | Node Documentation IDENT ':' MAYBE '<' Identifier '>' ';'     { TRY $$ = $1->with_child(tree_gen::Maybe, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' ONE '<' Identifier '>' ';'       { TRY $$ = $1->with_child(tree_gen::One, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' ANY '<' Identifier '>' ';'       { TRY $$ = $1->with_child(tree_gen::Any, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' MANY '<' Identifier '>' ';'      { TRY $$ = $1->with_child(tree_gen::Many, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' OLINK '<' Identifier '>' ';'     { TRY $$ = $1->with_child(tree_gen::OptLink, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' LINK '<' Identifier '>' ';'      { TRY $$ = $1->with_child(tree_gen::Link, *$7, std::string($3), *$2); delete $2; std::free($3); delete $7; CATCH }
                | Node Documentation IDENT ':' EXT MAYBE '<' Identifier '>' ';' { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::Maybe); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' EXT ONE '<' Identifier '>' ';'   { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::One); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' EXT ANY '<' Identifier '>' ';'   { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::Any); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' EXT MANY '<' Identifier '>' ';'  { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::Many); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' EXT OLINK '<' Identifier '>' ';' { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::OptLink); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' EXT LINK '<' Identifier '>' ';'  { TRY $$ = $1->with_prim(*$8, std::string($3), *$2, tree_gen::Link); delete $2; std::free($3); delete $8; CATCH }
                | Node Documentation IDENT ':' Identifier ';'                   { TRY $$ = $1->with_prim(*$5, std::string($3), *$2, tree_gen::Prim); delete $2; std::free($3); delete $5; CATCH }
                | Node Documentation REORDER '(' Identifiers ')' ';'            { TRY $$ = $1->with_order(std::move(*$5)); delete $2; delete $5; CATCH }
                | Node Node '}'                                                 { TRY $2->derive_from($1->node); CATCH }
                ;

Root            :                                                               {}
                | Root Documentation SOURCE                                     { TRY specification.set_source_doc(*$2); delete $2; CATCH }
                | Root Documentation HEADER                                     { TRY specification.set_header_doc(*$2); delete $2; CATCH }
                | Root Documentation HEADER String                              { TRY specification.set_header_doc(*$2); delete $2; specification.set_header_fname(*$4); delete $4; CATCH }
                | Root Documentation PYTHON                                     { TRY specification.set_python_doc(*$2); delete $2; CATCH }
                | Root TREE_NS Identifier                                       { TRY specification.set_tree_namespace(*$3); delete $3; CATCH }
                | Root SUPPORT_NS Identifier                                    { TRY specification.set_support_namespace(*$3); delete $3; CATCH }
                | Root INIT_FN Identifier                                       { TRY specification.set_initialize_function(*$3); delete $3; CATCH }
                | Root SERDES_FNS Identifier Identifier                         { TRY specification.set_serdes_functions(*$3, *$4); delete $3; delete $4; CATCH }
                | Root SOURCE_LOC Identifier                                    { TRY specification.set_source_location(*$3); delete $3; CATCH }
                | Root INCLUDE                                                  { TRY specification.add_include(std::string($2)); std::free($2); CATCH }
                | Root SRC_INCLUDE                                              { TRY specification.add_src_include(std::string($2 + 4)); std::free($2); CATCH }
                | Root PY_INCLUDE                                               { TRY specification.add_python_include(std::string($2)); std::free($2); CATCH }
                | Root Documentation NAMESPACE IDENT                            { TRY specification.add_namespace(std::string($4), *$2); delete $2; std::free($4); CATCH }
                | Root Node '}'                                                 {}
                ;

%%

void yyerror(YYLTYPE* yyllocp, yyscan_t unused, tree_gen::Specification &specification, const char* msg) {
    (void)unused;
    (void)specification;
    std::cerr << "Parse error at " << ":"
              << yyllocp->first_line << ":" << yyllocp->first_column << ".."
              << yyllocp->last_line << ":" << yyllocp->last_column
              << ": " << msg;
}
