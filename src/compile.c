#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
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
		if (path.simple_index)json_decref(path.simple_index);
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

static jsonpath_t* build_func_call(json_t* func_name){
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
	json_decref(arbitrary.func_name);
	do_free(arbitrary.nodes);
}

static jsonpath_t* make_root(void){
	path_single_t real_node = { SINGLE_ROOT, NULL };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_SINGLE;
	ret->single = real_node;
	return ret;
}

static jsonpath_t* make_curr(void) {
	path_single_t real_node = { SINGLE_CURR, NULL };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_SINGLE;
	ret->single = real_node;
	return ret;
}

static jsonpath_t* make_const(json_t* constant) {
	path_single_t real_node = { SINGLE_CONST, constant };
	jsonpath_t* ret = do_malloc(sizeof(jsonpath_t));
	ret->tag = JSON_SINGLE;
	ret->single = real_node;
	return ret;
}

static void single_release(path_single_t single){
	if (single.tag != SINGLE_CONST)return; // json_decref on null is safe, should we remove this condition?
	json_decref(single.constant);
}

void JANSSONPATH_NO_EXPORT jsonpath_release_no_free(jsonpath_t* jsonpath) {
	if (!jsonpath) return;
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
#ifdef JANSSONPATH_CONSTANT_FOLD
	case JSON_CONSTANT:
		jsonpath_decref(jsonpath->constant_result);
		break;
#endif
	default:
		break;
	}
}

void JANSSONPATH_EXPORT jsonpath_release(jsonpath_t* jsonpath) {
	jsonpath_release_no_free(jsonpath);
	do_free(jsonpath);
}

#define w_begin (*pw_begin)
static string_slice word_peek;

// larger the number, higher the precedence
static const int binary_precedence[BINARY_MAX + 1] = {
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

static const path_binary_tag_t mul_op_sequnce_value[] = {
	BINARY_AND,BINARY_OR,BINARY_LSH,BINARY_RSH,
	BINARY_EQ,BINARY_NE,BINARY_LE,BINARY_GE,
	BINARY_ARRAY_CON,
#ifdef JANSSONPATH_SUPPORT_REGEX
	BINARY_REGEX,
#endif // JANSSONPATH_SUPPORT_REGEX
};

static string_slice next_word_with_merge(
	const char** pw_begin, const char* w_end, jsonpath_error_t* error
) {
	string_slice word1 = next_nonspace_lexeme(&w_begin, w_end, error);
	if (error->abort || is_eof(word1)) return word1;

	const char* backup_w_begin = w_begin;
	jsonpath_error_t backup_error = *error;
	string_slice word2 = next_lexeme(&w_begin, w_end, error);

	size_t i;
	if(!error->abort) {
		for (i = 0; i < sizeof(mul_op_sequnce) / sizeof(mul_op_sequnce[0]); ++i) {
			if (is_punctor(word1, mul_op_sequnce[i][0]) && is_punctor(word2, mul_op_sequnce[i][1])) {
				string_slice ret = { word1.begin,word2.end };
				return ret;
			}
		}
	}

	if(is_punctor(word1,'.')&& is_punctor(word2, '.')){
		string_slice ret = { word1.begin,word2.end };
		return ret;
	}

	w_begin = backup_w_begin;
	*error = backup_error;
	return word1;
}

#define go_next() do{word_peek=next_word_with_merge(&w_begin, w_end, error);}while(is_space(word_peek)&&!error->abort)
#define init_peek go_next

typedef enum path_indicate{
	PATH_IND_DOT, PATH_IND_DDOT, PATH_IND_LBR, PATH_IND_MAX
}path_indicate;

static path_indicate map_to_path_ind(string_slice slice) {
	if (is_punctor(slice, '.')) return PATH_IND_DOT;
	if (is_punctor(slice, '[')) return PATH_IND_LBR;

	if (slice.end - slice.begin != 2) return PATH_IND_MAX;
	if (!strncmp("..", slice.begin, 2)) {
		return PATH_IND_DDOT;
	}
	
	return PATH_IND_MAX;
}

static path_unary_tag_t map_to_unary_op(string_slice slice) {
	static const char single_key[] = "!+-&*~";
	static const path_unary_tag_t single_value[] = {
		UNARY_NOT, UNARY_POS, UNARY_NEG, UNARY_TO_ARRAY, UNARY_FROM_ARRAY, UNARY_BITNOT
	};

	size_t i;
	for (i = 0; i < sizeof(single_key) / sizeof(char); ++i) {
		if (is_punctor(slice, single_key[i])) {
			return single_value[i];
		}
	}

	return UNARY_MAX;
}

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
	if (slice.end - slice.begin != 2) return BINARY_MAX;
	for (i = 0; i < sizeof(single_value) / sizeof(single_value[0]); ++i) {
		if(!strncmp(mul_op_sequnce[i],slice.begin,2)){
			return mul_op_sequnce_value[i];
		}
	}
		
	return BINARY_MAX;
}

