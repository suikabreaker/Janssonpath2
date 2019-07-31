#ifndef JSONPATH_AST_H
#define JSONPATH_AST_H

#include "jansson.h"

// structures below are not visiable external. in interface there's only jsonpath_t*.

// we avoid to contain jsonpath_t* node here as $.a[1].tag is a really commonly seen pattern.
// straightforward implementation would make it like a linked list which causes indirect memory access at execution.
// this affects performance expecially for filter expression.
// instead we save it as an array.

struct jsonpath_t;
typedef struct jsonpath_t jsonpath_t;

typedef enum path_index_tag_t {
	// .? | [?] | ..? 
	INDEX_DOT, INDEX_SUB_SIMPLE = INDEX_DOT, INDEX_DOT_RECURSIVE, INDEX_SUB_EXP, INDEX_SUB_RANGE, INDEX_FILTER, INDEX_MAX
}path_index_tag_t;
typedef struct path_index_t {
	path_index_tag_t tag;
	union {
		// NULL for *, "#" for length, minus for counting back, INT_MAX for end(one pass the last)
		// path_index_t is responsible to lifetime of objects below
		json_t* simple_index; // including recursive
		jsonpath_t* range[2]; // [begin: end]
		jsonpath_t* expression; // including filter
	};
}path_index_t;

typedef struct path_indexes_t {
	jsonpath_t* root_node; // note it can be a calculated one
	path_index_t* indexes;
	size_t size;
	size_t capacity;
}path_indexes_t;

typedef enum path_unary_tag_t {
	// ! | + | - | & | * | ~
	UNARY_NOT, UNARY_POS, UNARY_NEG, UNARY_TO_LIST, UNARY_FROM_LIST, UNARY_BITNOT, UNARY_MAX
}path_unary_tag_t;
typedef struct path_unary_t {
	path_unary_tag_t tag;
	jsonpath_t* node;
}path_unary_t;

typedef enum path_binary_tag_t {
	// + | - | * | / | %
	BINARY_ADD, BINARY_MNU, BINARY_MUL, BINARY_DIV, BINARY_REMINDER,
	//  & | '|' | ^ | && | '||' 
	BINARY_BITAND, BINARY_BITOR, BINARY_BITXOR, BINARY_AND, BINARY_OR,
	// < | == | > | <= | >=
	BINARY_LT, BINARY_EQ, BINARY_GT, BINARY_LE, BINARY_GE,
	// ++
	BINARY_LIST_CON,
#ifdef JANSSONPATH_SUPPORT_REGEX
	// =~
	BINARY_REGEX,
#endif
	BINARY_MAX
}path_binary_tag_t;
typedef struct path_binary_t {
	path_binary_tag_t tag;
	jsonpath_t* lhs;
	jsonpath_t* rhs;
}path_binary_t;

typedef enum path_arbitrary_tag_t {
	// function call | // should we support to form JSON nodes in janssonpath?
	ARB_FUNC
}path_arbitrary_tag_t;
typedef struct path_arbitrary_t {
	path_arbitrary_tag_t tag;
	char* func_name;
	jsonpath_t** nodes;
	size_t size;
	size_t capacity;
}path_arbitrary_t;

typedef enum path_single_tag_t {
	// $ | @ | constant
	SINGLE_ROOT, SINGLE_CURR, SINGLE_CONST, SINGLE_MAX
}path_single_tag_t;
typedef struct path_single_t {
	path_single_tag_t tag;
	json_t* constant;
}path_single_t;

typedef enum jsonpath_tag_t {
	// single | index | unary | binary | arbitray
	JSON_SINGLE, JSON_INDEX, JSON_UNARY, JSON_BINARY, JSON_ARBITRAY
}jsonpath_tag_t;
struct jsonpath_t {
	jsonpath_tag_t tag;
	// not using pointer, to reduce indirections
	union {
		path_single_t single;
		path_indexes_t indexes;
		path_unary_t unary;
		path_binary_t binary;
		path_arbitrary_t arbitrary;
	};
};

#endif