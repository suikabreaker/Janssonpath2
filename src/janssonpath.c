#include "janssonpath_private.h"
#include "memory.h"

// todo: add optional support for wildcard and regular expression
// we allow wild card in subscribe string indexing like ["test_*"], but not in dot indexing
// because it causes ambiguity
// should we support [$.pattern] where $.pattern is a wildcard pattern? It could be problematical

// structures are not of large size. use return value rather than pointer
// and potentially it can be optimized out with RVO
// functions below steals ownership of passed in values. remember to incref them if it will be used elsewhere.

void EXPORT jsonpath_release(jsonpath_t* jsonpath) {
	// TODO
	do_free(jsonpath);
}

// use static to expect inline optimization

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

static path_index_t build_range_index(jsonpath_t* begin, jsonpath_t* end) {
	path_index_t ret = { INDEX_SUB_RANGE, {.range = {begin, end}} };
	return ret;
}

static void path_index_release(path_index_t* path){
	switch(path->tag){
	case INDEX_DOT:
	case INDEX_DOT_RECURSIVE:
		json_decref(path->simple_index);
		break;
	case INDEX_SUB_EXP:
	case INDEX_FILTER:
		jsonpath_release(path->expression);
		break;
	case INDEX_SUB_RANGE:
		jsonpath_release(path->range[0]);
		jsonpath_release(path->range[1]);
		break;
	default:{
		assert(false);
		break;
	}
	}
}

static path_unary_t build_unary(path_unary_tag_t type, jsonpath_t* oprand){
	path_unary_t ret = { type,oprand };
	return ret;
}

static path_binary_t build_binary(path_unary_tag_t type, jsonpath_t* lhs, jsonpath_t* rhs) {
	path_binary_t ret = { type, lhs, rhs };
	return ret;
}

static path_arbitrary_t build_func_call(char * func_name){ // note func_name will be deleted, make copy!
	static const nodes_capacity_default = 4;// it should be sufficient for most call
	jsonpath_t** nodes = do_malloc(sizeof(jsonpath_t*) * nodes_capacity_default);
	path_arbitrary_t ret = { ARB_FUNC, func_name, nodes, 0, nodes_capacity_default };
}

static void add_oprand_arbitrary(path_arbitrary_t* arbitrary, jsonpath_t* oprand) {
	if (arbitrary->size + 1 > arbitrary->capacity) {
		jsonpath_t** new_nodes = do_malloc(sizeof(jsonpath_t*) * arbitrary->capacity * 2);
		memcpy(new_nodes, arbitrary->nodes, arbitrary->size);
		do_free(new_nodes);
		arbitrary->nodes = new_nodes;
	}
	arbitrary->nodes[arbitrary->size] = oprand;
	++arbitrary->size;
}

static void arbitray_release(path_arbitrary_t* arbitrary){
	size_t i;
	for(i=0;i<arbitrary->size;++i){
		jsonpath_release(arbitrary->nodes[i]);
	}
	do_free(arbitrary->nodes);
}

static path_single_t make_root(){
	path_single_t ret = { SINGLE_ROOT, NULL };
	return ret;
}

static path_single_t make_curr() {
	path_single_t ret = { SINGLE_CURR, NULL };
	return ret;
}

static void single_release(path_single_t* single){
	if (single->tag != SINGLE_CONST)return; // json_decref on null is safe, should we remove this condition?
	json_decref(single->constant);
}