static jsonpath_t* parse_binary(const char** pw_begin, const char* w_end, int precedence, jsonpath_error_t* error);
static jsonpath_t* match_brackets(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	size_t count = 0;
	while(is_punctor(word_peek, '(')){
		count++;
		go_next();
		if (error->abort) return NULL;
	}
	jsonpath_t* ret = parse_binary(&w_begin, w_end, 0, error);
	if (!ret) return ret;
	while (is_punctor(word_peek, ')') && count) {
		count--;
		go_next();
		if (error->abort) {
			jsonpath_release(ret);
			return NULL;
		}
	}
	if (count != 0) {
		jsonpath_release(ret);
		error->abort = true;
		error->code = 0x61;
		error->reason = "unmacthed bracket";
		error->extra = (void*)w_begin;
		return NULL;
	}
	return ret;
}

static jsonpath_t* parse_arbitray(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	assert(is_identifier(word_peek)); // should we accept string/calculated function name?
	
	jsonpath_t* func_call = build_func_call(json_stringn(word_peek.begin, SLICE_SIZE(word_peek)));
	do{
		go_next();
		if (error->abort) break;
		if (!is_punctor(word_peek, '(')) {// should we accept named constant? shoulde we accept emit () for function with no parameter?
			return func_call;
		}
		go_next();
		if (error->abort) break;
		while (!is_punctor(word_peek, ')')) {
			jsonpath_t* argument = parse_binary(&w_begin, w_end, 0, error);
			if (!argument) break;
			add_oprand_arbitrary(func_call, argument);
			if (!is_punctor(word_peek, ',') && !is_punctor(word_peek, ')')) { // yes, we accept extra ',' on end of argument list, like func(1,2,)
				break;
			}
			if (is_punctor(word_peek, ',')) {
				go_next();
				if (error->abort) break;
			}
		}
		go_next();
		if (error->abort) break;
	} while (0);
	if(error->abort){
		jsonpath_release(func_call);
	}
	return func_call;
}

static const path_index_t error_index = { INDEX_MAX,{.simple_index = NULL} };

static path_index_t parse_index_sub(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	// brackets in ?(exp) (exp) can be omitted, so they could be treat as simple expressions in grammar
	if (is_punctor(word_peek, '?')) {
		go_next();
		if (error->abort) return error_index;
		jsonpath_t* filter = parse_binary(&w_begin, w_end, 0, error);
		if (!filter) return error_index;
		path_index_t ret = { INDEX_FILTER,{.expression = filter} };
		return ret;
	}
	else {
		jsonpath_t* range[2] = { NULL,NULL };
		do{
			bool is_range = false;
			if (!is_punctor(word_peek, ':')) {
				range[0] = parse_binary(&w_begin, w_end, 0, error);
				if (!range[0]) break;
			}
			if (is_punctor(word_peek, ':')) {
				is_range = true;
				go_next();
				if (error->abort) break;
				if (!is_punctor(word_peek, ']')) { // should we allow [:] ? it's simply a translation from array to collection
					range[1] = parse_binary(&w_begin, w_end, 0, error);
					if (!range[1]) break;
				}
			}
			return is_range ? build_range_index(range[0], range[1]) : build_subexp_index(range[0]);
		} while (0);
		jsonpath_release(range[0]);
		jsonpath_release(range[1]);
		return error_index;
	}
}

static path_index_t parse_index(path_indicate path_ind, const char** pw_begin, const char* w_end, jsonpath_error_t* error){
	go_next();
	if (error->abort) return error_index;
	switch (path_ind) {
	case PATH_IND_DOT:
	case PATH_IND_DDOT: {
		json_t* index_simple = NULL;
		if (is_identifier(word_peek)) {
			index_simple = json_stringn(word_peek.begin, SLICE_SIZE(word_peek));
		}
		else if (is_punctor(word_peek, '*')) {
			index_simple = NULL;
		}
		else if (is_punctor(word_peek, '#')) {
			index_simple = json_null();
		}
		else { // should we support string here?
			error->abort = true;
			error->code = 0x62;
			error->reason = "expecting simple index(*/#/identifier)";
			error->extra = w_begin;
			return error_index;
		}
		go_next();
		if (error->abort) {
			json_decref(index_simple);
			return error_index;
		}
		return (path_ind == PATH_IND_DOT) ? build_simple_index(index_simple) : build_recursive_index(index_simple);
	}
	case PATH_IND_LBR: {
		path_index_t ret = parse_index_sub(pw_begin, w_end, error);
		if (ret.tag == INDEX_MAX) return ret;
		if(is_punctor(word_peek,']')){
			go_next();
			if (error->abort) {
				index_release(ret);
				return error_index;
			}
			return ret;
		}else{
			index_release(ret);
			error->abort = true;
			error->code = 0x65;
			error->reason = "expecting ']'";
			error->extra = w_begin;
			return error_index;
		}
	}
	default:
		assert(false);
		return error_index;
	}
}

