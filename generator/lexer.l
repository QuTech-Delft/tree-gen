%option noinput nounput noyywrap 8bit nodefault
%option yylineno
%option reentrant bison-bridge bison-locations

%{
    #include "parser.hpp"

    //#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
    #define DEBUG(...)

    #define YY_USER_ACTION yylloc->last_column += yyleng;
    #define LOCATION_RESET \
        yylloc->first_column = 1; yylloc->first_line = 1; \
        yylloc->last_column = 1; yylloc->last_line = 1;
    #define LOCATION_LINE \
        yylloc->last_column = 1; yylloc->last_line += 1;
    #define LOCATION_STEP \
        yylloc->first_column = yylloc->last_column; \
        yylloc->first_line = yylloc->last_line;
    #define WITH_STR(TOKNAME) \
        DEBUG("Pushing %s token (%s) starting at %d:%d\n", #TOKNAME, yytext, yylloc->first_line, yylloc->first_column); \
        yylval->str = strdup(yytext); return TOKNAME
    #define WITHOUT_STR(TOKNAME) \
        DEBUG("Pushing %s token starting at %d:%d\n", #TOKNAME, yylloc->first_line, yylloc->first_column); \
        return TOKNAME
    #define OPERATOR \
        DEBUG("Pushing '%c' token starting at %d:%d\n", yytext[0], yylloc->first_line, yylloc->first_column); \
        return yytext[0]
    #define DELETE LOCATION_STEP
%}

%%

%{
    LOCATION_STEP;
%}

<INITIAL>{

    /* Newline */
(\n|\r\n?)                                          LOCATION_LINE; DELETE;

    /* Docstrings */
#.*                                                 WITH_STR(DOCSTRING);

    /* Comments */
[/][/].*                                            DELETE;

    /* Gobble up whitespace */
[ \t]+                                              DELETE;

    /* Generator control keywords */
source                                              WITHOUT_STR(SOURCE);
header                                              WITHOUT_STR(HEADER);
python                                              WITHOUT_STR(PYTHON);
tree_namespace                                      WITHOUT_STR(TREE_NS);
support_namespace                                   WITHOUT_STR(SUPPORT_NS);
initialize_function                                 WITHOUT_STR(INIT_FN);
serdes_functions                                    WITHOUT_STR(SERDES_FNS);
location                                            WITHOUT_STR(SOURCE_LOC);
include[ \t].*                                      WITH_STR(INCLUDE);
src_include[ \t].*                                  WITH_STR(SRC_INCLUDE);
import[ \t].*                                       WITH_STR(PY_INCLUDE);
from[ \t].*[ \t]import[ \t].*                       WITH_STR(PY_INCLUDE);

    /* Namespaces */
namespace                                           WITHOUT_STR(NAMESPACE);
::                                                  WITHOUT_STR(NAMESPACE_SEP);

    /* Error marker keyword */
error                                               WITHOUT_STR(ERROR);

    /* Edge types */
Maybe                                               WITHOUT_STR(MAYBE);
One                                                 WITHOUT_STR(ONE);
Any                                                 WITHOUT_STR(ANY);
Many                                                WITHOUT_STR(MANY);
OptLink                                             WITHOUT_STR(OLINK);
Link                                                WITHOUT_STR(LINK);

    /* Parameter reordering for compatibility when hierarchy changes */
reorder                                             WITHOUT_STR(REORDER);

    /* External node (typedef'd multiplicity thing from another tree) */
external                                            WITHOUT_STR(EXT);

    /* Identifiers */
[_a-zA-Z][_a-zA-Z0-9]*                              WITH_STR(IDENT);

    /* Strings */
["][^"]+["]                                         WITH_STR(STRING);

    /* Interpunction */
[(){}<>:;,]                                         OPERATOR;

    /* Unknown character in input */
.                                                   WITHOUT_STR(BAD_CHARACTER);

}
