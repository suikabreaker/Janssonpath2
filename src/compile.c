#include <string.h>
#include "private/common.h"
#include "private/jsonpath_ast.h"
#include "private/jansson_memory.h"
#include "private/lexeme.h"

// todo: add optional support for wildcard and regular expression
// we allow wild card in subscribe string indexing like ["test_*"], but not in dot indexing
// because it causes ambiguity
// should we support [$.pattern] where $.pattern is a wildcard pattern? It could be problematical

// structures are not of large size. use return value rather than pointer
// and potentially it can be optimized out with RVO
// functions below steals ownership of passed in values. remember to incref them if it will be used elsewhere.

void JANSSONPATH_EXPORT jsonpath_release(jsonpath_t* jsonpath);

// use static to expect inline optimization
// pass by value for release functions, because them are small to pass, and safe to copy(as they are not referenced by address)

static path_index_t build_simple_index(json_t* index){
	path_index_t ret = { INDEX_SUB_SIMPLE, {.simple_index=index} };
	return ret;
}

static path_index_t build_recursive_index(json_t* index) {
	path_index_t ret = { INDEX_DOT_RECURSIVE, {.simple_index = index} };
	return ret;
}

static path_index_t build_subexp_index(jsonpath_t* index){
	path_index_t ret = { INDEX_SUB_EXP, {.expression = index} };
	return ret;
}

static path_index_t build_filter_index(jsonpath_t* filter) {
	path_index_t ret = { INDEX_FILTER, {.expression = filter} };
	return ret;
}

static path_index_t build_range_index(jsonpath_t* w_begin, jsonpath_t* end) {
	path_index_t ret = { INDEX_SUB_RANGE, {.range = {w_begin, end}} };
	return ret;
}

static void index_release(path_index_t path){
	switch(path.tag){
	case INDEX_DOT:
	case INDEX_DOT_RECURSIVE:
		json_decref(path.simple_index);
		break;
	case INDEX_SUB_EXP:
	case INDEX_FILTER:
		jsonpath_release(path.expression);
		break;
	case INDEX_SUB_RANGE:
		jsonpath_release(path.range[0]);
		jsonpath_release(path.range[1]);
		break;
	default:{
		assert(false);
		break;
	}
	}
}

static jsonpath_t* build_indexes(jsonpath_t* root){
	static const size_t indexes_capacity_default = 4;
	path_index_t* indexes = do_malloc(sizeof(path_index_t) * indexes_capacity_default);
	path_indexes_t real_node = { root, indexes, 0, indexes_capacity_default };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_INDEX;
	ret->indexes = real_node;
	return ret;
}

static void add_index(jsonpath_t* index_node, path_index_t index) {
	assert(index_node->tag == JSON_INDEX);
	path_indexes_t* indexes = &index_node->indexes;
	if (indexes->size + 1 > indexes->capacity) {
		path_index_t* new_nodes = do_malloc(sizeof(path_index_t) * indexes->capacity * 2);
		memcpy(new_nodes, indexes->indexes, indexes->size);
		do_free(indexes->indexes);
		indexes->indexes = new_nodes;
	}
	indexes->indexes[indexes->size] = index;
	++indexes->size;
}

static void indexes_release(path_indexes_t indexes){
	jsonpath_release(indexes.root_node);
	size_t i;
	for(i=0;i<indexes.size;++i){
		index_release(indexes.indexes[i]);
	}
	do_free(indexes.indexes);
}

static jsonpath_t* build_unary(path_unary_tag_t type, jsonpath_t* oprand){
	path_unary_t real_node = { type,oprand };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_UNARY;
	ret->unary = real_node;
	return ret;
}

static void unary_release(path_unary_t unary){
	jsonpath_release(unary.node);
}

static jsonpath_t* build_binary(path_binary_tag_t type, jsonpath_t* lhs, jsonpath_t* rhs) {
	path_binary_t real_node = { type, lhs, rhs };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_BINARY;
	ret->binary = real_node;
	return ret;
}

static void binary_release(path_binary_t binary) {
	jsonpath_release(binary.lhs);
	jsonpath_release(binary.rhs);
}

static jsonpath_t* build_func_call(char * func_name){ // note func_name will be deleted, make copy!
	static const size_t nodes_capacity_default = 4;// it should be sufficient for most call
	jsonpath_t** nodes = do_malloc(sizeof(jsonpath_t*) * nodes_capacity_default);
	path_arbitrary_t real_node = { ARB_FUNC, func_name, nodes, 0, nodes_capacity_default };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_ARBITRAY;
	ret->arbitrary = real_node;
	return ret;
}

static void add_oprand_arbitrary(jsonpath_t* arbitrary_node, jsonpath_t* oprand) {
	assert(arbitrary_node->tag == JSON_ARBITRAY);
	path_arbitrary_t* arbitrary = &arbitrary_node->arbitrary;
	if (arbitrary->size + 1 > arbitrary->capacity) {
		jsonpath_t** new_nodes = do_malloc(sizeof(jsonpath_t*) * arbitrary->capacity * 2);
		memcpy(new_nodes, arbitrary->nodes, arbitrary->size);
		do_free(arbitrary->nodes);
		arbitrary->nodes = new_nodes;
	}
	arbitrary->nodes[arbitrary->size] = oprand;
	++arbitrary->size;
}

static void arbitray_release(path_arbitrary_t arbitrary){
	size_t i;
	for(i=0;i<arbitrary.size;++i){
		jsonpath_release(arbitrary.nodes[i]);
	}
	do_free(arbitrary.func_name);
	do_free(arbitrary.nodes);
}