static jsonpath_t* parse_path(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	jsonpath_t* root = NULL;
	bool is_empty = false;
	if (is_identifier(word_peek)) {
		root = parse_arbitray(pw_begin, w_end, error);
	}else if(is_punctor(word_peek, '(')) {
		root = match_brackets(pw_begin, w_end, error);
	}else if (is_punctor(word_peek, '$')) {
		go_next();
		if (error->abort) return NULL;
		root = make_root();
	}else if (is_punctor(word_peek, '@')) {
		go_next();
		if (error->abort) return NULL;
		root = make_curr();
	}else{
		is_empty = true;
		root = make_curr(); // root node can be omitted and default to @
	}
	if (!root) return NULL;
	jsonpath_t* path = build_indexes(root);

	path_indicate path_ind = map_to_path_ind(word_peek);
	while(path_ind!=PATH_IND_MAX){
		path_index_t index = parse_index(path_ind, pw_begin, w_end, error);
		if (index.tag == INDEX_MAX) {
			jsonpath_release(path);
			return NULL; // that's why i prefer exceptions
		}
		is_empty = false;
		add_index(path, index);
		path_ind = map_to_path_ind(word_peek);
	}
	if(is_empty){
		error->abort = true;
		error->code = 0x88;
		error->reason = "expecting path";
		error->extra = w_begin;
		jsonpath_release(root);
		return NULL;
	}
	return path;
}

jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_eof_escape(const char* position);
jsonpath_error_t JANSSONPATH_NO_EXPORT jsonpath_error_zero_length_escape(const char* position);

static json_t* unescaped_string(string_slice slice, jsonpath_error_t* error){
	assert(slice.begin);
	const char delima = slice.begin[0];
	assert(delima == '"' || delima == '\'');

	const char* iter;
	const char* const end = slice.begin + SLICE_SIZE(slice);

	char* buffer = do_malloc(sizeof(char) * SLICE_SIZE(slice)); // sufficient as we do not include '"' '\''
	char* buffer_iter = buffer;
	json_t* ret = NULL;
	for (iter = slice.begin + 1; iter != end && *iter;) {
		if (*iter == '\\') {  // escape
			++iter;
			assert(iter != end && *iter); // already checked by lexeme.c
			if (*iter == 'x' || isdigit(*iter)) {
				bool is_hex = (*iter == 'x');
				if(is_hex) ++iter;
				const int max_len = is_hex ? 2 : 3;
				int (*pred)(int) = is_hex ? isxdigit : isdigit;
				int radix = is_hex ? 16 : 10;
				int i;
				char number[4] = { 0 };
				for (i = 0; i < max_len && iter != end && !*iter && pred(*iter); i++) {
					number[i] = *iter;
					++iter;
				}
				if (i == 0) {
					*error = jsonpath_error_zero_length_escape(iter);
					break;
				}

				long value = strtol(number, NULL, radix);
				if (value > UCHAR_MAX) {
					error->abort = true;
					error->code = 0x86;
					error->reason = "not representable charactor in escape sequnence";
					error->extra = (void*)iter;
					break;
				}
				*buffer_iter = (char)value;
				++buffer_iter;
			}else if(*iter == '\\' || *iter == '\'' || *iter == '\"' || *iter == '\?'){
				*buffer_iter = *iter;
				++buffer_iter;
				++iter;
			}else if (*iter == '0') {
				*buffer_iter = '\0';
				++buffer_iter;
				++iter;
			}else{
				static const char esc_map[26] = {//abfnrtv
					0,['a' - 'a'] = '\a',['f' - 'a'] = '\f',['n' - 'a'] = '\n',['r' - 'a'] = '\r',['t' - 'a'] = '\t',['v' - 'a'] = '\v'
				};
				char result = 0;
				if (isalpha(*iter))result = esc_map[*iter - 'a'];
				if(!result){
					error->abort = true;
					error->code = 0x84;
					error->reason = "unknown escape";
					error->extra = (void*)iter;
					break;
				}
				*buffer_iter = result;
				++buffer_iter;
				++iter;
			}
		}else if (*iter == delima) {
			// we do not fill zero as we are using json_stringn
			break;
		}else {
			*buffer_iter = *iter;
			++buffer_iter;
			++iter;
		}
	}

	if (!error->abort) {
		assert(*iter == delima && iter + 1 == end);
		ret = json_stringn(buffer, buffer_iter - buffer);
	}
	do_free(buffer);
	return ret;
}

static json_t* try_parse_constant(string_slice slice, jsonpath_error_t* error){
	if (is_number(slice)) {
		size_t i;
		bool is_real_number = false;
		for (i = 0; i < SLICE_SIZE(slice); ++i) {
			if (slice.begin[i] == '.') {
				is_real_number = true;
				break;
			}
		}
		const char* begin = slice.begin;
		char* end = (char*)slice.end; // stupid stdlib
		if(is_real_number){
			double value = strtod(begin, &end);
			assert(end == slice.end);
			return json_real(value);
		}else{
			long long value = strtoll(begin, &end, 10);
			assert(end == slice.end);
			return json_integer(value);
		}
	}
	else if (is_string(slice)) {
		return unescaped_string(slice, error);
	}
	else if (!slice_cstr_cmp(slice, "true")) {
		return json_true();
	}
	else if (!slice_cstr_cmp(slice, "false")) {
		return json_false();
	}
	return NULL;
}

static jsonpath_t* parse_unary(const char** pw_begin, const char* w_end, jsonpath_error_t* error) {
	path_unary_tag_t unary_op = map_to_unary_op(word_peek);
	if (unary_op != UNARY_MAX){
		go_next();
		if (error->abort) return NULL;
	}
	jsonpath_t* inner = NULL;

	if (map_to_unary_op(word_peek) != UNARY_MAX){
		inner = parse_unary(pw_begin, w_end, error);
	}else{
		json_t* constant = try_parse_constant(word_peek, error);
		if (error->abort) {
			return NULL;
		}
		if (constant) {
			go_next();
			if (error->abort) {
				json_decref(constant);
				return NULL;
			}
			inner = make_const(constant);
		}
		else {
			inner = parse_path(pw_begin, w_end, error);
			if (!inner) return NULL;
		}
	}
	
	jsonpath_t* ret = unary_op != UNARY_MAX ? build_unary(unary_op, inner) : inner;
	return ret;
	
}

#define child_node(down_grade, w_begin, w_end, precedence, error) (down_grade?parse_unary(&w_begin, w_end, error):parse_binary(&w_begin, w_end, precedence + 1, error))

// they have lower precedence than all others
static jsonpath_t* parse_binary(const char** pw_begin, const char* w_end, int precedence, jsonpath_error_t* error){
	bool down_grade = binary_precedence_max == precedence;
	jsonpath_t* left_node = child_node(down_grade, w_begin, w_end, precedence, error);
	if (!left_node) return NULL;
	path_binary_tag_t bin_op = map_to_bin_op(word_peek);
	int curr_precedence = binary_precedence[bin_op];
	while (curr_precedence == precedence) {
		go_next();
		if (error->abort) break;
		jsonpath_t* right_node = child_node(down_grade, w_begin, w_end, precedence, error);
		if (!right_node)break;
		left_node = build_binary(bin_op, left_node, right_node);
		bin_op = map_to_bin_op(word_peek);
		curr_precedence = binary_precedence[bin_op];
	}
	if(error->abort){
		jsonpath_release(left_node);
		return NULL;
	}
	return left_node;
}

JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile_ranged(const char* jsonpath_begin, const char** pjsonpath_end, jsonpath_error_t* error) {
	*error = jsonpath_error_ok;
	const char** pw_begin = &jsonpath_begin;
	const char* w_end = pjsonpath_end ? *pjsonpath_end : NULL;
	init_peek();
	jsonpath_t* ret = NULL;

	// do ... while(0) works as try, break as throw, below as finally
	do {
		if (error->abort) break;
		ret = parse_binary(&w_begin, w_end, 0, error);
		if (!ret) break;
		if (!IS_SLICE_EMPTY(word_peek)) {
			jsonpath_release(ret);
			ret = NULL;
			error->abort = true;
			error->code = 0x91;
			error->reason = "jsonpath not ended correctly";
			error->extra = (void*)w_begin;
		}
	} while (0);
	if(pjsonpath_end){
		*pjsonpath_end = w_begin;
	}
	return ret;
}

JANSSONPATH_EXPORT jsonpath_t* jsonpath_compile(const char* jsonpath_begin, jsonpath_error_t* error){
	return jsonpath_compile_ranged(jsonpath_begin, NULL, error);
}

JANSSONPATH_EXPORT jsonpath_t* jsonpath_parse(const char** p_jsonpath_begin, jsonpath_error_t* error) {
	const char* end = NULL;
	jsonpath_t* ret= jsonpath_compile_ranged(*p_jsonpath_begin, &end, error);
	*p_jsonpath_begin = end;
	return ret;
}