static jsonpath_t* make_root(){
	path_single_t real_node = { SINGLE_ROOT, NULL };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_SINGLE;
	ret->single = real_node;
	return ret;
}

static jsonpath_t* make_curr() {
	path_single_t real_node = { SINGLE_CURR, NULL };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_SINGLE;
	ret->single = real_node;
	return ret;
}

static void single_release(path_single_t single){
	if (single.tag != SINGLE_CONST)return; // json_decref on null is safe, should we remove this condition?
	json_decref(single.constant);
}

void JANSSONPATH_EXPORT jsonpath_release(jsonpath_t* jsonpath) {
	switch (jsonpath->tag) {
	case JSON_SINGLE:
		single_release(jsonpath->single);
		break;
	case JSON_INDEX:
		indexes_release(jsonpath->indexes);
		break;
	case JSON_UNARY:
		unary_release(jsonpath->unary);
		break;
	case JSON_BINARY:
		binary_release(jsonpath->binary);
		break;
	case JSON_ARBITRAY:
		arbitray_release(jsonpath->arbitrary);
		break;
	default:
		break;
	}
	do_free(jsonpath);
}

#define w_begin (*pw_begin)
static string_slice word_peek;

static const int JANSSONPATH_NO_EXPORT binary_precedence[BINARY_MAX + 1] = {
	8,8,9,9,9,
	4,3,2,1,0,7,7,
	5,5,6,6,6,6,
	8,
#ifdef JANSSONPATH_SUPPORT_REGEX
	5,
#endif
	- 1
};

static const int binary_precedence_max = 9;

static const char mul_op_sequnce[][3] = {
	"&&","||","<<",">>",
	"==","!=","<=",">=",
	"++",
#ifdef JANSSONPATH_SUPPORT_REGEX
		"=~",
#endif // JANSSONPATH_SUPPORT_REGEX
};

static const char mul_op_sequnce_value[] = {
	BINARY_AND,BINARY_OR,BINARY_LSH,BINARY_RSH,
	BINARY_EQ,BINARY_NE,BINARY_LE,BINARY_GE,
	BINARY_LIST_CON,
#ifdef JANSSONPATH_SUPPORT_REGEX
	BINARY_REGEX,
#endif // JANSSONPATH_SUPPORT_REGEX
};

static string_slice next_word_with_merge(
	const char** pw_begin, const char* w_end, jsonpath_error_t* error
) {
	string_slice word1 = next_nonspace_lexeme(&w_begin, w_end, error);
	if (error->code || is_eof(word1)) return word1;

	const char* backup_w_begin = w_begin;
	jsonpath_error_t backup_error = *error;
	string_slice word2 = next_lexeme(&w_begin, w_end, error);
	


	size_t i;
	if(!error->code) {
		for (i = 0; i < sizeof(mul_op_sequnce) / sizeof(mul_op_sequnce[0]); ++i) {
			if (is_punctor(word1, mul_op_sequnce[i][0]) && is_punctor(word2, mul_op_sequnce[i][1])) {
				string_slice ret = { word1.begin,word2.end };
				return ret;
			}
		}
	}
	w_begin = backup_w_begin;
	*error = backup_error;
	return word1;
}

#define init_peek() do{word_peek=next_word_with_merge(&w_begin, w_end, error);}while(0)
#define go_next init_peek

static path_binary_tag_t map_to_bin_op(string_slice slice) {
	static const char single_key[] = "+-*/%^&|<>";
	static const path_binary_tag_t single_value[] = {
		BINARY_ADD,BINARY_MNU,BINARY_MUL,BINARY_DIV,BINARY_REMINDER,BINARY_BITXOR,BINARY_BITAND,BINARY_BITOR,BINARY_LT,BINARY_GT
	};

	size_t i;
	for (i = 0; i < sizeof(single_key) / sizeof(char); ++i) {
		if (is_punctor(word_peek, single_key[i])) {
			return single_value[i];
		}
	}

	// maybe bad practices
	if (slice.end - slice.begin < 2) return BINARY_MAX;
	for (i = 0; i < sizeof(mul_op_sequnce) / sizeof(mul_op_sequnce[0]); ++i) {
		if(!strncmp(mul_op_sequnce[i],slice.begin,2)){
			return mul_op_sequnce_value[i];
		}
	}
		
	return BINARY_MAX;
}

static jsonpath_t* parse_path(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	return NULL;
}

static jsonpath_t* parse_unary(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	return NULL;
}

#define child_node(down_grade, w_begin, w_end, precedence, error) (down_grade?parse_unary(&w_begin, w_end, error):parse_binary(&w_begin, w_end, precedence + 1, error))

// they have lower precedence than all others
static jsonpath_t* parse_binary(const char** pw_begin, const char* w_end, int precedence, jsonpath_error_t* error){
	bool down_grade = binary_precedence_max == precedence;
	jsonpath_t* left_node = child_node(down_grade, w_begin, w_end, precedence, error);
	path_binary_tag_t bin_op = map_to_bin_op(word_peek);
	int curr_precedence = binary_precedence[bin_op];
	while (curr_precedence == precedence) {
		go_next();
		jsonpath_t* right_node = child_node(down_grade, w_begin, w_end, precedence, error);
		left_node = build_binary(bin_op, left_node, right_node);
	}
	return left_node;
}

JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile(const char* jsonpath_begin, const char* jsonpath_end, jsonpath_error_t* error) {
	const char** pw_begin = &jsonpath_begin;
	const char* w_end = jsonpath_end;
	init_peek();
	if (error->code) return NULL;
	return parse_binary(&w_begin, w_end, 0, error);
}
